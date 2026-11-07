# Native Windows-Entwicklung

Dieser Build verwendet Arduino CLI 1.5.1 und Arduino-ESP32 3.3.12 mit vorgebautem Matter-Stack. WSL, Docker und ein Cloud-Build sind dafür nicht nötig. Die bereits installierte ESP-IDF-Version wird nicht verändert und nicht für diesen Build verwendet.

Zielgerät: ESP32-D0WD-V3, 4 MiB Flash, COM5. Die Firmware simuliert vier Lüftungsstufen ohne GPIO-Ausgabe. Der portable Controller wird aus `components/ventilation` übernommen. Der Arduino-Einstieg liegt unter `firmware/windows/ventilation_controller`.

Die Matter-Geräteangaben stehen zentral in [`device_identity.hpp`](../firmware/windows/ventilation_controller/device_identity.hpp): Herstellername, Produktname, Gerätebezeichnung, optionale Seriennummer und Hardwareversion samt Beschreibung. Leere optionale Texte beziehungsweise `std::nullopt` behalten die SDK-Vorgaben bei. Die dort ebenfalls aufgeführten Vendor-/Product-IDs stammen aus dem vorgebauten SDK und sind keine frei änderbaren Laufzeiteinstellungen. Nach Änderungen neu bauen und hochladen; die Firmware wendet die Angaben vor `Matter.begin()` an. Die individuellen Kopplungsdaten bleiben unabhängig davon im ESP-NVS.

## Einrichtung und Build

PowerShell 7 im Projektverzeichnis:

```powershell
.\scripts\setup-windows.ps1
.\scripts\build-windows.ps1
```

Werkzeuge, Paketindex und Compiler werden ausschließlich unter `.tools/` installiert. Das Setup lädt Pakete über die Windows-Systemproxy-Einstellung, falls erforderlich. Arduino CLI wird anhand der festgehaltenen SHA-256-Prüfsumme kontrolliert; der Paketmanager prüft die Paketarchive. Die Installation benötigt mehrere GB Speicherplatz.

Alternativ in VS Code: **Terminal > Buildtask ausführen** (`Strg+Umschalt+B`). Die weiteren Aufgaben stehen unter **Terminal > Task ausführen** als `Windows: ...` bereit.

Die Boardkonfiguration verwendet `esp32:esp32:esp32`, 4 MiB Flash und `huge_app` (3-MiB-Anwendung, ohne OTA). Ergebnisse liegen unter `build/windows/`. Der bestehende ESP-IDF-Einstieg unter `main/`, `sdkconfig.defaults` und die Root-CMake-Datei gehören zum alternativen Linux-Build und konfigurieren diesen Arduino-Build nicht.

## Flashen

Vor dem Upload alle seriellen Monitore schließen. Die originale 4-MiB-Sicherung befindet sich unter `.tools/backups/esp32-original-20261107.bin`. Das Upload-Skript verlangt eine Sicherungsdatei passender Größe.

```powershell
.\scripts\upload-windows.ps1 -Port COM5
```

Beim Wechsel von einer inkompatiblen Firmware kann ein bewusstes Löschen des gesamten Flashs nötig sein. Das löscht auch WLAN- und Matter-Daten:

```powershell
.\scripts\upload-windows.ps1 -Port COM5 -EraseAll
```

## WLAN und Matter

Beim klassischen ESP32 enthält der Arduino-Matter-Stack keine Bluetooth-Kopplung. Der ESP bekommt deshalb zuerst über USB die Daten des 2,4-GHz-WLANs. Anschließend erfolgt Matter-Commissioning über das LAN.

```powershell
.\scripts\setup-wifi-windows.ps1 -Port COM5
```

Das Skript fragt SSID und verdeckt das Passwort ab. Es überträgt beides nur über den seriellen Anschluss. Es hinterlegt keine Zugangsdaten im Quelltext oder in Kommandozeilenargumenten und erzeugt im interaktiven Modus keine Zugangsdaten-Datei. Die Firmware speichert WLAN-Daten in NVS auf dem ESP; dieser Prototyp verwendet keine Flash-Verschlüsselung. Unterstützt werden Passphrasen mit 8–63 UTF-8-Bytes, keine offenen Netze oder rohen 64-stelligen PSKs.

Optional kann eine bereits vorhandene lokale Datei verwendet werden:

```powershell
.\scripts\setup-wifi-windows.ps1 -Port COM5 -EnvFile .\wlan.env
```

Erwartet werden die Schlüssel `SSID` und `PASSWORD`, jeweils durch `=` oder `:` vom Wert getrennt. Zusammengehörige einfache oder doppelte Anführungszeichen werden entfernt; Inhalte werden niemals als Code ausgewertet. Der interaktive Modus erstellt keine Zugangsdaten-Datei. `wlan.env` ist durch `.gitignore` ausgeschlossen; bei einem anderen Dateinamen muss derselbe Schutz sichergestellt werden. Dateiinhalte werden nicht ausgegeben.

