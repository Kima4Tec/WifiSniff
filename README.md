# A) Teknisk: Hvordan finder man position uden GPS?

## Introduktion
Der findes flere metoder til at estimere position uden brug af GPS. I dette projekt undersøges især teknologier baseret på Wi-Fi og ESP32-enheder. Undersøgelse har strukket sig over tre dage, blandet med et andet smiley-projekt

## Logbog
### Dag 1
Vi fandt ud at sniffe os til flere forskellige enheder i nærheden. Havde tanker på sikkerhed med hashing. Vi opsatte en mqtt server på pc. Herefter satte vi programmet til at søge på specifikke mobiler, med kendte mac-adresser.
### Dag 2
Vi er gået videre med vores sniffer-program, der bruger mqtt-serveren som modtager af de tre esp32s data med en broker (program skrevet i python), som samler data fra de tre esp32 og udregner afstand til kendt mobil vha triangulering. 
Vi krypterer alle fundne mac-adresser med hashing, og da vi fandt ud af, at det ikke er sikkert nok, har vi også saltet disse data. Vi har finder kun afstand fra en kendt mobil og registrerer desuden antal af fundne devices uden at bruge deres mac-adresser.
Vi opstartede desuden nyt projekt, hvor vi undersøger mulighederne med esp-now. 


### Dag 3

---

# Sammenligning af teknologier til indoor positioning med ESP32

| Teknologi | Hvordan virker det? | Fordele | Ulemper | Egnet til projektet? |
|---|---|---|---|---|
| ESP32 → MQTT → Broker → Triangulering | Alle ESP32-enheder sniffer WiFi-signaler og sender RSSI-data direkte til MQTT-server. Broker/backend udregner position via trilateration. | Simpel arkitektur, let debugging, central databehandling, nem visualisering, god skalerbarhed, let at integrere med dashboards/databaser | Kræver WiFi-netværk og broker, flere MQTT-forbindelser, mere netværkstrafik, backend skal samle alle målinger | Ja – meget god og stabil løsning |
| ESP-NOW + Master ESP32 + MQTT | Slave-ESP32’er sender RSSI-data til en master via ESP-NOW. Master samler data og sender til MQTT. | Lav latency, mindre netværkstrafik, kun én MQTT-forbindelse, fungerer uden router mellem ESP32’er, mere professionel edge/gateway-arkitektur | Mere kompleks kode, ESP-NOW kræver samme WiFi-kanal, begrænset rækkevidde, sværere debugging | Ja – meget stærk løsning til projektet |
| ESP-MESH | ESP32’er danner selvorganiserende mesh-netværk og videresender data mellem noder til root-node | Stor rækkevidde, selvhelende netværk, god til store områder, robust mod node-fejl | Kompleks opsætning, højere latency, mere RAM/CPU-forbrug, svær debugging | Muligt, men ofte overkill til mindre projekter |
| RTT (Round Trip Time) | Måler tiden et signal bruger på at rejse mellem enheder og tilbage igen | Potentielt mere præcis afstandsbestemmelse end RSSI | ESP32 understøtter ikke præcis hardware-timing til RTT/Fine Timing Measurement (FTM), meget vanskelig implementering, kræver synkronisering | Ikke realistisk til dette projekt |
| RSSI-triangulering | Afstand estimeres ud fra signalstyrke (RSSI), hvorefter position beregnes geometrisk | Simpel implementering, virker med standard ESP32 hardware, ingen aktiv forbindelse nødvendig | Lav præcision, påvirkes af vægge, mennesker og støj, signalstyrke varierer meget | Ja – mest realistiske metode med ESP32 |


Valget faldt først på direkte mqtt-arkitektur

1. **Direkte MQTT-arkitektur**

   * lettest at implementere
   * mest stabil

2. **ESP-NOW med master/slave**

   * mere avanceret
   * mindre netværkstrafik
   * mere interessant arkitektur

RTT er teoretisk mere præcist, men praktisk svært eller umuligt med almindelige ESP32-enheder. Derfor bruges RSSI-triangulering typisk i skoleprojekter og simple indoor positioning-systemer.

---


# RSSI (Wi-Fi signalstyrke)

RSSI (*Received Signal Strength Indicator*) bruges til at estimere afstanden mellem to enheder ud fra signalstyrken.

## Hvordan virker det?
En ESP32 måler styrken på et modtaget Wi-Fi-signal i dBm:

- **-30 dBm** → meget tæt på
- **-90 dBm** → langt væk

Signalstyrken kan derefter omregnes til en estimeret afstand ved hjælp af matematiske modeller som:

- *Log-Distance Path Loss Model*

## Fordele
- Indbygget i ESP32
- Simpel at implementere
- Lavt strømforbrug

## Ulemper
RSSI er upræcist, fordi signalstyrken påvirkes af:

- vægge og beton
- mennesker
- refleksioner
- antennens retning
- møbler og metal

## Præcision
Indendørs præcision ligger typisk på:

- **2–8 meters usikkerhed**

RSSI fungerer derfor bedst til:
- zonedetektion
- rum-positionering
- grove afstandsvurderinger

---

# Triangulering / Trilateration

Triangulering bruges til at finde en position ved hjælp af flere kendte målepunkter.

## Hvordan virker det?
Tre eller flere ESP32-enheder placeres på kendte koordinater:

- `(x1, y1)`
- `(x2, y2)`
- `(x3, y3)`

Den mobile enhed måler RSSI til hver node, hvorefter afstanden estimeres.

Ud fra afstandene beregnes en cirka-position.

## Sikkerhed og præcision

Positioneringen er aldrig 100 % præcis og afhænger af:

- signalforhold
- afstand til målepunkter
- refleksioner
- geometri mellem noderne

### Typiske fejlkilder
- flervejsudbredelse (*multipath*)
- vinkelfejl
- dårlig placering af referencepunkter
- interferens på 2,4 GHz

## Praktisk præcision

| Miljø | Typisk præcision |
|---|---|
| Åbent område | 1–5 meter |
| Indendørs | 2–10 meter |
| RTK-GPS | Centimeter-niveau |

Usikkerheden visualiseres ofte som et fejlområde eller fejlpolygon.

---

# ESP-NOW vs ESP-MESH

ESP-NOW og ESP-MESH er to forskellige kommunikationsmetoder til ESP32.

---

## ESP-NOW

ESP-NOW er en hurtig punkt-til-punkt-protokol uden traditionel Wi-Fi-router.

