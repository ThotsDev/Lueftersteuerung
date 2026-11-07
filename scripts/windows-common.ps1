# Shared, project-local Arduino configuration. No global Arduino settings change.
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$ArduinoCli = Join-Path $ProjectRoot '.tools\arduino-cli\arduino-cli.exe'
$ArduinoCoreVersion = '3.3.12'
$ArduinoIndexUrl = 'https://espressif.github.io/arduino-esp32/package_esp32_index.json'
$ArduinoBoard = 'esp32:esp32:esp32:PartitionScheme=huge_app,FlashSize=4M,CPUFreq=240,DebugLevel=none'
$env:ARDUINO_DIRECTORIES_DATA = Join-Path $ProjectRoot '.tools\arduino\data'
$env:ARDUINO_DIRECTORIES_DOWNLOADS = Join-Path $ProjectRoot '.tools\arduino\downloads'
$env:ARDUINO_DIRECTORIES_USER = Join-Path $ProjectRoot '.tools\arduino\user'
# Arduino CLI does not automatically use the Windows system proxy.
if (-not $env:ARDUINO_NETWORK_PROXY) {
    $indexAddress = [uri]$ArduinoIndexUrl
    $systemProxy = [System.Net.WebRequest]::GetSystemWebProxy().GetProxy($indexAddress)
    if ($systemProxy -and $systemProxy.AbsoluteUri -ne $indexAddress.AbsoluteUri) {
        $env:ARDUINO_NETWORK_PROXY = $systemProxy.AbsoluteUri
    }
}
$WindowsSketch = Join-Path $ProjectRoot 'firmware\windows\ventilation_controller'
$WindowsBuild = Join-Path $ProjectRoot 'build\windows'
