# Gerät und Entwicklungsumgebung

## Beobachtet am 07.11.2026

- Workspace war leer; keine übergeordneten AGENTS.md gefunden.
- Windows erkennt inzwischen `Silicon Labs CP210x USB to UART Bridge (COM5)`.
- USB-ID: `VID_10C4&PID_EA60`, Geräteinstanz endet auf `0001`.
- Der anfängliche Treiberfehler (Code 28) ist behoben; serieller Zugriff auf COM5 funktioniert.
- Mit esptool bestätigt: **ESP32-D0WD-V3, Revision v3.1, 240 MHz, 40-MHz-Quarz, 4 MiB Flash**, WLAN und Bluetooth. IDF-Target: `esp32`. Vorhandene Firmware noch unbekannt.
- Git 2.55.0 und Python 3.12.10 vorhanden, über absolute Pfade aufrufbar.
- C++-Hostcompiler: Strawberry Perl GCC 8.3.0.
- ESP-IDF bei der ersten Prüfung nicht gefunden; inzwischen ist **ESP-IDF v6.1** unter `C:\esp\v6.1\esp-idf` installiert. Werkzeuge liegen unter `C:\Espressif\tools`, die Python-Umgebung unter `C:\Espressif\tools\python\v6.1\venv`. ESP-Matter ist weiterhin nicht eingerichtet. esptool 5.6.0 und pyserial 3.5 sind isoliert unter `.tools/python` installiert.
- WSL-Programm vorhanden, meldet jedoch, dass WSL nicht installiert ist.
- Der automatische Silicon-Labs-Treiberdownload scheiterte ursprünglich mit HTTP 403; der Treiber ist inzwischen vorhanden.

## Seriellen Zugriff herstellen

**Erledigt:** `chip-id` und `flash-id` auf COM5 wurden erfolgreich ausgeführt. Dabei wurde kein Flash überschrieben; das Board wurde zurückgesetzt. Die folgenden Installationsschritte dienen nur noch als Referenz. Eine vollständige Sicherung liegt inzwischen vor, siehe unten.

Den [CP210x-VCP-Treiber von Silicon Labs](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers) über den Browser beziehen oder den passenden Treiber über Windows Update installieren. Bei einem verwalteten Rechner kann die Installation Administratorrechte erfordern.

Danach ESP ab- und anstecken und `scripts/diagnose-esp.ps1` ausführen. Erwartet: fehlerfreies Gerät unter Anschlüsse und ein COM-Port.

esptool isoliert einrichten:

```powershell
& 'C:\Python312\python.exe' -m venv .tools\python
& '.\.tools\python\Scripts\python.exe' -m pip install esptool
# COM5 durch den tatsächlich gefundenen Port ersetzen:
& '.\.tools\python\Scripts\python.exe' -m esptool --port COM5 chip-id
& '.\.tools\python\Scripts\python.exe' -m esptool --port COM5 flash-id
```

ROM-Abfragen können das Board zurücksetzen, überschreiben jedoch keinen Flash. Falls keine Verbindung zustande kommt: BOOT gedrückt halten, EN kurz drücken, BOOT nach Beginn der Verbindung lösen.

Die vollständigen 4 MiB wurden am 07.11.2026 erfolgreich gesichert:

- Datei: `.tools/backups/esp32-original-20261107.bin`
- Länge: 4.194.304 Bytes
- SHA-256: `5B58274DC6721ED4BDDB383F6E6109848ED1C0DF612249FD82830C517E19F416`
- Der Hash bestätigt die lokale Datei; ein zweiter Vergleich mit dem Geräteinhalt wurde noch nicht durchgeführt.
- Die Datei bleibt durch `.gitignore` vom Repository ausgeschlossen.

Der verwendete Aufruf (nicht erneut unter demselben Dateinamen ausführen):

```powershell
& '.\.tools\python\Scripts\python.exe' -m esptool --port COM5 --baud 460800 read-flash 0 0x400000 .tools\backups\esp32-original-20261107.bin
```

## SDK

**Aktueller Entwicklungsweg:** [Native Windows-Anleitung](windows.md), ohne WSL oder Cloud. Die folgenden Angaben betreffen die alternative direkte ESP-IDF-/ESP-Matter-Umgebung.