### Egenskaber
- direkte kommunikation
- meget lav latency
- lavt strømforbrug
- små datapakker

### Positionering
Bruger primært:
- RSSI-baseret afstandsmåling

### Præcision
- ca. **1–5 meter** indendørs

### Ulemper
Signalet påvirkes kraftigt af:
- vægge
- mennesker
- refleksioner

---

## ESP-MESH

ESP-MESH er et netværk, hvor noder videresender data for hinanden.

### Egenskaber
- selvhelende netværk
- stor rækkevidde
- mange noder
- høj skalerbarhed

### Positionering
Kan triangulere ud fra flere faste noder samtidigt.

### Fordele
- mere stabil positionering
- flere referencepunkter
- færre ekstreme fejlmålinger

---

# Sammenligning

| Funktion | ESP-NOW | ESP-MESH |
|---|---|---|
| Batterilevetid | Meget høj | Lav/Medium |
| Skalerbarhed | Begrænset | Meget høj |
| Opsætning | Simpel | Kompleks |
| Båndbredde | Lav | Høj |
| Positioneringsstabilitet | Lav/Medium | Medium/Høj |

---

# Round-Trip Time (RTT / Ping / Latency)

RTT (*Round-Trip Time*) måler den tid, det tager for et signal at rejse fra sender til modtager og tilbage igen.

## Sådan fungerer det

1. Enhed A sender et signal
2. Enhed B svarer straks
3. Enhed A måler tiden
4. Tiden divideres med 2

Afstanden beregnes derefter:

```text
Distance = Tid × Hastighed
```

Da radiobølger bevæger sig med lysets hastighed, kræver præcis RTT målinger på nanosekund-niveau.

---

# RTT på ESP32-WROOM-32 DevKit V1

Den klassiske ESP32-WROOM-32 understøtter **ikke hardwarebaseret RTT/FTM**.

## Problemet
Wi-Fi-signaler bevæger sig ekstremt hurtigt:

- 1 mikrosekund ≈ 300 meter

ESP32’ens software og FreeRTOS giver for stor forsinkelse (*jitter*), hvilket gør softwarebaseret RTT upræcist.

## Resultat
Præcisionen bliver:

- **100+ meters usikkerhed**

Derfor er software-ping ubrugeligt til reel positionering.

---

# Nyere ESP32-chips med RTT-support

Følgende chips understøtter hardwarebaseret Wi-Fi RTT (*802.11mc FTM*):

- ESP32-S2
- ESP32-S3
- ESP32-C3
- ESP32-C6

## Typisk præcision
- **1–2 meter indendørs**

## Krav
- stabilt signal
- line-of-sight
- minimal refleksion

---

# Ultra-Wideband (UWB)

Hvis høj præcision er nødvendig, er UWB den bedste løsning.

## Fordele
- meget præcis afstandsmåling
- påvirkes mindre af refleksioner
- RTT måles direkte i hardware

## Præcision
- ned til **±10 cm**

## Eksempel på modul
- Decawave DWM1000

---

# Andre afstandssensorer

## HC-SR04 Ultralyd

## VL53L0X Time-of-Flight Laser

---

# RSSI-præcision i praksis

RSSI er generelt upræcist til afstandsmåling.

## Typisk usikkerhed
- **2–8 meter indendørs**

## Hvorfor?

### Refleksioner
Wi-Fi-signaler reflekteres af:
- vægge
- metal
- lofter

### Menneskekroppen
Vand absorberer 2,4 GHz-signaler meget effektivt.

### Antennens retning
ESP32’s PCB-antenne ændrer signalstyrken afhængigt af orienteringen.

---

# Hvad virker bedst?

## Bedste metode

| Metode | Præcision |
|---|---|
| RSSI | Lav |
| RTT | Medium/Høj |
| UWB | Meget høj |

## Konklusion
- RSSI er simpelt, men upræcist
- RTT kræver nyere hardware
- UWB giver bedst præcision

---

# Projektstatus

## Vi har implementeret

- ✔ Wi-Fi sniffing
- ✔ MQTT med TLS-kryptering
- ✔ ISR-safe queue
- ✔ RSSI → distance
- ✔ channel hopping
- ✔ node-positioner

---

# Sikkerhed og kryptering

## Krypterede data
Følgende data er beskyttet:

- MQTT payload
- login credentials
- timestamps
- positionsdata

---

# Hashing med SHA-256

MAC-adresser hashes med SHA-256.

## Kan det brydes?
Teknisk set ja — via brute-force.

## Hvorfor?
En MAC-adresse består af 6 bytes:

```text
256^6 ≈ 281 billioner kombinationer
```

Moderne GPU’er kan beregne milliarder af SHA-256 hashes per sekund.

Det gør det muligt at lave:

- brute-force attacks
- rainbow tables

---

# Saltede hashes

For at forbedre sikkerheden anvendes salt.

## Hvorfor salt?
Salt gør det svært at:
- genbruge rainbow tables
- sammenligne hashes direkte
- forudsige output

## Resultat
Sikkerheden øges markant sammenlignet med ren SHA-256 hashing.


## Bilag

