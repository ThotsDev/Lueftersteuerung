# Netzwerk und Matter

## Festlegung für den Prototyp

Für den aktuellen Windows-Build gilt: WLAN-Einrichtung über USB, anschließend Matter-Commissioning im bestehenden LAN. Die vorgebauten Arduino-Matter-Bibliotheken für den klassischen ESP32 bieten keine Bluetooth-Kopplung. Siehe [Windows-Anleitung](windows.md). BLE bleibt eine Möglichkeit der alternativen direkten ESP-IDF-Variante.

Matter über 2,4-GHz-WLAN auf dem bestätigten ESP32-D0WD-V3 mit 4 MiB Flash. Ein Thread Border Router ist für diesen WLAN-Pfad nicht erforderlich.

Die Windows-Firmware stellt WLAN mit den per USB eingegebenen und im ESP-NVS gespeicherten Zugangsdaten her. IPv6 wird aktiviert, die Wiederverbindung übernimmt Arduino-WiFi. Matter verwaltet mDNS/DNS-SD und die Kopplung. Keine WLAN-Passwörter im Quelltext, kein MQTT-Broker, keine Cloud-Abhängigkeit für die Lüfterbedienung. Im Prototyp kein eigener Webserver und kein Access Point.

## Einrichtung

1. Home Assistant mit Matter-Integration und laufendem Matter-Server vorbereiten. Unter HA OS wird die entsprechende App durch die Integration eingerichtet; Container benötigen einen passend erreichbaren Matter-Server.
2. ESP, Home Assistant und Smartphone zunächst im selben LAN/VLAN betreiben. Kein Gastnetz und keine WLAN-Client-Isolation.
3. Lokales IPv6 und Multicast/mDNS zulassen. IPv6-Internetzugang und DHCPv6-Server sind dafür nicht erforderlich. mDNS nutzt UDP 5353, Matter üblicherweise UDP 5540. Diese Ports allein sind keine vollständige Firewall-Freigabeliste.
4. Nach geprüftem Build und Flash `scripts/setup-wifi-windows.ps1` ausführen und WLAN lokal eingeben. Die Firmware gibt anschließend die Kopplungsdaten aus. Die SDK-Test-Attestierungsidentität ist nur für diesen Prototyp gedacht.
5. Im Matter-Controller ein neues Gerät im bestehenden LAN hinzufügen. Es findet beim Windows-Prototyp keine WLAN-Übertragung über Bluetooth statt.
6. Den ausgegebenen Setup-Code nutzen. WLAN-Zugangsdaten und individuelle Setup-Daten bleiben auf dem Gerät gespeichert.
7. Fan-Endpunkt prüfen: Aus und 33/66/100 Prozent. Zusätzlich Lüftungsstufe, Betriebsart und Standard-Zulufträume in den Matter-Auswahlfeldern prüfen. Jede Einstellung muss als simulierter Zustand im seriellen Log erscheinen.

WLAN-Zugangsdaten bitte nicht im Chat oder Repository ablegen. Zugang zum tatsächlichen Heimnetz und HA-Version sind hier noch nicht geprüft.

## Verhalten und offene Punkte

- Erstinbetriebnahme über USB-WLAN-Konfiguration und anschließende LAN-Kopplung; Eignung und RAM müssen auf dem erkannten ESP im Gerätetest bestätigt werden.
- Netzwerkrückkehr und Fabric-Persistenz übernimmt der Stack. Ein Neustart startet den simulierten Lüfter mit Aus.
- Kein automatisches Zurücksetzen bei WLAN-Ausfall oder NVS-Fehler.
- Der Windows-Adapter verlässt sich für die Fabric-Lebensdauer auf die Arduino-Matter-Bibliothek. Entfernung und erneute Kopplung müssen am Gerät geprüft werden. Für die endgültige Hardware ist eine lokale Reset-/Commissioning-Taste einzuplanen.
- Kopplungscodes in Logs sind vertraulich. Seriengeräte brauchen individuelle Inbetriebnahmedaten und geeignete Geräteidentitäten.
- OTA ist noch deaktiviert; vor festem Einbau muss ein überprüfter Update- und Wiederherstellungsweg existieren.

## Noch auszuführende Gerätetests

| Test | Erwartung |
| --- | --- |
| Erste Kopplung | Fan erscheint in Home Assistant |
| Alle vier Stufen und Aus | Vorgabe und Rückmeldung stimmen überein |
| ESP-Neustart | Fabric bleibt, WLAN verbindet sich wieder, Mock startet Aus |
| WLAN kurz aus | Keine Reset-Schleife; Rückkehr ohne erneute Kopplung |
| HA-Neustart | ESP bleibt gekoppelt und wird wieder erreichbar |
| Entfernen und erneut koppeln | Bewusster, nachvollziehbarer Commissioning-Ablauf |

[Home Assistant: Matter und Netzwerkanforderungen](https://www.home-assistant.io/integrations/matter/)
