# Lüftungssteuerung

ESP32-Projekt für einen späteren Ersatz der Buderus VC40 H mit lokaler Matter-Anbindung an Home Assistant.

**Stand: 07.11.2026:** ESP auf COM5 erfolgreich ausgelesen: ESP32-D0WD-V3, Revision v3.1, 4 MiB Flash. Firmware-Grundgerüst angelegt; noch nicht mit ESP-IDF gebaut, geflasht oder mit Home Assistant gekoppelt. Ausschließlich simulierter Lüftertreiber, keinerlei GPIO-Zugriffe.

Bestätigt: vier Lüfter (Modelle unbekannt), Home Assistant als Container. ESP-IDF v6.1 wurde inzwischen installiert; die Kompatibilität mit der unten festgehaltenen Matter-Basis ist noch zu klären. Siehe [Bring-up](docs/bringup.md).

Der ursprüngliche 4-MiB-Flash ist unter `.tools/backups/esp32-original-20261107.bin` gesichert. Build und Flash der Testfirmware stehen aus: WSL/Ubuntu wurde nicht installiert, weil die Installationsanfrage nicht freigegeben wurde.

## Einstieg

- [Gerät und Entwicklungsumgebung](docs/bringup.md)
- [Architektur und nächste Schritte](docs/architecture.md)
- [Matter, WLAN und Home Assistant](docs/network.md)
- [Offene Hardwarefragen](docs/hardware.md)

Basis: ESP-IDF 5.4.1 und ESP-Matter release/v1.4.2, auf den in `sdk-versions.env` festgehaltenen Commit begrenzt. Diese dokumentierte SDK-Kombination ist eine Startbasis; vor dem realen Betrieb sind Wartungsstand und Updates erneut zu prüfen.

## Ohne Leiterplatte

Der Matter-Endpunkt stellt einen Ventilator dar. Prozentvorgaben werden auf vier **logische Stufen** abgebildet: 0 = aus, 1–25 = 1, 26–50 = 2, 51–75 = 3, 76–100 = 4. Die Prozentwerte sind weder gemessene Luftmengen noch PWM-Tastgrade. Der Mock bestätigt nur einen simulierten Zustand.

Eco, Durchlüften, Einschlafen und Feuchteautomatik sind Architekturziele und noch nicht implementiert oder als Matter-Funktionen beworben.

## Lokale Prüfung

PowerShell mit installiertem C++17-Compiler:

```powershell
.\scripts\test-domain.ps1 -Compiler 'C:\StPerl532_x64\c\bin\g++.exe'
.\scripts\diagnose-esp.ps1
```

Firmware-Build und Inbetriebnahme siehe [Bring-up](docs/bringup.md).
