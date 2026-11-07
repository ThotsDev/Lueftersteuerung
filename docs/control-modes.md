# Lüftungsstufen, Betriebsarten und Räume

Die Anlage hat vier Räume: Wohnzimmer, Schlafzimmer, Bad und Büro. Eine gemeinsame Stufe steuert alle vier Lüfter. Im Standard starten Wohnzimmer und Schlafzimmer mit Zuluft, Bad und Büro mit Abluft.

## Bezug zur VC40 H

Laut [Buderus-Anleitung, Abschnitt 6](https://www.heima24.de/shop/images/products/media/ia-ba-buderus-Bedieneinheit-VC40-H.pdf) bleibt die Richtung beim Durchlüften konstant; Eco kehrt sie stufenabhängig alle 50–70 Sekunden um. Zusätzlich besitzt das Original vier aktive Stufen, eine feuchteabhängige Automatik und eine zweistündige Einschlafpause.

Hier werden auf Nutzerwunsch **Aus plus drei aktive Stufen** und die Betriebsarten **Durchzug / Wechsellüftung** nachgebildet. Feuchteautomatik und Einschlafpause sind nicht Bestandteil dieser Änderung.

| Lüftungsstufe | Matter-Prozent | Wechselintervall im Prototyp |
| --- | --- | --- |
| Aus | 0 | kein Richtungswechsel |
| Leicht | 33 | 70 s |
| Mittel | 66 | 60 s |
| Stark | 100 | 50 s |

70/60/50 Sekunden sind bewusst gewählte Simulationswerte innerhalb des dokumentierten Bereichs. Die genaue Zuordnung der originalen vier Stufen ist nicht belegt. Die Prozentwerte bilden logische Stufen ab, weder gemessene Luftmengen noch PWM-Tastgrade. Andere Prozentvorgaben werden auf die nächsthöhere der drei Stufen gerundet.

## Durchzug und Wechsellüftung

- **Durchzug:** Alle Räume behalten ihre konfigurierte Standardrichtung.
- **Wechsellüftung:** Alle vier Richtungen werden gemeinsam umgekehrt. Aus zwei Zulufträumen werden zwei Ablufträume und umgekehrt. Die Standardzuordnung bleibt gespeichert und wird nicht bei jeder Phase umgeschrieben.
- **Aus:** Alle Lüfter stehen; die Wechselphase wird zurückgesetzt. Das nächste Einschalten startet mit der Standardrichtung.

Ein Wechsel der Betriebsart oder Standardzuordnung startet eine neue Phase mit der Standardrichtung. Ein Wechsel der aktiven Stufe erhält die momentane Richtung und startet deren Zeitintervall neu. Identische wiederholte Stufenbefehle verschieben den Timer nicht. Bei verzögertem Aufruf holt die lokale Logik die Phase anhand der verstrichenen Intervalle nach; sie führt keine schnellen Zwischenwechsel aus.

Die Zeitsteuerung läuft lokal ohne Home Assistant oder Netzwerkverbindung. Der `millis()`-Überlauf ist berücksichtigt. Standardzuordnung und Betriebsart werden im ESP-NVS gespeichert. Nach einem Neustart startet die Anlage mit **Aus**; Betriebsart, Standardzuordnung und Matter-Kopplung bleiben erhalten. Ohne gespeicherte Konfiguration ist Wechsellüftung voreingestellt.

## Einstellungen über Matter

Der bestehende Ventilator-Endpunkt bleibt erhalten. Er hat nun drei aktive Geschwindigkeitsstufen. Zusätzlich gibt es native Mode-Select-Endpunkte:

| Auswahlfeld | Optionen |
| --- | --- |
| Lüftungsstufe | Aus, Leicht, Mittel, Stark |
| Betriebsart | Durchzug, Wechsellüftung |
| Standard-Zuluft (andere Räume Abluft) | alle sechs Kombinationen aus zwei Räumen |

Die sechs Zuordnungen sind:

| Standard-Zuluft | Standard-Abluft |
| --- | --- |
| Wohnzimmer + Schlafzimmer | Bad + Büro |
| Wohnzimmer + Bad | Schlafzimmer + Büro |
| Wohnzimmer + Büro | Schlafzimmer + Bad |
| Schlafzimmer + Bad | Wohnzimmer + Büro |
| Schlafzimmer + Büro | Wohnzimmer + Bad |
| Bad + Büro | Wohnzimmer + Schlafzimmer |

Eine Auswahl ändert die komplette Zuordnung in einem Schritt. Es entstehen keine Zwischenzustände mit nur einem Zu- oder Abluftraum. Die Konfiguration garantiert die Anzahl zwei zu zwei; die reale Luftmengenabstimmung ist Teil der späteren Hardware-Inbetriebnahme.

Home Assistant unterstützt diese standardisierten Auswahlfelder. Sie können bei den Konfigurationseinstellungen des Geräts erscheinen. Nach einem Firmware-Update können neue Endpunkte eine erneute Abfrage der Geräteinformationen im Matter-Controller benötigen. Eine Neukopplung oder ein Factory Reset ist dafür nicht vorgesehen. Quelle: [Home Assistant ModeSelect](https://github.com/home-assistant/core/blob/dev/homeassistant/components/matter/select.py).

## USB-Diagnose

```text
status
stage Aus
stage Leicht
stage Mittel
stage Stark
mode durchzug
mode wechsel
supply wohnzimmer schlafzimmer
supply bad buero
```

`status` zeigt Stufe, Betriebsart, Standardmaske, Phase sowie aktuelle und konfigurierte Richtung aller vier Räume. Bei Aus steht `ACTIVE=0`; die angezeigte Richtung bedeutet dann keine laufende Luftförderung. Die USB-Befehle verwenden denselben Controller wie Matter und aktualisieren dessen Rückmeldungen.

## Hardwaregrenze und Tests

Weiterhin ausschließlich Simulation: keine GPIOs, keine PWM und keine elektrische Lüfteransteuerung. Der Hardwareadapter erhält später ein vollständiges Bild mit Stufe und allen vier Richtungen. Signalpegel, Richtungsprotokoll und erforderliche Umschaltpausen müssen vor der Leiterplatte geklärt werden.

Die Hosttests prüfen sämtliche Prozentvorgaben, alle gültigen und ungültigen Raumzuordnungen, beide Richtungsphasen, Aus/Wiedereinschalten, verspätete Timeraufrufe, Zeitgeberüberlauf und abgelehnte Ausgangsänderungen. Sie sind bestanden. Am ESP sind zusätzlich alle Stufen/Zuordnungen, zwei echte 50-Sekunden-Richtungswechsel, ungültige Eingaben und Persistenz samt bestehender Kopplung nach Neustart geprüft. Die drei Auswahlfelder sind laut Nutzer in Home Assistant sichtbar. Details und verbleibende Prüfungen stehen in der [Windows-Dokumentation](windows.md).
