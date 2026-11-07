#Requires -Version 7.0
param([string]$Port = 'COM5', [switch]$EraseAll)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'windows-common.ps1')
if (-not (Test-Path -LiteralPath (Join-Path $WindowsBuild 'ventilation_controller.ino.bin'))) {
    throw 'Run scripts/build-windows.ps1 first.'
}
$backups = @(Get-ChildItem -LiteralPath (Join-Path $ProjectRoot '.tools\backups') -Filter '*.bin' -ErrorAction SilentlyContinue | Where-Object Length -eq 4194304)
if ($backups.Count -eq 0) { throw 'Create a complete 4 MiB flash backup in .tools/backups before the first upload.' }
$board = if ($EraseAll) { "$ArduinoBoard,EraseFlash=all" } else { $ArduinoBoard }
& $ArduinoCli upload --fqbn $board --port $Port --input-dir $WindowsBuild $WindowsSketch
if ($LASTEXITCODE -ne 0) { throw 'Firmware upload failed.' }