Nach dem Neustart und der WLAN-Verbindung gibt die Firmware aus:

```text
MATTER_MANUAL_CODE=...
MATTER_QR_URL=...
```

Den tatsächlichen Code erst aus der laufenden Firmware übernehmen. Pro Gerät werden beim ersten Matter-Start ein zufälliger gültiger Setup-Passcode und ein Discriminator erzeugt und über Neustarts beibehalten. Es handelt sich weiterhin um eine Entwicklung mit Test-Attestierungsidentität, nicht um ein zertifiziertes Produkt. Kopplungsdaten nicht öffentlich in das Repository laden.

Für Status oder erneute Codeausgabe:

```powershell
.\scripts\monitor-windows.ps1 -Port COM5
```

Serielle Befehle: `status`, `pairing`, `wifi-setup` (jeweils mit Zeilenumbruch). Bei bereits gekoppeltem Gerät verweist `pairing` auf den bestehenden Matter-Controller zum Öffnen eines neuen Kopplungsfensters. Das bloße Ändern des WLANs entfernt keine Matter-Fabric.

Im Matter-Controller werden Aus und 33/66/100 Prozent auf Aus/Leicht/Mittel/Stark abgebildet. Benannte Modi Low/Medium/High entsprechen den Stufen 1/2/3. Die Betriebsart und die Standard-Zulufträume sind über zusätzliche Matter-Auswahlfelder einstellbar. Durchzug und Wechsellüftung sind simuliert implementiert; Feuchteautomatik und Hardwaretreiber bleiben offen. Der Mock startet nach jedem Neustart mit Aus, Kopplung und Konfiguration bleiben gespeichert. Siehe [Betriebsarten](control-modes.md).

## Validierungsstand

**Buildprüfung am 07.11.2026 bestanden:** Arduino CLI 1.5.1, ESP32-Core 3.3.12, Anwendung nach Auslagerung der Geräteangaben 1.557.503 / 3.145.728 Bytes (49 %), statischer RAM 106.008 / 327.680 Bytes (32 %). Die 221.672 verbleibenden Bytes sind vor den dynamischen Laufzeitallokationen angegeben, keine Messung des freien Heaps nach dem Matter-Start. PowerShell-Syntax und VS-Code-Task-JSON geprüft.

Nach der Auslagerung in `device_identity.hpp` wurde erneut geflasht und ein Starttest durchgeführt: Matter-Endpunkte bereit, WLAN verbunden, bestehende Fabric erhalten, genau ein Boot ohne Identitäts-/Laufzeitfehler und Stufe Aus. Die vorhandenen Geräteangaben wurden bei dieser Umstrukturierung nicht geändert.

**Gerätetest am 07.11.2026 bestanden:** Upload auf COM5 einschließlich esptool-Hashprüfung aller geschriebenen Images. Boot ohne Panic, USB-Statusabfrage, Ablehnung überlanger Eingaben, WLAN-Einrichtung aus lokaler Datei und Matter-Start. Leere SSID wird abgewiesen, eine abgebrochene Einrichtung verändert die bestehende Verbindung nicht. Die erste Matter-Kopplung war laut Nutzer erfolgreich.

**Betriebsarten-Update geprüft:** Alle vier Stufen, beide Betriebsarten und alle sechs Raumzuordnungen über die USB-Diagnose am echten ESP angewendet. Zwei echte 50-Sekunden-Phasen beobachtet: zunächst alle vier Richtungen umgekehrt, dann die Standardrichtung wiederhergestellt; stets zwei Zu- und zwei Ablufträume. Ungültige Eingaben wurden ohne Konfigurationsänderung abgewiesen. Nach Hardware-Reset bleiben Betriebsart, Raumzuordnung und bestehende Matter-Fabric erhalten, die Stufe startet mit Aus. Der Nutzer bestätigt die Sichtbarkeit aller drei neuen Matter-Auswahlfelder.

Gerätetests wiederholen (nur bei simulierten Ausgängen, verändert während des Tests die Stufen und Konfiguration, stellt am Ende die ursprüngliche Zuordnung/Betriebsart sowie Aus wieder her):

```powershell
& '.\.tools\python\Scripts\python.exe' .\scripts\test-modes-device.py --port COM5 --timing
```

Noch offen: Bedienung und Rückmeldung der neuen Auswahlfelder durch den Controller vollständig gegenprüfen, WLAN-Ausfall während des Betriebs und ein längerer Laufzeittest. Die Zeitintervalle 70/60/50 Sekunden sind Simulationswerte, keine verifizierte originale VC40H-Kennlinie.

Geräteprotokolle und tatsächliche Kopplungsdaten liegen ausschließlich lokal unter `.tools/`; sie werden nicht in das Repository übernommen.

Quellen: [Arduino-ESP32/Matter](https://docs.espressif.com/projects/arduino-esp32/en/latest/matter/matter.html), [Arduino CLI](https://arduino.github.io/arduino-cli/1.5/).