### Kode til Slave
```
#include "Arduino.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <mbedtls/md.h>
#include "secrets.h"   // SSID, WIFIPASSWORD, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS
#include "config.h"    // DEVICENAME
#include "ca_cert.h"   // MQTT_CA_CERT

// ===================== CONFIG =====================
// Sæt dette til "slaveA" eller "slaveB" i config.h som SLAVE_ID
// Topic bliver: /sensors/raw/slaveA  eller  /sensors/raw/slaveB
#define MQTT_TOPIC_PREFIX "devices/device03/raw/"

// Slave-position i rummet (meter) — sæt i config.h som SLAVE_X og SLAVE_Y
// Bruges af master til triangulering
// Eks: slaveA = (0.0, 0.0), slaveB = (5.0, 0.0)

// ===================== MAC HASH =====================
// SHA256(MAC) → hex-streng, kun de første 8 tegn bruges som kortID
// Rå MAC forlader aldrig enheden
void macToHash(const uint8_t* mac, char* outHex8) {
  uint8_t digest[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, mac, 6);
  mbedtls_md_finish(&ctx, digest);
  mbedtls_md_free(&ctx);
  // Kun de første 4 bytes (8 hex-tegn) — kort nok til ikke at være identificerbart
  snprintf(outHex8, 9, "%02X%02X%02X%02X",
    digest[0], digest[1], digest[2], digest[3]);
}

// ===================== DETECTION QUEUE =====================
#define QUEUE_SIZE 32

typedef struct {
  char    macHash[9];   // 8 hex chars + null
  int8_t  rssi;
} DetectionEvent;

static DetectionEvent eventQueue[QUEUE_SIZE];
static volatile int   queueHead = 0;
static volatile int   queueTail = 0;

static inline bool queueFull()  { return ((queueTail + 1) % QUEUE_SIZE) == queueHead; }
static inline bool queueEmpty() { return queueHead == queueTail; }

static inline void enqueueEvent(const char* hash, int8_t rssi) {
  if (!queueFull()) {
    strncpy(eventQueue[queueTail].macHash, hash, 9);
    eventQueue[queueTail].rssi = rssi;
    queueTail = (queueTail + 1) % QUEUE_SIZE;
  }
}

static bool dequeueEvent(DetectionEvent* out) {
  if (queueEmpty()) return false;
  *out = eventQueue[queueHead];
  queueHead = (queueHead + 1) % QUEUE_SIZE;
  return true;
}

// ===================== RSSI THROTTLE =====================
// Undgå at oversvømme brokeren — send maks én måling per enhed per interval
#define THROTTLE_MS     500
#define THROTTLE_SLOTS  32

typedef struct {
  char hash[9];
  unsigned long lastSent;
} ThrottleEntry;

static ThrottleEntry throttleTable[THROTTLE_SLOTS];
static int           throttleCount = 0;

bool shouldSend(const char* hash) {
  unsigned long now = millis();
  for (int i = 0; i < throttleCount; i++) {
    if (strcmp(throttleTable[i].hash, hash) == 0) {
      if (now - throttleTable[i].lastSent < THROTTLE_MS) return false;
      throttleTable[i].lastSent = now;
      return true;
    }
  }
  // Ny enhed
  if (throttleCount < THROTTLE_SLOTS) {
    strncpy(throttleTable[throttleCount].hash, hash, 9);
    throttleTable[throttleCount].lastSent = now;
    throttleCount++;
  }
  return true;
}

// ===================== TIMESTAMP =====================
String getTimestamp() {
  struct tm timeinfo;
  char buf[30] = "unknown";
  if (getLocalTime(&timeinfo)) {
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  }
  return String(buf);
}

// ===================== MQTT =====================
static WiFiClientSecure tlsClient;
static PubSubClient     mqttClient(tlsClient);

static volatile uint8_t currentChannel = 1;

// ===================== WIFI IEEE80211 STRUCTS =====================
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

// ===================== SNIFFER CALLBACK =====================
// Kører i ISR-kontekst — ingen Serial, ingen MQTT, ingen heap-allokering
void IRAM_ATTR snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) return;

  wifi_promiscuous_pkt_t*   pkt  = (wifi_promiscuous_pkt_t*)buf;
  wifi_ieee80211_packet_t*  ipkt = (wifi_ieee80211_packet_t*)pkt->payload;
  wifi_ieee80211_mac_hdr_t* hdr  = &ipkt->hdr;

  uint8_t* mac = hdr->addr2;

  // Filtrer broadcast, multicast og locally-administered (randomiserede) MAC'er
  if (mac[0] & 0x01) return;  // Broadcast/multicast
  if (mac[0] & 0x02) return;  // Locally-administered (randomiseret)

  int8_t rssi = pkt->rx_ctrl.rssi;

  // Hash i ISR er tungt — brug en lille statisk buffer
  // Vi sender raw mac bytes til queue og hasher i loop()
  // For at undgå heap: pack mac i 6 bytes + rssi
  if (!queueFull()) {
    // Gem rå MAC midlertidigt — hashes i loop() uden for ISR
    // Vi genbruger macHash-feltet til rå bytes (første 6 bytes)
    memcpy(eventQueue[queueTail].macHash, mac, 6);
    eventQueue[queueTail].macHash[6] = '\0';  // markér som rå
    eventQueue[queueTail].rssi = rssi;
    queueTail = (queueTail + 1) % QUEUE_SIZE;
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
    String clientId = String(SLAVE_ID) + "-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("[MQTT] Forbundet!");
    } else {
      Serial.printf("[MQTT] Fejlede, rc=%d — prøver igen om 5 sek\n", mqttClient.state());
      attempts++;
      delay(5000);
    }
  }
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Kunne ikke forbinde — fortsætter uden MQTT");
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("========================================");
  Serial.printf("[BOOT] %s starter (slave: %s)\n", DEVICENAME, SLAVE_ID);
  Serial.printf("[BOOT] Position: (%.1f, %.1f)\n", (float)SLAVE_X, (float)SLAVE_Y);
  Serial.println("========================================");

  initWiFi();

  tlsClient.setCACert(MQTT_CA_CERT);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setBufferSize(256);

  reconnectMQTT();

  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  delay(1500);
  Serial.println("[NTP] Tid synkroniseret");

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  Serial.println("[SNIFFER] Kørende");
  Serial.println("========================================");
}

// ===================== LOOP =====================
void loop() {
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Forbindelse tabt — genopretter...");
    reconnectMQTT();
  }
  mqttClient.loop();

  // Drain queue — hash MAC og publish
  while (!queueEmpty()) {
    DetectionEvent evt;
    evt = eventQueue[queueHead];
    queueHead = (queueHead + 1) % QUEUE_SIZE;

    // Hash de 6 rå MAC-bytes
    char macHash[9];
    macToHash((const uint8_t*)evt.macHash, macHash);

    // Throttle — undgå flood
    if (!shouldSend(macHash)) continue;

    Serial.printf("[SNIFFER] Hash: %s  RSSI: %d dBm  Kanal: %d\n",
      macHash, evt.rssi, currentChannel);

    if (mqttClient.connected()) {
      char topic[64];
      snprintf(topic, sizeof(topic), "%s%s", MQTT_TOPIC_PREFIX, SLAVE_ID);

      char payload[192];
      snprintf(payload, sizeof(payload),
        "{\"slave\":\"%s\",\"x\":%.2f,\"y\":%.2f,\"macHash\":\"%s\",\"rssi\":%d,\"ts\":\"%s\"}",
        SLAVE_ID, (float)SLAVE_X, (float)SLAVE_Y,
        macHash, evt.rssi, getTimestamp().c_str());

      mqttClient.publish(topic, payload);
    }
  }

  // Channel hopping — 1, 6, 11 (de tre primære kanaler)
  static const uint8_t channels[] = {1, 6, 11};
  static uint8_t       channelIdx = 0;
  static unsigned long lastHop    = 0;

  if (millis() - lastHop > 150) {
    channelIdx   = (channelIdx + 1) % 3;
    currentChannel = channels[channelIdx];
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    lastHop = millis();
  }
}

```

