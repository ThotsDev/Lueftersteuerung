param([string]$Compiler = 'g++.exe')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$build = Join-Path $root '.tools\host-tests'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$binary = Join-Path $build 'domain_test.exe'
& $Compiler -std=c++17 -Wall -Wextra -Werror -pedantic -static '-I' (Join-Path $root 'components\ventilation\include') (Join-Path $root 'tests\domain_test.cpp') -o $binary
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
& $binary
if ($LASTEXITCODE -ne 0) { throw 'Domain tests failed' }
