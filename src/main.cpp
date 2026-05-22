#include "Arduino.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "secrets.h"
#include "config.h"
#include "ca_cert.h"

typedef struct {
  uint8_t frame_ctrl[2];
  uint8_t duration[2];
  uint8_t addr1[6];
  uint8_t addr2[6];
  uint8_t addr3[6];
  uint8_t seq_ctrl[2];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0];
} wifi_ieee80211_packet_t;

// ===================== DETECTION QUEUE =====================
// ISR-safe queue: sniffer writes here, loop() publishes
#define QUEUE_SIZE 16

typedef struct {
  int     phoneIndex;
  int8_t  rssi;
  float   distance;
} DetectionEvent;

static DetectionEvent eventQueue[QUEUE_SIZE];
static volatile int   queueHead = 0;
static volatile int   queueTail = 0;

static inline bool queueFull()  { return ((queueTail + 1) % QUEUE_SIZE) == queueHead; }
static inline bool queueEmpty() { return queueHead == queueTail; }

// Called from ISR — only enqueue, no Serial/MQTT
static inline void enqueueEvent(int phoneIdx, int8_t rssi, float distance) {
  if (!queueFull()) {
    eventQueue[queueTail] = { phoneIdx, rssi, distance };
    queueTail = (queueTail + 1) % QUEUE_SIZE;
  }
}

// Called from loop() — safe to use Serial/MQTT
static bool dequeueEvent(DetectionEvent* out) {
  if (queueEmpty()) return false;
  *out = eventQueue[queueHead];
  queueHead = (queueHead + 1) % QUEUE_SIZE;
  return true;
}

// ===================== DEVICE IDENTITY =====================
String myName = "UNKNOWN";
float  myX    = 0.0;
float  myY    = 0.0;

// ===================== TIMESTAMP =====================
String getTimestamp() {
  struct tm timeinfo;
  char timestamp[30] = "unknown";
  if (getLocalTime(&timeinfo)) {
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  }
  return String(timestamp);
}

// ===================== MQTT (med TLS) =====================
static WiFiClientSecure tlsClient;
static PubSubClient     mqttClient(tlsClient);

// ===================== AFSTAND FRA RSSI =====================
float calculateDistance(int rssi, int txPower = -59, float n = 2.5) {
  return pow(10.0, (txPower - rssi) / (10.0 * n));
}

// ===================== DEVICE ID =====================
void espId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);

  Serial.printf("[BOOT] Min MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  for (int i = 0; i < 3; i++) {
    if (memcmp(mac, espMACs[i], 6) == 0) {
      myName = espNames[i];
      myX    = espPositions[i][0];
      myY    = espPositions[i][1];
      Serial.printf("[BOOT] %s på position (%.1f, %.1f)\n",
        myName.c_str(), myX, myY);
      return;
    }
  }
  Serial.println("[BOOT] ADVARSEL — MAC ikke genkendt i secrets.h!");
}

// ===================== SNIFFER CALLBACK =====================
// Runs in ISR context — NO Serial, NO MQTT, NO getLocalTime here
void IRAM_ATTR snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) return;

  wifi_promiscuous_pkt_t*  pkt  = (wifi_promiscuous_pkt_t*)buf;
  wifi_ieee80211_packet_t* ipkt = (wifi_ieee80211_packet_t*)pkt->payload;
  wifi_ieee80211_mac_hdr_t* hdr = &ipkt->hdr;

  uint8_t* mac = hdr->addr2;
  if (mac[0] & 0x01) return;  // Filtrer broadcast/multicast

  int8_t rssi = pkt->rx_ctrl.rssi;

  for (int i = 0; i < knownCount; i++) {
    if (memcmp(mac, knownMACs[i], 6) == 0) {
      float distance = calculateDistance(rssi);
      enqueueEvent(i, rssi, distance);
      return;
    }
  }
}

// ===================== WIFI =====================
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, WIFIPASSWORD);
  Serial.print("[WIFI] Forbinder");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n[WIFI] Forbundet: " + WiFi.localIP().toString());
}

// ===================== MQTT RECONNECT =====================
void reconnectMQTT() {
  int attempts = 0;
  while (!mqttClient.connected() && attempts < 5) {
    Serial.printf("[MQTT] Forbinder til %s:%d ...\n", MQTT_HOST, MQTT_PORT);
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("[MQTT] Forbundet!");
    } else {
      Serial.printf("[MQTT] Fejlede, rc=%d — prøver igen om 5 sek\n", mqttClient.state());
      attempts++;
      delay(5000);
    }
  }
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Kunne ikke forbinde efter 5 forsøg — fortsætter uden MQTT");
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("========================================");
  Serial.println("[BOOT] " + String(DEVICENAME) + " starter");
  Serial.println("========================================");

  initWiFi();

  tlsClient.setCACert(MQTT_CA_CERT);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setBufferSize(512);  // Ensure payload fits with MQTT overhead

  espId();
  reconnectMQTT();

  // Synkroniser tid
  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  // Wait briefly for NTP to sync
  delay(1500);
  Serial.println("[NTP] Tid synkroniseret");

  // Start sniffer — WiFi forbliver forbundet
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  Serial.println("[SNIFFER] Kørende — lytter på kanal 1");
  Serial.println("========================================");
}

// ===================== LOOP =====================
void loop() {
  // Hold MQTT forbindelsen i live
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Forbindelse tabt — genopretter...");
    reconnectMQTT();
  }
  mqttClient.loop();

  // Drain detection queue and publish
  DetectionEvent evt;
  while (dequeueEvent(&evt)) {
    Serial.printf("[SNIFFER] Telefon %d — RSSI: %d dBm  ~%.1f m\n",
      evt.phoneIndex + 1, evt.rssi, evt.distance);

    if (mqttClient.connected()) {
      char payload[256];
      int len = snprintf(payload, sizeof(payload),
        "{\"device\":\"%s\",\"x\":%.1f,\"y\":%.1f,\"telefon\":%d,\"rssi\":%d,\"distance\":%.2f,\"timestamp\":\"%s\"}",
        myName.c_str(), myX, myY,
        evt.phoneIndex + 1, evt.rssi, evt.distance,
        getTimestamp().c_str());

      if (len >= (int)sizeof(payload)) {
        Serial.println("[MQTT] ADVARSEL: payload afkortet!");
      }

      mqttClient.publish("/devices/device03", payload);
      Serial.println("[MQTT] Sendt: " + String(payload));
    } else {
      Serial.println("[MQTT] Ikke forbundet — kan ikke sende");
    }
  }

  // Channel hopping — disable sniffer briefly to avoid mid-hop callbacks
  static uint8_t        channel = 1;
  static unsigned long  lastHop = 0;

  if (millis() - lastHop > 1000) {
    channel = (channel % 13) + 1;
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    lastHop = millis();
  }
}