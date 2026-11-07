#Requires -Version 7.0
param([string]$Port = 'COM5')
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'windows-common.ps1')
& $ArduinoCli monitor --port $Port --config baudrate=115200
if ($LASTEXITCODE -ne 0) { throw 'Serial monitor failed.' }