Die installierte Windows-Version v6.1 entspricht noch nicht der festgehaltenen Matter-Basis (ESP-IDF v5.4.1). Sie wird deshalb nicht automatisch zum Bauen dieses Matter-Projekts verwendet. Die Installation bleibt nutzbar; entweder muss die bestehende Basis separat eingerichtet oder die Kombination mit einem neueren ESP-Matter gezielt geprüft und umgestellt werden. Eine installierte ESP-IDF-Version allein ersetzt ESP-Matter und dessen Buildvoraussetzungen nicht.

Die vom Installer erzeugte Aktivierung liegt unter `C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1`. Für reine Diagnose funktioniert auch der direkte Aufruf mit den passenden Umgebungsvariablen:

```powershell
$env:PYTHONUTF8 = '1'
$env:IDF_PATH = 'C:\esp\v6.1\esp-idf'
$env:IDF_TOOLS_PATH = 'C:\Espressif\tools'
$env:IDF_PYTHON_ENV_PATH = 'C:\Espressif\tools\python\v6.1\venv'
$env:ESP_IDF_VERSION = '6.1'
$env:IDF_VERSION = '6.1.0'
& "$env:IDF_PYTHON_ENV_PATH\Scripts\python.exe" "$env:IDF_PATH\tools\idf.py" --version
```

Espressif unterstützt die Matter-Entwicklung unter Windows über WSL. WSL2/Ubuntu sowie gegebenenfalls usbipd-win fehlen noch. Diese Betriebssysteminstallation wird nicht durch das Projektsetup ausgelöst und kann einen Neustart benötigen.

Die angefragte WSL-/Ubuntu-Installation wurde nicht freigegeben und deshalb nicht ausgeführt. Der Firmware-Build ist damit weiterhin offen. Alternativ können dieselben Projekt-Skripte auf einem vorhandenen Linux-Rechner laufen; der ESP bleibt zum Sichern und Flashen an Windows angeschlossen. Die installierte Windows-ESP-IDF-Umgebung bleibt unverändert.

SDKs und Buildkopie im Linux-Dateisystem ablegen, nicht direkt im OneDrive-Verzeichnis bauen. Nach WSL-/Ubuntu-Installation:

```bash
sudo apt-get update
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv \
  cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
# Projekt in einen Ordner unter ~/projects/ kopieren; danach:
bash scripts/setup-sdk.sh
bash scripts/build-firmware.sh
```

Bei anderem Chip das passende Target wählen. Das vorläufige Profil erfordert mindestens 4 MiB Flash; Größe und erzeugte Firmware vor dem Flashen prüfen. SDK-Konfigurationswarnungen müssen beim ersten Build ausgewertet werden.

USB-Zugriff aus WSL benötigt usbipd-win. Alternativ Build-Artefakte zurück nach Windows kopieren und dort das vom Build erzeugte Flash-Kommando mit lokalem esptool ausführen. Keine geratenen Offsets verwenden.

## Validierungsstand

Hosttests erfolgreich: alle 101 gültigen Prozentwerte, ungültige Werte, Aus und abgelehnte Ausgangsbefehle. PowerShell- und Bash-Syntax geprüft. Die Hosttests prüfen ausschließlich die portable Stufenlogik. Matter-Adapter, SDK-Konfiguration, BLE/WLAN-Inbetriebnahme und Home-Assistant-Kompatibilität erfordern noch einen echten SDK-Build und Gerätetest.

Inzwischen zusätzlich bestätigt: Der Arduino-Windows-Build mit ESP32-Core 3.3.12 ist erfolgreich. Aktuelle Anwendung: 1.557.411 Bytes, statischer RAM: 106.008 Bytes. Firmware auf COM5 geflasht und von esptool verifiziert. USB-WLAN-Konfiguration und Matter-Kopplung funktionieren. Die neuen Stufen, Betriebsarten, sechs Raumzuordnungen und zwei echte Richtungswechsel sind am Gerät geprüft; nach Reset bleiben Konfiguration und bestehende Matter-Fabric erhalten, die Stufe startet Aus. Der ausführliche aktuelle Teststand steht in [Windows](windows.md). Die direkte ESP-IDF-Variante bleibt ungeprüft.

Vor dem Betriebsarten-Update wurde zusätzlich der gekoppelte Flash unter `.tools/backups/esp32-before-modes-20261107.bin` gesichert (4.194.304 Bytes, SHA-256 `DFF8176AB3B042CB3532398237F629A20255BF661D1F69EBE64B8C0E69A7E0D0`). Diese Sicherung enthält Geräte- und Kopplungsdaten und bleibt lokal.

[Espressif: Entwicklung und Windows/WSL](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html)
