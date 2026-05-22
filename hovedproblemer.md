# A) Teknisk: Hvordan finder man position uden GPS?

Når man skal spore en enhed indendørs eller i områder uden GPS-dækning, må man forlade sig på de lokale radiotrådløse signaler. Til dette formål anvendes ofte ESP32-hardware.

---

## 1. RSSI (Wi-Fi Signalstyrke)
RSSI står for *Received Signal Strength Indicator*. Metoden måler den rå signalstyrke i dBm mellem en sender og en modtager.

* **Funktion:** Giver en groft estimeret afstand ud fra, hvor svagt eller kraftigt signalet modtages.
* **Udfordring:** Signalet er ustabilt. Det skifter drastisk alt efter:
    * Hvordan telefonen/enheden fysisk ligger eller er roteret.
    * Om der står mennesker, møbler eller betonvægge i vejen (vand i kroppen absorberer 2,4 GHz-bølger effektivt).

---

## 2. Triangulering (Flere målepunkter)
For at omsætte signalstyrke til en konkret placering, skal der bruges matematiske udregninger af ESP32'erens X- og Y-koordinater i et koordinatsystem.

* **Funktion:** Ved at placere mindst 3 faste målepunkter (ankre/beacons) med kendte koordinater, kan man bruge de individuelle RSSI-afstande til at beregne, hvor de tre cirkler overlapper (teknisk set kaldes dette *trilateration*).
* **Nøjagtighed:** Sikkerheden på en positionering er aldrig absolut. Den afhænger direkte af dine referencestationers præcision, de aktuelle signalforhold og afstanden til målet.
* **Præcision:** 
    * I åbne arealer med fri sigt (f.eks. GPS eller professionel landmåling) kan nøjagtigheden svinge fra få millimeter til 10-15 meter.
    * Indendørs med ESP32 og RSSI må man forvente en usikkerhed på **2 til 8 meter**.
* **Fejlkilder:** Vinkelfejl, signalrefleksioner (*flervejsudbredelse* hvor signalet bouncer på vægge og lofter) samt dårlig geometri (hvis anker-punkterne står på en lige linje) forstørrer usikkerheden, jo længere væk målet er fra de kendte punkter.
* **Praktisk anvendelse:** Usikkerheden visualiseres ofte som et **fejlpolygon** (et usikkerhedsområde) i et netværk af trekanter. Inden for moderne digital sporing og telekommunikation kombinerer man ofte vinkelmålinger med tid/afstandsmåling for at højne sikkerheden.
* **Avancerede alternativer:** For at opnå ekstrem sikkerhed udendørs bruges **RTK-GPS** (Real-Time Kinematic), der korrigerer for atmosfæriske forstyrrelser og giver centimeters nøjagtighed. Indendørs supplerer man i stedet med tætte Wi-Fi/Bluetooth-netværk eller Ultra-Wideband (UWB).

---

## 3. ESP-NOW vs. ESP-MESH
Når man opbygger sit sporingsnetværk med ESP32-enheder, adskiller kommunikationsprotokollerne **ESP-NOW** og **ESP-MESH** sig markant fra hinanden i opbygning, rækkevidde og præcision. Begge kører på det indbyggede 2,4 GHz Wi-Fi-hardware, men løser opgaven forskelligt.

### ESP-NOW
En hurtig, punkt-til-punkt-protokol udviklet af Espressif, som fungerer uden en traditionel Wi-Fi-router.
* **Direkte kommunikation:** Enhederne taler sammen parvis direkte med hinanden.
* **Lav latency:** Datapakker sendes lynhurtigt på få millisekunder.
* **Positionering:** Baseres primært på den rå RSSI-signalstyrke fra de modtagne pakker.
* **Præcision:** Lav til medium (1-5 meters usikkerhed under optimale forhold indendørs).
* **Sikkerhed/Stabilitet:** Sårbar over for fysiske blokeringer. Signalet påvirkes kraftigt af vægge, mennesker og refleksioner.

### ESP-MESH
Et netværk, hvor enhederne (nodes) arbejder sammen og videresender data for hinanden.
* **Selvhelende netværk:** Enhederne danner automatisk nye ruter til hinanden, hvis én node i netværket svigter eller slukkes.
* **Stor rækkevidde:** Data kan hoppe (*hoppe gennem mange led*) over meget store afstande og igennem hele bygninger.
* **Positionering:** Giver mulighed for at triangulere ud fra flere faste noder i netværket samtidigt, da data flyder på tværs af hele infrastrukturen.
* **Præcision:** Højere stabilitet end ESP-NOW, fordi du har adgang til flere målepunkter ad gangen.
* **Sikkerhed/Stabilitet:** Flere referencepunkter minimerer store udfald og sorterer ekstreme fejlmålinger fra.

### Sammenligning: Hvilken skal du vælge?


| Funktion | ESP-NOW | ESP-MESH |
| :--- | :--- | :--- |
| **Batterilevetid** | Meget høj (enheden kan sove i deep-sleep ofte) | Lav/Medium (enheder skal lytte konstant for at holde netværket i live) |
| **Skalerbarhed** | Begrænset (kræver direkte kontakt til modtager) | Enorm (kan dække store komplekse bygninger) |
| **Opsætning** | Meget simpel og hurtig at kode | Kompleks og tung kodestruktur |
| **Båndbredde** | Lille (kun til korte, hurtige datapakker) | Høj (hele netværket deler og distribuerer data) |

---

## 4. Round-trip time (RTT / Ping / Latency)
Round-trip time er den tid, det tager en datapakke at rejse fra en sender (Enhed A), blive modtaget og besvaret af en modtager (Enhed B), og nå tilbage til senderen (Enhed A) igen.

