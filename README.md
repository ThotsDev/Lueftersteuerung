# Lüftungssteuerung

ESP32-Projekt für einen späteren Ersatz der Buderus VC40 H mit lokaler Matter-Anbindung an Home Assistant.

**Stand: 07.11.2026:** ESP auf COM5 erfolgreich geflasht und mit Home Assistant gekoppelt: ESP32-D0WD-V3, Revision v3.1, 4 MiB Flash. Nativer Windows-Build: 1.557.411 Bytes Anwendung (49 %), 106.008 Bytes statischer RAM (32 %). Aus/Leicht/Mittel/Stark, Durchzug/Wechsellüftung, alle sechs 2-zu-2-Raumzuordnungen, zwei reale Timerphasen und Konfigurations-/Fabric-Persistenz am Gerät geprüft. Drei neue Auswahlfelder sind laut Nutzer in Home Assistant sichtbar. Ausschließlich simulierter Lüftertreiber, keinerlei GPIO-Zugriffe.

Bestätigt: vier Lüfter (Modelle unbekannt), Home Assistant als Container. Die separat installierte ESP-IDF v6.1 wird für den Arduino-Windows-Build nicht benötigt. Siehe [Bring-up](docs/bringup.md).

Der ursprüngliche 4-MiB-Flash ist unter `.tools/backups/esp32-original-20261107.bin` gesichert. Der primäre Entwicklungsweg ist jetzt der native **Windows-Build mit Arduino CLI und vorgebauten Matter-Bibliotheken**. Siehe [Windows-Anleitung](docs/windows.md).

## Einstieg

- [Lüftungsstufen, Betriebsarten und Raumrichtungen](docs/control-modes.md)
- [Windows: Setup, Build, Upload und WLAN](docs/windows.md)
- [Gerät und Entwicklungsumgebung](docs/bringup.md)
- [Architektur und nächste Schritte](docs/architecture.md)
- [Matter, WLAN und Home Assistant](docs/network.md)
- [Offene Hardwarefragen](docs/hardware.md)

Windows-Basis: Arduino CLI 1.5.1 und Arduino-ESP32 3.3.12. Die alternative Linux-Basis bleibt ESP-IDF 5.4.1 und ESP-Matter release/v1.4.2, auf den Commit in `sdk-versions.env` begrenzt.

## Ohne Leiterplatte

Der Matter-Endpunkt stellt eine gemeinsam gesteuerte Anlage dar: **Aus, Leicht, Mittel, Stark** (0/33/66/100 Prozent). Durchzug hält die Standardrichtungen; Wechsellüftung kehrt alle vier Räume periodisch um. Eine einstellbare Zuordnung hält jeweils zwei Räume auf Zuluft und zwei auf Abluft. Vorgabe: Wohnzimmer und Schlafzimmer Zuluft, Bad und Büro Abluft. Die Prozentwerte sind weder gemessene Luftmengen noch PWM-Tastgrade. Der Mock bestätigt nur einen simulierten Zustand.

Durchzug, Wechsellüftung und die sechs ausgeglichenen Raumzuordnungen sind implementiert. Einschlafpause und Feuchteautomatik bleiben offen. Die Bedienung nutzt den Ventilator sowie native Matter-Auswahlfelder; siehe [Betriebsarten](docs/control-modes.md).

## Lokale Prüfung

PowerShell mit installiertem C++17-Compiler:

```powershell
.\scripts\test-domain.ps1 -Compiler 'C:\StPerl532_x64\c\bin\g++.exe'
.\scripts\diagnose-esp.ps1
```

Firmware-Build und Inbetriebnahme siehe [Bring-up](docs/bringup.md).
