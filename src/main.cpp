#include "Arduino.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"
#include "config.h"

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

// ===================== MQTT (uden TLS) =====================
static WiFiClient   wifiClient;
static PubSubClient mqttClient(wifiClient);

// ===================== KENDTE TELEFONER =====================
uint8_t knownMACs[][6] = {
  { 0x0C, 0xE4, 0xA0, 0x77, 0x47, 0xB0 },  // Telefon 1
};
int knownCount = 1;

// ===================== AFSTAND FRA RSSI =====================
float calculateDistance(int rssi, int txPower = -59, float n = 2.5) {
  return pow(10.0, (txPower - rssi) / (10.0 * n));
}

// ===================== SNIFFER CALLBACK =====================
void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) return;

  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  wifi_ieee80211_packet_t* ipkt = (wifi_ieee80211_packet_t*)pkt->payload;
  wifi_ieee80211_mac_hdr_t* hdr = &ipkt->hdr;

  uint8_t* mac = hdr->addr2;
  if (mac[0] & 0x01) return;  // Filtrer broadcast

  int8_t rssi = pkt->rx_ctrl.rssi;

  for (int i = 0; i < knownCount; i++) {
    if (memcmp(mac, knownMACs[i], 6) == 0) {
      float distance = calculateDistance(rssi);
      Serial.printf("[SNIFFER] Telefon %d — RSSI: %d dBm  ~%.1f m\n",
        i + 1, rssi, distance);

      if (mqttClient.connected()) {
        char payload[128];
        sprintf(payload,
          "{\"id\":%d,\"telefon\":%d,\"rssi\":%d,\"distance\":%.2f}",
          DEVICENAME,i + 1, rssi, distance);
        mqttClient.publish("/devices/indoor/rssi", payload);
        Serial.println("[MQTT] Sendt: " + String(payload));
      } else {
        Serial.println("[MQTT] Ikke forbundet — kan ikke sende");
      }
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
  while (!mqttClient.connected()) {
    Serial.printf("[MQTT] Forbinder til %s:%d ...\n", MQTT_HOST, MQTT_PORT);
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("[MQTT] Forbundet!");
    } else {
      Serial.printf("[MQTT] Fejlede, rc=%d — prøver igen om 5 sek\n", mqttClient.state());
      delay(5000);
    }
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("========================================");
  Serial.println("[BOOT] " + String(DEVICENAME) + " starter");
  Serial.println("========================================");

  // Forbind WiFi og MQTT først
  initWiFi();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  reconnectMQTT();

  // Synkroniser tid
  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
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

  // Channel hopping
  static uint8_t channel = 1;
  static unsigned long lastHop = 0;

  if (millis() - lastHop > 1000) {
    channel = (channel % 13) + 1;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    lastHop = millis();
  }
}