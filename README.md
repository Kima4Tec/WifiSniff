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
