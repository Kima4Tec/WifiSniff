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
#define MQTT_TOPIC_PREFIX "/sensors/raw/"

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
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <mbedtls/md.h>
#include <ArduinoJson.h>
#include "secrets.h"   // SSID, WIFIPASSWORD, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS
#include "config.h"    // DEVICENAME
#include "ca_cert.h"   // MQTT_CA_CERT

// ===================== CONFIG =====================
#define TOPIC_SLAVE_A    "/sensors/raw/slaveA"
#define TOPIC_SLAVE_B    "/sensors/raw/slaveB"
#define TOPIC_POSITIONS  "/devices/positions"

// Tidsvindue for at kombinere målinger fra begge slaves (ms)
// Målinger der er ældre end dette kasseres
#define WINDOW_MS        2000

// Minimum RSSI for at tage en måling med (-85 dBm er typisk grænse for brugbar signal)
#define MIN_RSSI         -85

// Maks antal samtidige enheder vi tracker
#define MAX_DEVICES      40

// Dagligt salt til anonymisering — skiftes automatisk ved midnat
// Gør at samme MAC giver forskelligt devId fra dag til dag
static char dailySalt[16] = "SALT_INIT";

// ===================== ANONYMISERING =====================
// SHA256(macHash + dagligSalt) → 4 tegn brugt som "DEV-XXXX"
// macHash kommer allerede hashet fra slaven — dobbelthashning
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