### Kode til Master
```
#include "Arduino.h"
#include "esp_wifi.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <mbedtls/md.h>
#include <ArduinoJson.h>
#include "secrets.h"   // SSID, WIFIPASSWORD, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS
#include "config.h"    // DEVICENAME
#include "ca_cert.h"   // MQTT_CA_CERT

// ===================== CONFIG =====================
#define TOPIC_SLAVE_A    "devices/device03/raw/slaveA"
#define TOPIC_SLAVE_B    "devices/device03/raw/slaveB"
#define TOPIC_POSITIONS  "devices/device03/positions"

// Master-position i rummet (meter)
#define MASTER_X   0.0f
#define MASTER_Y   5.0f

#define WINDOW_MS   2000   // Tidsvindue for at kombinere målinger (ms)
#define MIN_RSSI    -85    // Svageste signal der accepteres
#define MAX_DEVICES  40
#define THROTTLE_MS  500   // Maks én måling per enhed per interval (master sniffer)

// ===================== DAGLIGT SALT =====================
static char dailySalt[16] = "SALT_INIT";

void updateDailySalt() {
  struct tm t;
  if (getLocalTime(&t)) {
    snprintf(dailySalt, sizeof(dailySalt), "%04d%02d%02d",
      t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
  }
}

// ===================== ANONYMISERING =====================
void makeDevId(const char* macHash, char* outDevId) {
  char input[32];
  snprintf(input, sizeof(input), "%s%s", macHash, dailySalt);
  uint8_t digest[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const uint8_t*)input, strlen(input));
  mbedtls_md_finish(&ctx, digest);
  mbedtls_md_free(&ctx);
  snprintf(outDevId, 10, "DEV-%02X%02X", digest[0], digest[1]);
}

// ===================== MAC HASH =====================
void macToHash(const uint8_t* mac, char* outHex8) {
  uint8_t digest[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, mac, 6);
  mbedtls_md_finish(&ctx, digest);
  mbedtls_md_free(&ctx);
  snprintf(outHex8, 9, "%02X%02X%02X%02X",
    digest[0], digest[1], digest[2], digest[3]);
}

// ===================== DEVICE TABLE =====================
typedef struct {
  char          macHash[9];
  char          devId[10];

  // Slave A måling
  float         axPos, ayPos;
  int8_t        aRssi;
  unsigned long aTime;

  // Slave B måling
  float         bxPos, byPos;
  int8_t        bRssi;
  unsigned long bTime;

  // Master (lokal) måling
  float         mxPos, myPos;
  int8_t        mRssi;
  unsigned long mTime;

  // Beregnet position
  float         estX, estY;
  bool          published;
} DeviceEntry;

static DeviceEntry devices[MAX_DEVICES];
static int         deviceCount = 0;

DeviceEntry* findOrCreate(const char* macHash) {
  for (int i = 0; i < deviceCount; i++) {
    if (strcmp(devices[i].macHash, macHash) == 0) return &devices[i];
  }
  if (deviceCount >= MAX_DEVICES) {
    int oldest = 0;
    unsigned long minTime = ULONG_MAX;
    for (int i = 0; i < MAX_DEVICES; i++) {
      unsigned long t = max({devices[i].aTime, devices[i].bTime, devices[i].mTime});
      if (t < minTime) { minTime = t; oldest = i; }
    }
    memset(&devices[oldest], 0, sizeof(DeviceEntry));
    strncpy(devices[oldest].macHash, macHash, 9);
    makeDevId(macHash, devices[oldest].devId);
    return &devices[oldest];
  }
  DeviceEntry* e = &devices[deviceCount++];
  memset(e, 0, sizeof(DeviceEntry));
  strncpy(e->macHash, macHash, 9);
  makeDevId(macHash, e->devId);
  return e;
}

// ===================== TRIANGULERING =====================
// Weighted centroid fra op til 3 punkter
// Vægt = 1 / distance² — stærkt signal trækker mere

float rssiToDistance(int8_t rssi, int txPower = -59, float n = 2.5) {
  return pow(10.0f, (txPower - rssi) / (10.0f * n));
}

bool triangulate(DeviceEntry* dev, float* outX, float* outY) {
  unsigned long now = millis();
  bool hasA = (dev->aTime > 0) && ((now - dev->aTime) < WINDOW_MS) && (dev->aRssi > MIN_RSSI);
  bool hasB = (dev->bTime > 0) && ((now - dev->bTime) < WINDOW_MS) && (dev->bRssi > MIN_RSSI);
  bool hasM = (dev->mTime > 0) && ((now - dev->mTime) < WINDOW_MS) && (dev->mRssi > MIN_RSSI);

  if (!hasA && !hasB && !hasM) return false;

  float totalW = 0;
  float sumX   = 0;
  float sumY   = 0;

  auto addPoint = [&](float x, float y, int8_t rssi) {
    float d = rssiToDistance(rssi);
    float w = 1.0f / (d * d + 0.001f);
    sumX   += w * x;
    sumY   += w * y;
    totalW += w;
  };

  if (hasA) addPoint(dev->axPos, dev->ayPos, dev->aRssi);
  if (hasB) addPoint(dev->bxPos, dev->byPos, dev->bRssi);
  if (hasM) addPoint(dev->mxPos, dev->myPos, dev->mRssi);

  *outX = sumX / totalW;
  *outY = sumY / totalW;
  return true;
}

// ===================== SNIFFER (master lokal) =====================
// ISR-safe queue — samme mønster som slave
#define QUEUE_SIZE 32

typedef struct {
  uint8_t mac[6];
  int8_t  rssi;
} SniffEvent;

static SniffEvent sniffQueue[QUEUE_SIZE];
static volatile int qHead = 0;
static volatile int qTail = 0;

static inline bool qFull()  { return ((qTail + 1) % QUEUE_SIZE) == qHead; }
static inline bool qEmpty() { return qHead == qTail; }

typedef struct {
  uint8_t frame_ctrl[2], duration[2], addr1[6], addr2[6], addr3[6], seq_ctrl[2];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0];
} wifi_ieee80211_packet_t;

void IRAM_ATTR snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) return;
  wifi_promiscuous_pkt_t*   pkt  = (wifi_promiscuous_pkt_t*)buf;
  wifi_ieee80211_packet_t*  ipkt = (wifi_ieee80211_packet_t*)pkt->payload;
  uint8_t* mac = ipkt->hdr.addr2;
  if (mac[0] & 0x01) return;  // Broadcast
  if (mac[0] & 0x02) return;  // Randomiseret
  if (!qFull()) {
    memcpy(sniffQueue[qTail].mac, mac, 6);
    sniffQueue[qTail].rssi = pkt->rx_ctrl.rssi;
    qTail = (qTail + 1) % QUEUE_SIZE;
  }
}

// Throttle til master-sniffer
#define THROTTLE_SLOTS 32
typedef struct { char hash[9]; unsigned long lastSent; } ThrottleEntry;
static ThrottleEntry throttleTable[THROTTLE_SLOTS];
static int           throttleCount = 0;

bool shouldProcess(const char* hash) {
  unsigned long now = millis();
  for (int i = 0; i < throttleCount; i++) {
    if (strcmp(throttleTable[i].hash, hash) == 0) {
      if (now - throttleTable[i].lastSent < THROTTLE_MS) return false;
      throttleTable[i].lastSent = now;
      return true;
    }
  }
  if (throttleCount < THROTTLE_SLOTS) {
    strncpy(throttleTable[throttleCount].hash, hash, 9);
    throttleTable[throttleCount].lastSent = now;
    throttleCount++;
  }
  return true;
}

// ===================== MQTT =====================
static WiFiClientSecure tlsClient;
static PubSubClient     mqttClient(tlsClient);

static volatile uint8_t currentChannel = 1;

// ===================== MQTT CALLBACK =====================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (length > 255) return;
  char buf[256];
  memcpy(buf, payload, length);
  buf[length] = '\0';

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, buf)) return;

  const char* macHash = doc["macHash"];
  int8_t      rssi    = doc["rssi"];
  float       sx      = doc["x"];
  float       sy      = doc["y"];

  if (!macHash || rssi == 0 || rssi < MIN_RSSI) return;

  DeviceEntry* dev = findOrCreate(macHash);
  unsigned long now = millis();

  if (strcmp(topic, TOPIC_SLAVE_A) == 0) {
    dev->aRssi = rssi; dev->axPos = sx; dev->ayPos = sy; dev->aTime = now;
  } else if (strcmp(topic, TOPIC_SLAVE_B) == 0) {
    dev->bRssi = rssi; dev->bxPos = sx; dev->byPos = sy; dev->bTime = now;
  }

  float ex, ey;
  if (triangulate(dev, &ex, &ey)) {
    dev->estX = ex; dev->estY = ey; dev->published = false;
  }
}

// ===================== TIMESTAMP =====================
String getTimestamp() {
  struct tm t;
  char buf[30] = "unknown";
  if (getLocalTime(&t)) strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &t);
  return String(buf);
}

// ===================== WIFI =====================
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, WIFIPASSWORD);
  Serial.print("[WIFI] Forbinder");
  while (WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(500); }
  Serial.println("\n[WIFI] Forbundet: " + WiFi.localIP().toString());
}

// ===================== MQTT RECONNECT =====================
void reconnectMQTT() {
  int attempts = 0;
  while (!mqttClient.connected() && attempts < 5) {
    Serial.printf("[MQTT] Forbinder til %s:%d ...\n", MQTT_HOST, MQTT_PORT);
    String clientId = "Master-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("[MQTT] Forbundet!");
      mqttClient.subscribe(TOPIC_SLAVE_A);
      mqttClient.subscribe(TOPIC_SLAVE_B);
    } else {
      Serial.printf("[MQTT] Fejlede, rc=%d — prøver igen om 5 sek\n", mqttClient.state());
      attempts++;
      delay(5000);
    }
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("========================================");
  Serial.printf("[BOOT] Master starter @ (%.1f, %.1f)\n", MASTER_X, MASTER_Y);
  Serial.println("========================================");

  initWiFi();

  tlsClient.setCACert(MQTT_CA_CERT);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setBufferSize(512);
  mqttClient.setCallback(mqttCallback);
  reconnectMQTT();

  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  delay(1500);
  Serial.println("[NTP] Tid synkroniseret");

  updateDailySalt();
  Serial.printf("[MASTER] Salt: %s\n", dailySalt);

  // Start sniffer
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial.println("[SNIFFER] Kørende");
  Serial.println("========================================");
}

// ===================== LOOP =====================
static unsigned long lastSaltCheck = 0;
static int           lastDay       = -1;

void loop() {
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Forbindelse tabt — genopretter...");
    reconnectMQTT();
  }
  mqttClient.loop();

  // Opdater dagligt salt
  if (millis() - lastSaltCheck > 60000) {
    lastSaltCheck = millis();
    struct tm t;
    if (getLocalTime(&t) && t.tm_mday != lastDay) {
      lastDay = t.tm_mday;
      updateDailySalt();
      Serial.printf("[MASTER] Salt opdateret: %s\n", dailySalt);
    }
  }

  // Drain master sniffer queue
  while (!qEmpty()) {
    SniffEvent evt = sniffQueue[qHead];
    qHead = (qHead + 1) % QUEUE_SIZE;

    char macHash[9];
    macToHash(evt.mac, macHash);

    if (!shouldProcess(macHash)) continue;

    DeviceEntry* dev = findOrCreate(macHash);
    dev->mRssi = evt.rssi;
    dev->mxPos = MASTER_X;
    dev->myPos = MASTER_Y;
    dev->mTime = millis();

    float ex, ey;
    if (triangulate(dev, &ex, &ey)) {
      dev->estX = ex; dev->estY = ey; dev->published = false;
    }
  }

  // Publish klar-til-send entries
  for (int i = 0; i < deviceCount; i++) {
    if (!devices[i].published && mqttClient.connected()) {
      char payload[192];
      snprintf(payload, sizeof(payload),
        "{\"devId\":\"%s\",\"x\":%.2f,\"y\":%.2f,\"ts\":\"%s\"}",
        devices[i].devId, devices[i].estX, devices[i].estY,
        getTimestamp().c_str());
      mqttClient.publish(TOPIC_POSITIONS, payload);
      devices[i].published = true;
      Serial.printf("[MASTER] %s @ (%.2f, %.2f)\n",
        devices[i].devId, devices[i].estX, devices[i].estY);
    }
  }

  // Ryd enheder ikke set i 30 sek
  unsigned long now = millis();
  for (int i = 0; i < deviceCount; i++) {
    unsigned long newest = max({devices[i].aTime, devices[i].bTime, devices[i].mTime});
    if (newest > 0 && (now - newest) > 30000) {
      devices[i] = devices[deviceCount - 1];
      memset(&devices[deviceCount - 1], 0, sizeof(DeviceEntry));
      deviceCount--;
      i--;
    }
  }

  // Channel hopping — kanal 1, 6, 11
  static const uint8_t channels[] = {1, 6, 11};
  static uint8_t       channelIdx = 0;
  static unsigned long lastHop    = 0;

  if (millis() - lastHop > 150) {
    channelIdx     = (channelIdx + 1) % 3;
    currentChannel = channels[channelIdx];
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    lastHop = millis();
  }
}

```

