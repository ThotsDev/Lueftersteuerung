#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'windows-common.ps1')
$cliVersion = '1.5.1'
$cliSha256 = 'FABE42E0EB04D00E776A66178299FF95A46C623DBC260F997E58FD514853DD40'
if (-not (Test-Path -LiteralPath $ArduinoCli)) {
    $cliDir = Split-Path $ArduinoCli -Parent
    New-Item -ItemType Directory -Path $cliDir -Force | Out-Null
    $archive = Join-Path $cliDir "arduino-cli_${cliVersion}_Windows_64bit.zip"
    Invoke-WebRequest -Uri "https://github.com/arduino/arduino-cli/releases/download/v$cliVersion/arduino-cli_${cliVersion}_Windows_64bit.zip" -OutFile $archive
    if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $cliSha256) {
        throw 'Arduino CLI checksum mismatch.'
    }
    Expand-Archive -LiteralPath $archive -DestinationPath $cliDir -Force
}
$version = & $ArduinoCli version --format json | ConvertFrom-Json
if ($LASTEXITCODE -ne 0 -or $version.VersionString -ne $cliVersion) {
    throw "Expected Arduino CLI $cliVersion."
}
& $ArduinoCli core update-index --additional-urls $ArduinoIndexUrl
if ($LASTEXITCODE -ne 0) { throw 'Arduino package index download failed.' }
& $ArduinoCli core install "esp32:esp32@$ArduinoCoreVersion" --additional-urls $ArduinoIndexUrl
if ($LASTEXITCODE -ne 0) { throw 'ESP32 core installation failed.' }
& $ArduinoCli core list
if ($LASTEXITCODE -ne 0) { throw 'Core verification failed.' }