void updateDailySalt() {
  struct tm t;
  if (getLocalTime(&t)) {
    snprintf(dailySalt, sizeof(dailySalt), "%04d%02d%02d",
      t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
  }
}

// ===================== DEVICE TABLE =====================
// Holder seneste måling fra hver slave per enhed
typedef struct {
  char          macHash[9];
  char          devId[10];

  // Slave A måling
  float         axPos;
  float         ayPos;
  int8_t        aRssi;
  unsigned long aTime;

  // Slave B måling
  float         bxPos;
  float         byPos;
  int8_t        bRssi;
  unsigned long bTime;

  // Beregnet position
  float         estX;
  float         estY;
  bool          published;
} DeviceEntry;

static DeviceEntry devices[MAX_DEVICES];
static int         deviceCount = 0;

DeviceEntry* findOrCreate(const char* macHash) {
  for (int i = 0; i < deviceCount; i++) {
    if (strcmp(devices[i].macHash, macHash) == 0) return &devices[i];
  }
  if (deviceCount >= MAX_DEVICES) {
    // Udskift den ældste entry (simpel LRU: find entry med ældst timestamp)
    int oldest = 0;
    unsigned long minTime = ULONG_MAX;
    for (int i = 0; i < MAX_DEVICES; i++) {
      unsigned long t = max(devices[i].aTime, devices[i].bTime);
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
// Weighted centroid baseret på RSSI
// Jo stærkere signal (højere RSSI), jo mere vægt
// Kræver minimum én måling — publicerer med én slave hvis kun én ser enheden

float rssiToDistance(int8_t rssi, int txPower = -59, float n = 2.5) {
  return pow(10.0, (txPower - rssi) / (10.0 * n));
}

bool triangulate(DeviceEntry* dev, float* outX, float* outY) {
  unsigned long now = millis();
  bool hasA = (dev->aTime > 0) && ((now - dev->aTime) < WINDOW_MS) && (dev->aRssi > MIN_RSSI);
  bool hasB = (dev->bTime > 0) && ((now - dev->bTime) < WINDOW_MS) && (dev->bRssi > MIN_RSSI);

  if (!hasA && !hasB) return false;

  if (hasA && hasB) {
    // Weighted centroid: vægt = 1 / distance²
    float dA = rssiToDistance(dev->aRssi);
    float dB = rssiToDistance(dev->bRssi);
    float wA = 1.0f / (dA * dA + 0.001f);
    float wB = 1.0f / (dB * dB + 0.001f);
    float total = wA + wB;
    *outX = (wA * dev->axPos + wB * dev->bxPos) / total;
    *outY = (wA * dev->ayPos + wB * dev->byPos) / total;
  } else if (hasA) {
    *outX = dev->axPos;
    *outY = dev->ayPos;
  } else {
    *outX = dev->bxPos;
    *outY = dev->byPos;
  }
  return true;
}

// ===================== MQTT =====================
static WiFiClientSecure tlsClient;
static PubSubClient     mqttClient(tlsClient);

// ===================== MQTT CALLBACK =====================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (length > 255) return;

  char buf[256];
  memcpy(buf, payload, length);
  buf[length] = '\0';

  // Parse JSON
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, buf);
  if (err) {
    Serial.printf("[MASTER] JSON fejl: %s\n", err.c_str());
    return;
  }

  const char* macHash = doc["macHash"];
  int8_t      rssi    = doc["rssi"];
  float       sx      = doc["x"];
  float       sy      = doc["y"];

  if (!macHash || rssi == 0) return;
  if (rssi < MIN_RSSI) return;

  DeviceEntry* dev = findOrCreate(macHash);
  unsigned long now = millis();

  if (strcmp(topic, TOPIC_SLAVE_A) == 0) {
    dev->aRssi = rssi;
    dev->axPos = sx;
    dev->ayPos = sy;
    dev->aTime = now;
  } else if (strcmp(topic, TOPIC_SLAVE_B) == 0) {
    dev->bRssi = rssi;
    dev->bxPos = sx;
    dev->byPos = sy;
    dev->bTime = now;
  }

  // Forsøg triangulering
  float ex, ey;
  if (triangulate(dev, &ex, &ey)) {
    dev->estX = ex;
    dev->estY = ey;
    dev->published = false;  // marker som klar til publish
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
    String clientId = "Master-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("[MQTT] Forbundet!");
      mqttClient.subscribe(TOPIC_SLAVE_A);
      mqttClient.subscribe(TOPIC_SLAVE_B);
      Serial.println("[MQTT] Abonnerer på slave topics");
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
  Serial.println("[BOOT] Master starter");
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
  Serial.printf("[MASTER] Dagligt salt sat: %s\n", dailySalt);
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

  // Opdater dagligt salt ved midnat
  if (millis() - lastSaltCheck > 60000) {
    lastSaltCheck = millis();
    struct tm t;
    if (getLocalTime(&t) && t.tm_mday != lastDay) {
      lastDay = t.tm_mday;
      updateDailySalt();
      Serial.printf("[MASTER] Salt opdateret: %s\n", dailySalt);
    }
  }

  // Publish klar-til-send entries
  for (int i = 0; i < deviceCount; i++) {
    if (!devices[i].published && mqttClient.connected()) {
      char payload[192];
      snprintf(payload, sizeof(payload),
        "{\"devId\":\"%s\",\"x\":%.2f,\"y\":%.2f,\"ts\":\"%s\"}",
        devices[i].devId,
        devices[i].estX,
        devices[i].estY,
        getTimestamp().c_str());

      mqttClient.publish(TOPIC_POSITIONS, payload);
      devices[i].published = true;

      Serial.printf("[MASTER] Publiceret: %s @ (%.2f, %.2f)\n",
        devices[i].devId, devices[i].estX, devices[i].estY);
    }
  }

  // Ryd gamle entries der ikke er set i mere end 30 sekunder
  unsigned long now = millis();
  for (int i = 0; i < deviceCount; i++) {
    unsigned long newest = max(devices[i].aTime, devices[i].bTime);
    if (newest > 0 && (now - newest) > 30000) {
      // Flyt sidste element ind på denne plads
      devices[i] = devices[deviceCount - 1];
      memset(&devices[deviceCount - 1], 0, sizeof(DeviceEntry));
      deviceCount--;
      i--;
    }
  }
}
```
