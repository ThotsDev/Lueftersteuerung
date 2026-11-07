#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'windows-common.ps1')
if (-not (Test-Path -LiteralPath $ArduinoCli)) { throw 'Run scripts/setup-windows.ps1 first.' }
$cores = & $ArduinoCli core list --format json | ConvertFrom-Json
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect installed cores.' }
$esp32 = @($cores.platforms | Where-Object { $_.id -eq 'esp32:esp32' })
if ($esp32.Count -ne 1 -or $esp32[0].installed_version -ne $ArduinoCoreVersion) {
    throw "Run scripts/setup-windows.ps1; ESP32 core $ArduinoCoreVersion is required."
}
& $ArduinoCli compile --fqbn $ArduinoBoard --library (Join-Path $ProjectRoot 'components\ventilation') --build-path $WindowsBuild $WindowsSketch
if ($LASTEXITCODE -ne 0) { throw 'Windows firmware build failed.' }
Write-Output "Firmware ready in $WindowsBuild"