## Heatmap men mangler websocket port til mqtt-server
test port 9001 eller  9883 
```
const client = mqtt.connect('wss://DIN_BROKER:9001', {
  username: 'DIN_USER',
  password: 'DIN_PASS'
});
client.on('connect', () => console.log('Forbundet!'));
client.on('error', (e) => console.log('Fejl:', e));
```

## Html-kode til heatmap
```
<!DOCTYPE html>
<html lang="da">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Positionsoverblik</title>
<script src="https://cdnjs.cloudflare.com/ajax/libs/mqtt/5.3.4/mqtt.min.js"></script>
<style>
  @import url('https://fonts.googleapis.com/css2?family=DM+Mono:wght@400;500&family=DM+Sans:wght@300;400;500&display=swap');

  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

  :root {
    --bg:       #0e1117;
    --surface:  #161b24;
    --border:   #242b38;
    --text:     #c8d0e0;
    --muted:    #4a5568;
    --accent:   #3b82f6;
    --online:   #22c55e;
    --warn:     #f59e0b;
    --danger:   #ef4444;

    /* Rum-dimensioner i meter — tilpas til jeres rum */
    --room-w-m: 10;
    --room-h-m: 8;
  }

  body {
    background: var(--bg);
    color: var(--text);
    font-family: 'DM Sans', sans-serif;
    font-size: 14px;
    min-height: 100vh;
    display: grid;
    grid-template-rows: 48px 1fr;
    grid-template-columns: 1fr 280px;
    grid-template-areas: "header header" "canvas sidebar";
  }

  /* ── HEADER ── */
  header {
    grid-area: header;
    background: var(--surface);
    border-bottom: 1px solid var(--border);
    display: flex;
    align-items: center;
    gap: 16px;
    padding: 0 20px;
  }

  header h1 {
    font-family: 'DM Mono', monospace;
    font-size: 13px;
    font-weight: 500;
    letter-spacing: .08em;
    text-transform: uppercase;
    color: var(--text);
  }

  #conn-badge {
    display: flex;
    align-items: center;
    gap: 6px;
    font-family: 'DM Mono', monospace;
    font-size: 11px;
    color: var(--muted);
  }

  #conn-dot {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--muted);
    transition: background .4s;
  }

  #conn-dot.online  { background: var(--online); box-shadow: 0 0 6px var(--online); }
  #conn-dot.error   { background: var(--danger);  box-shadow: 0 0 6px var(--danger); }

  #msg-count {
    margin-left: auto;
    font-family: 'DM Mono', monospace;
    font-size: 11px;
    color: var(--muted);
  }

  /* ── CANVAS AREA ── */
  #canvas-wrap {
    grid-area: canvas;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px;
    overflow: hidden;
  }

  #room-canvas {
    border: 1px solid var(--border);
    background: var(--surface);
    display: block;
    cursor: crosshair;
  }

  /* ── SIDEBAR ── */
  aside {
    grid-area: sidebar;
    background: var(--surface);
    border-left: 1px solid var(--border);
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }

  .sidebar-section {
    padding: 16px;
    border-bottom: 1px solid var(--border);
  }

  .sidebar-section h2 {
    font-family: 'DM Mono', monospace;
    font-size: 10px;
    font-weight: 500;
    letter-spacing: .12em;
    text-transform: uppercase;
    color: var(--muted);
    margin-bottom: 12px;
  }

  /* Connection config */
  .field { margin-bottom: 10px; }
  .field label {
    display: block;
    font-size: 11px;
    color: var(--muted);
    margin-bottom: 4px;
    font-family: 'DM Mono', monospace;
  }

  .field input {
    width: 100%;
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--text);
    font-family: 'DM Mono', monospace;
    font-size: 12px;
    padding: 6px 8px;
    outline: none;
    transition: border-color .2s;
  }
  .field input:focus { border-color: var(--accent); }

  #btn-connect {
    width: 100%;
    padding: 8px;
    background: var(--accent);
    color: #fff;
    border: none;
    border-radius: 4px;
    font-family: 'DM Mono', monospace;
    font-size: 12px;
    font-weight: 500;
    cursor: pointer;
    transition: opacity .2s;
    margin-top: 4px;
  }
  #btn-connect:hover { opacity: .85; }

  /* Device list */
  #device-list {
    flex: 1;
    overflow-y: auto;
    padding: 12px 16px;
    scrollbar-width: thin;
    scrollbar-color: var(--border) transparent;
  }

  .dev-item {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 8px 10px;
    border-radius: 6px;
    margin-bottom: 6px;
    background: var(--bg);
    border: 1px solid var(--border);
    transition: border-color .3s;
    animation: fadeIn .3s ease;
  }

  .dev-item.fresh { border-color: var(--accent); }

  @keyframes fadeIn { from { opacity: 0; transform: translateY(4px); } to { opacity: 1; transform: none; } }

  .dev-dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    flex-shrink: 0;
  }

  .dev-info { flex: 1; min-width: 0; }

  .dev-id {
    font-family: 'DM Mono', monospace;
    font-size: 12px;
    font-weight: 500;
    color: var(--text);
  }

  .dev-pos {
    font-family: 'DM Mono', monospace;
    font-size: 10px;
    color: var(--muted);
    margin-top: 2px;
  }

  .dev-age {
    font-family: 'DM Mono', monospace;
    font-size: 10px;
    color: var(--muted);
    flex-shrink: 0;
  }

  /* Slave positions legend */
  .slave-tag {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    font-family: 'DM Mono', monospace;
    font-size: 11px;
    color: var(--muted);
    margin-right: 10px;
  }

  .slave-icon {
    width: 8px; height: 8px;
    border-radius: 2px;
    background: #f59e0b;
    display: inline-block;
  }

  #btn-clear {
    background: none;
    border: 1px solid var(--border);
    color: var(--muted);
    border-radius: 4px;
    font-family: 'DM Mono', monospace;
    font-size: 11px;
    padding: 5px 10px;
    cursor: pointer;
    transition: border-color .2s, color .2s;
    width: 100%;
    margin-top: 4px;
  }
  #btn-clear:hover { border-color: var(--danger); color: var(--danger); }
</style>
</head>
<body>

<header>
  <h1>Positionsoverblik</h1>
  <div id="conn-badge">
    <div id="conn-dot"></div>
    <span id="conn-label">Ikke forbundet</span>
  </div>
  <span id="msg-count">0 beskeder</span>
</header>

<div id="canvas-wrap">
  <canvas id="room-canvas"></canvas>
</div>

<aside>
  <div class="sidebar-section">
    <h2>Broker</h2>
    <div class="field">
      <label>WebSocket URL</label>
      <!-- SKIFT PORT HER — typisk 9001 (Mosquitto) eller 8083 (EMQX) -->
      <input id="inp-url" type="text" value="wss://DIN_BROKER:9001">
    </div>
    <div class="field">
      <label>Brugernavn</label>
      <input id="inp-user" type="text" value="">
    </div>
    <div class="field">
      <label>Adgangskode</label>
      <input id="inp-pass" type="password" value="">
    </div>
    <div class="field">
      <label>Topic</label>
      <input id="inp-topic" type="text" value="devices/device03/positions">
    </div>
    <button id="btn-connect">Forbind</button>
  </div>

  <div class="sidebar-section">
    <h2>Rum (meter)</h2>
    <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px">
      <div class="field" style="margin:0">
        <label>Bredde</label>
        <input id="inp-rw" type="number" value="5" min="1" max="100">
      </div>
      <div class="field" style="margin:0">
        <label>Dybde</label>
        <input id="inp-rh" type="number" value="4" min="1" max="100">
      </div>
    </div>
  </div>

  <div class="sidebar-section">
    <h2>Enheder <span id="dev-count" style="color:var(--accent);font-size:12px"></span></h2>
    <div class="slave-tag"><span class="slave-icon" style="background:#f59e0b"></span> Slave</div>
    <div class="slave-tag"><span class="slave-icon" style="background:#a78bfa"></span> Master</div>
  </div>

  <div id="device-list"></div>

  <div class="sidebar-section">
    <button id="btn-clear">Ryd alle enheder</button>
  </div>
</aside>

<script>
// ── CONFIG ──────────────────────────────────────────────────
const FADE_MS      = 15000;  // Enhed forsvinder efter 15 sek uden opdatering
const HEATMAP_DECAY = 0.97;  // Heatmap falmer langsomt (pr. frame)
const DOT_RADIUS   = 14;     // Pixels

// Sensor-positioner i meter — skal matche config.h og MASTER_X/MASTER_Y
const SLAVES = [
  { id: 'slaveA',  x: 0.0, y: 0.0, color: '#f59e0b' },
  { id: 'slaveB',  x: 5.0, y: 0.0, color: '#f59e0b' },
  { id: 'master',  x: 0.0, y: 4.0, color: '#a78bfa' },
];

// ── STATE ────────────────────────────────────────────────────
let devices   = {};   // devId → { x, y, lastSeen, color }
let mqttClient = null;
let msgCount  = 0;

// Heatmap offscreen canvas
let heatCanvas, heatCtx;

// ── CANVAS SETUP ─────────────────────────────────────────────
const canvas = document.getElementById('room-canvas');
const ctx    = canvas.getContext('2d');

function roomDims() {
  return {
    wM: parseFloat(document.getElementById('inp-rw').value) || 10,
    hM: parseFloat(document.getElementById('inp-rh').value) || 8,
  };
}

function resizeCanvas() {
  const wrap = document.getElementById('canvas-wrap');
  const { wM, hM } = roomDims();
  const aspect = wM / hM;
  const maxW   = wrap.clientWidth  - 48;
  const maxH   = wrap.clientHeight - 48;
  let w = maxW, h = maxW / aspect;
  if (h > maxH) { h = maxH; w = h * aspect; }
  canvas.width  = Math.floor(w);
  canvas.height = Math.floor(h);

  heatCanvas        = document.createElement('canvas');
  heatCanvas.width  = canvas.width;
  heatCanvas.height = canvas.height;
  heatCtx           = heatCanvas.getContext('2d');
}

function mToPx(xM, yM) {
  const { wM, hM } = roomDims();
  return {
    px: (xM / wM) * canvas.width,
    py: (yM / hM) * canvas.height,
  };
}

// ── COLOR POOL ───────────────────────────────────────────────
const COLOR_POOL = [
  '#3b82f6','#8b5cf6','#ec4899','#06b6d4',
  '#10b981','#f97316','#ef4444','#a3e635',
];
let colorIdx = 0;
function nextColor() { return COLOR_POOL[colorIdx++ % COLOR_POOL.length]; }

// ── DRAW ─────────────────────────────────────────────────────
function draw() {
  const now = Date.now();
  ctx.clearRect(0, 0, canvas.width, canvas.height);

  // Heatmap fade
  heatCtx.globalAlpha = HEATMAP_DECAY;
  heatCtx.globalCompositeOperation = 'destination-out';
  heatCtx.fillRect(0, 0, heatCanvas.width, heatCanvas.height);
  heatCtx.globalCompositeOperation = 'source-over';
  heatCtx.globalAlpha = 1;

  // Draw heatmap layer
  ctx.drawImage(heatCanvas, 0, 0);

  // Grid
  ctx.strokeStyle = '#1e2530';
  ctx.lineWidth   = 1;
  const { wM, hM } = roomDims();
  for (let x = 0; x <= wM; x++) {
    const px = (x / wM) * canvas.width;
    ctx.beginPath(); ctx.moveTo(px, 0); ctx.lineTo(px, canvas.height); ctx.stroke();
  }
  for (let y = 0; y <= hM; y++) {
    const py = (y / hM) * canvas.height;
    ctx.beginPath(); ctx.moveTo(0, py); ctx.lineTo(canvas.width, py); ctx.stroke();
  }

  // Axis labels
  ctx.fillStyle = '#2a3444';
  ctx.font = '10px DM Mono, monospace';
  ctx.textAlign = 'center';
  for (let x = 0; x <= wM; x++) {
    const px = (x / wM) * canvas.width;
    ctx.fillText(x + 'm', px, canvas.height - 4);
  }
  ctx.textAlign = 'right';
  for (let y = 0; y <= hM; y++) {
    const py = (y / hM) * canvas.height;
    ctx.fillText(y + 'm', 22, py + 4);
  }

  // Slave positions
  SLAVES.forEach(s => {
    const { px, py } = mToPx(s.x, s.y);
    ctx.fillStyle = '#f59e0b';
    ctx.beginPath();
    ctx.rect(px - 6, py - 6, 12, 12);
    ctx.fill();
    ctx.fillStyle = '#0e1117';
    ctx.font = 'bold 8px DM Mono, monospace';
    ctx.textAlign = 'center';
    ctx.fillText('S', px, py + 3);
    ctx.fillStyle = '#f59e0b';
    ctx.font = '10px DM Mono, monospace';
    ctx.fillText(s.id, px, py - 10);
  });

  // Devices
  Object.values(devices).forEach(dev => {
    const age    = now - dev.lastSeen;
    if (age > FADE_MS) return;
    const alpha  = 1 - age / FADE_MS;
    const { px, py } = mToPx(dev.x, dev.y);

    // Heatmap blob
    const grad = heatCtx.createRadialGradient(px, py, 0, px, py, DOT_RADIUS * 3);
    grad.addColorStop(0, dev.color + '55');
    grad.addColorStop(1, dev.color + '00');
    heatCtx.fillStyle = grad;
    heatCtx.beginPath();
    heatCtx.arc(px, py, DOT_RADIUS * 3, 0, Math.PI * 2);
    heatCtx.fill();

    // Dot
    ctx.globalAlpha = alpha;
    ctx.fillStyle   = dev.color;
    ctx.beginPath();
    ctx.arc(px, py, DOT_RADIUS * 0.6, 0, Math.PI * 2);
    ctx.fill();

    // Puls-ring
    const pulse = ((now % 1600) / 1600);
    ctx.strokeStyle = dev.color;
    ctx.lineWidth   = 1.5;
    ctx.globalAlpha = alpha * (1 - pulse);
    ctx.beginPath();
    ctx.arc(px, py, DOT_RADIUS * 0.6 + pulse * DOT_RADIUS * 1.5, 0, Math.PI * 2);
    ctx.stroke();

    // Label
    ctx.globalAlpha = alpha;
    ctx.fillStyle   = dev.color;
    ctx.font        = '11px DM Mono, monospace';
    ctx.textAlign   = 'center';
    ctx.fillText(dev.id, px, py - DOT_RADIUS - 4);

    ctx.globalAlpha = 1;
  });

  requestAnimationFrame(draw);
}

// ── SIDEBAR DEVICE LIST ──────────────────────────────────────
function updateSidebar() {
  const now   = Date.now();
  const list  = document.getElementById('device-list');
  const count = document.getElementById('dev-count');
  const active = Object.values(devices).filter(d => now - d.lastSeen < FADE_MS);
  count.textContent = active.length ? `(${active.length})` : '';

  list.innerHTML = active.length === 0
    ? `<p style="color:var(--muted);font-size:12px;font-family:'DM Mono',monospace;padding:8px 0">Ingen enheder fundet endnu</p>`
    : active.sort((a,b) => b.lastSeen - a.lastSeen).map(dev => {
        const secs  = Math.round((now - dev.lastSeen) / 1000);
        const fresh = secs < 3;
        return `<div class="dev-item ${fresh ? 'fresh' : ''}">
          <div class="dev-dot" style="background:${dev.color}"></div>
          <div class="dev-info">
            <div class="dev-id">${dev.id}</div>
            <div class="dev-pos">(${dev.x.toFixed(2)}m, ${dev.y.toFixed(2)}m)</div>
          </div>
          <div class="dev-age">${secs}s</div>
        </div>`;
      }).join('');
}

setInterval(updateSidebar, 1000);

// ── MQTT ─────────────────────────────────────────────────────
function connect() {
  if (mqttClient) { try { mqttClient.end(true); } catch(e){} }

  const url   = document.getElementById('inp-url').value.trim();
  const user  = document.getElementById('inp-user').value.trim();
  const pass  = document.getElementById('inp-pass').value;
  const topic = document.getElementById('inp-topic').value.trim();

  setStatus('connecting', 'Forbinder...');

  const opts = { clientId: 'heatmap-' + Math.random().toString(16).slice(2) };
  if (user) { opts.username = user; opts.password = pass; }

  mqttClient = mqtt.connect(url, opts);

  mqttClient.on('connect', () => {
    setStatus('online', 'Forbundet');
    mqttClient.subscribe(topic, err => {
      if (err) console.error('[MQTT] Subscribe fejl:', err);
    });
  });

  mqttClient.on('error', err => {
    setStatus('error', 'Fejl: ' + (err.message || err));
  });

  mqttClient.on('close', () => setStatus('', 'Afbrudt'));

  mqttClient.on('message', (t, payload) => {
    msgCount++;
    document.getElementById('msg-count').textContent = msgCount + ' beskeder';

    try {
      const msg = JSON.parse(payload.toString());
      const { devId, x, y } = msg;
      if (!devId || x == null || y == null) return;

      if (!devices[devId]) {
        devices[devId] = { id: devId, color: nextColor() };
      }
      devices[devId].x        = parseFloat(x);
      devices[devId].y        = parseFloat(y);
      devices[devId].lastSeen = Date.now();
    } catch(e) {
      console.warn('[MQTT] Ugyldig JSON:', payload.toString());
    }
  });
}

function setStatus(state, label) {
  const dot = document.getElementById('conn-dot');
  const lbl = document.getElementById('conn-label');
  dot.className = state;
  lbl.textContent = label;
}

// ── EVENTS ───────────────────────────────────────────────────
document.getElementById('btn-connect').addEventListener('click', connect);

document.getElementById('btn-clear').addEventListener('click', () => {
  devices = {};
  heatCtx && heatCtx.clearRect(0, 0, heatCanvas.width, heatCanvas.height);
  updateSidebar();
});

['inp-rw','inp-rh'].forEach(id => {
  document.getElementById(id).addEventListener('change', () => { resizeCanvas(); });
});

window.addEventListener('resize', resizeCanvas);

// ── INIT ─────────────────────────────────────────────────────
resizeCanvas();
requestAnimationFrame(draw);
</script>
</body>
</html>


```


