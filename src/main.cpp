#include "Arduino.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <set>
#include <string>
#include "secrets.h"
#include "config.h"
#include "ca_cert.h"
#include "mbedtls/sha256.h"

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
#define QUEUE_SIZE 16

typedef struct {
  char    anonId[65];
  int8_t  rssi;
  float   distance;
  bool    isKnown;
  int     knownIndex; // index i knownMACs
} DetectionEvent;

static DetectionEvent eventQueue[QUEUE_SIZE];
static volatile int   queueHead = 0;
static volatile int   queueTail = 0;

static inline bool queueFull()  { return ((queueTail + 1) % QUEUE_SIZE) == queueHead; }
static inline bool queueEmpty() { return queueHead == queueTail; }

static inline void enqueueEvent(const char* anonId, int8_t rssi, float distance, bool isKnown, int knownIndex) {
  if (!queueFull()) {
    strncpy(eventQueue[queueTail].anonId, anonId, 65);
    eventQueue[queueTail].rssi       = rssi;
    eventQueue[queueTail].distance   = distance;
    eventQueue[queueTail].isKnown    = isKnown;
    eventQueue[queueTail].knownIndex = knownIndex;
    queueTail = (queueTail + 1) % QUEUE_SIZE;
  }
}

static bool dequeueEvent(DetectionEvent* out) {
  if (queueEmpty()) return false;
  *out = eventQueue[queueHead];
  queueHead = (queueHead + 1) % QUEUE_SIZE;
  return true;
}

// ===================== DEVICE TRACKING =====================
static std::set<std::string> seenDevices;
static unsigned long lastPublishTime = 0;
#define PUBLISH_INTERVAL_MS (30UL * 1000UL) // 30 sekunder

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

// ===================== HASH MAC =====================
void hashMAC(uint8_t* mac, char* output) {
  uint8_t input[6 + 20];
  memcpy(input, mac, 6);
  memcpy(input + 6, SALT, strlen(SALT));

  byte hash[32];
  mbedtls_sha256_ret(input, 6 + strlen(SALT), hash, 0);

  for (int i = 0; i < 32; i++) {
    sprintf(output + i * 2, "%02x", hash[i]);
  }
  output[64] = '\0';
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
void IRAM_ATTR snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) return;

  wifi_promiscuous_pkt_t*  pkt  = (wifi_promiscuous_pkt_t*)buf;
  wifi_ieee80211_packet_t* ipkt = (wifi_ieee80211_packet_t*)pkt->payload;
  wifi_ieee80211_mac_hdr_t* hdr = &ipkt->hdr;

  uint8_t* mac = hdr->addr2;
  if (mac[0] & 0x01) return;  // Filtrer broadcast/multicast

  int8_t rssi     = pkt->rx_ctrl.rssi;
  float distance  = calculateDistance(rssi);

  // Tjek om det er en known device
  bool isKnown    = false;
  int knownIndex  = -1;
  for (int i = 0; i < knownMACCount; i++) {
    if (memcmp(mac, knownMACs[i], 6) == 0) {
      isKnown    = true;
      knownIndex = i;
      break;
    }
  }

  char anonId[65];
  hashMAC(mac, anonId);

  enqueueEvent(anonId, rssi, distance, isKnown, knownIndex);
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
  mqttClient.setBufferSize(512);

  espId();
  reconnectMQTT();

  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  delay(1500);
  Serial.println("[NTP] Tid synkroniseret");

  lastPublishTime = millis();

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  Serial.println("[SNIFFER] Kørende — lytter på kanal 1");
  Serial.println("========================================");
}

// ===================== LOOP =====================
void loop() {
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Forbindelse tabt — genopretter...");
    reconnectMQTT();
  }
  mqttClient.loop();

  // Drain detection queue
  DetectionEvent evt;
  while (dequeueEvent(&evt)) {
    seenDevices.insert(std::string(evt.anonId));

    Serial.printf("[SNIFFER] Device %s — RSSI: %d dBm  ~%.1f m%s\n",
      evt.anonId,
      evt.rssi,
      evt.distance,
      evt.isKnown ? "  *** KNOWN DEVICE ***" : ""
    );

    // Publicer known device med det samme — med eget topic
    if (evt.isKnown && mqttClient.connected()) {
      char payload[192];
      snprintf(payload, sizeof(payload),
        "{\"device\":\"%s\",\"rssi\":%d,\"distance\":%.2f,\"timestamp\":\"%s\"}",
        knownNames[evt.knownIndex],
        evt.rssi,
        evt.distance,
        getTimestamp().c_str()
      );
      String topic = "/devices/device03/" + myName + "/known";
      mqttClient.publish(topic.c_str(), payload);
      Serial.println("[MQTT] Known device sendt: " + String(payload));
    }
  }

  // ===================== RAPPORT HVERT 30. SEK =====================
  if (millis() - lastPublishTime >= PUBLISH_INTERVAL_MS) {
    if (mqttClient.connected()) {
      char payload[128];
      snprintf(payload, sizeof(payload),
        "{\"node\":\"%s\",\"x\":%.1f,\"y\":%.1f,\"unique_devices\":%d,\"timestamp\":\"%s\"}",
        myName.c_str(),
        myX,
        myY,
        (int)seenDevices.size(),
        getTimestamp().c_str()
      );
      String topic = "/devices/device03/" + myName + "/count";
      mqttClient.publish(topic.c_str(), payload);
      Serial.printf("[MQTT] Rapport sendt: %d unikke enheder\n", (int)seenDevices.size());
      Serial.println("[MQTT] Payload: " + String(payload));
    } else {
      Serial.println("[MQTT] Rapport — ikke forbundet, kan ikke sende");
    }
    seenDevices.clear();
    lastPublishTime = millis();
  }

  // Channel hopping
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