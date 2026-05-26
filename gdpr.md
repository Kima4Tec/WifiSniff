https://pentests.dk/docs/gdpr-developers-guide/

Vi opsamler MAC-adresser fra personer der ikke har givet samtykke
Vi logger position + tid, hvilket er lokaliseringsdata
Vi transmitterer og gemmer data på en MQTT-broker
Det er muligt at rekonstruere en persons bevægelser over tid

De vigtigste GDPR-krav
Retsgrundlag — I skal have et gyldigt grundlag for behandlingen. For et skoleprojekt er det typisk legitim interesse (artikel 6(1)(f)), men det kræver at I kan argumentere for at interessen ikke overstiger de registreredes rettigheder.
Dataminimering — I må kun indsamle det I faktisk har brug for. Overvej:

Behøver vi gemme den fulde MAC, eller er et anonymt hash nok?
Behøver vi gemme præcise tidsstempler?

Opbevaringsbegrænsning — Data må ikke gemmes længere end nødvendigt. Definer en konkret slettefrist.
Information til de registrerede — Personer hvis telefoner I scanner, skal i princippet informeres. I et kontrolleret forsøg (f.eks. kun kendte telefoner tilhørende jer selv) er det håndterbart.
Hvad der gør jeres projekt mere acceptabelt

Vi scanner kun kendte, specifikke MAC-adresser — ikke alle i nærheden
Telefonerne tilhører sandsynligvis jer selv eller folk der har givet samtykke
Det er et afgrænset eksperiment, ikke kontinuerlig overvågning

Praktiske anbefalinger

Hash MAC-adresserne inden I gemmer dem — SHA256(MAC) er ikke reversibelt og reducerer risikoen markant
Indhent eksplicit samtykke fra ejerne af de telefoner I tracker
Begræns adgang til MQTT-brokeren og den data der gemmes
Slet data efter projektet er afsluttet
Dokumentér hvad vi indsamler, hvorfor, og hvem der har adgang — selv en enkel side er nok til et skoleprojekt

Kort sagt: fordi vi kun tracker kendte telefoner med ejernes samtykke, er vi i en relativt god position — men vi bør kunne dokumentere det samtykke og have en plan for datasletning.
