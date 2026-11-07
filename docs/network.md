# Netzwerk und Matter

## Festlegung für den Prototyp

Matter über 2,4-GHz-WLAN. Der vorhandene ESP wird erst nach Chip-Erkennung endgültig freigegeben. Ein Thread Border Router ist für diesen WLAN-Pfad nicht erforderlich.

Das ESP-Matter-SDK besitzt WLAN, Wiederverbindung, IPv6, BLE-Inbetriebnahme und mDNS/DNS-SD. Die Anwendung startet keinen konkurrierenden WLAN-Manager. Keine WLAN-Passwörter im Quelltext, kein MQTT-Broker, keine Cloud-Abhängigkeit für die Lüfterbedienung. Im Prototyp kein eigener Webserver und kein Access Point.

## Einrichtung

1. Home Assistant mit Matter-Integration und laufendem Matter-Server vorbereiten. Unter HA OS wird die entsprechende App durch die Integration eingerichtet; Container benötigen einen passend erreichbaren Matter-Server.
2. ESP, Home Assistant und Smartphone zunächst im selben LAN/VLAN betreiben. Kein Gastnetz und keine WLAN-Client-Isolation.
3. Lokales IPv6 und Multicast/mDNS zulassen. IPv6-Internetzugang und DHCPv6-Server sind dafür nicht erforderlich. mDNS nutzt UDP 5353, Matter üblicherweise UDP 5540. Diese Ports allein sind keine vollständige Firewall-Freigabeliste.
4. Nach geprüftem Build und Flash den seriellen Monitor öffnen. Die Firmware gibt die Entwicklungs-Kopplungsdaten aus. Die SDK-Testidentität ist nur für diesen Prototyp gedacht.
5. In der Home-Assistant-Companion-App ein neues Matter-Gerät hinzufügen. Telefon-Bluetooth und erforderliche Berechtigungen aktivieren; ESP in Reichweite platzieren.
6. Den ausgegebenen Setup-Code nutzen. WLAN-Zugangsdaten werden im Commissioning übertragen und auf dem Gerät gespeichert.
7. Fan-Endpunkt prüfen: aus und 25/50/75/100 Prozent. Jede Einstellung muss als simulierter Stufenzustand im seriellen Log erscheinen.

WLAN-Zugangsdaten bitte nicht im Chat oder Repository ablegen. Zugang zum tatsächlichen Heimnetz und HA-Version sind hier noch nicht geprüft.

## Verhalten und offene Punkte

- Erstinbetriebnahme über BLE ist das konfigurierte Ziel; Eignung und RAM müssen auf dem erkannten ESP im Build-/Gerätetest bestätigt werden.
- Netzwerkrückkehr und Fabric-Persistenz übernimmt der Stack. Ein Neustart startet den simulierten Lüfter mit Aus.
- Kein automatisches Zurücksetzen bei WLAN-Ausfall oder NVS-Fehler.
- Nach Entfernen aus der letzten Fabric wird aktuell kein eigenes neues Kopplungsfenster geöffnet. Erneute Kopplung über die seriellen SDK-Kommandos bzw. einen bewusst ausgelösten Factory Reset. Für die endgültige Hardware ist eine lokale Reset-/Commissioning-Taste einzuplanen.
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
