#Requires -Version 7.0
param([string]$Port = 'COM5', [string]$EnvFile)
$ErrorActionPreference = 'Stop'
$serial = [System.IO.Ports.SerialPort]::new($Port, 115200)
$serial.Encoding = [System.Text.UTF8Encoding]::new($false)
$serial.NewLine = "`n"
$serial.ReadTimeout = 500
$serial.WriteTimeout = 5000
$serial.DtrEnable = $false
$serial.RtsEnable = $false
function Wait-DeviceLine([string]$Prefix, [int]$Seconds = 20) {
    $deadline = [DateTime]::UtcNow.AddSeconds($Seconds)
    while ([DateTime]::UtcNow -lt $deadline) {
        try {
            $reply = $serial.ReadLine().TrimEnd("`r")
            if ($reply.StartsWith($Prefix)) { return $reply }
            if ($reply.StartsWith('ERROR:')) { throw $reply }
        } catch [System.TimeoutException] { }
    }
    throw "Device did not respond with $Prefix. Check firmware and close other serial monitors."
}
$secret = $null
$plainPassword = $null
try {
    if ($EnvFile) {
        # Read values literally; never evaluate a local configuration as code.
        $settings = @{}
        foreach ($entry in Get-Content -LiteralPath $EnvFile -Encoding utf8) {
            if ($entry -match '^\s*(SSID|PASSWORD)\s*[:=]\s*(.*?)\s*$') {
                $key = $matches[1].ToUpperInvariant()
                $value = $matches[2]
                if ($value.Length -ge 2 -and (($value.StartsWith('"') -and $value.EndsWith('"')) -or
                    ($value.StartsWith("'") -and $value.EndsWith("'")))) {
                    $value = $value.Substring(1, $value.Length - 2)
                }
                $settings[$key] = $value
            }
        }
        if (-not $settings.ContainsKey('SSID') -or -not $settings.ContainsKey('PASSWORD')) {
            throw 'Wi-Fi file must contain SSID and PASSWORD, separated by = or :.'
        }
        $ssid = $settings['SSID']
        $plainPassword = $settings['PASSWORD']
        $settings.Clear()
        $value = $null
        $entry = $null
    } else {
        $ssid = Read-Host 'WLAN SSID (2.4 GHz)'
        $secret = Read-Host 'WLAN password (hidden; not written to files)' -AsSecureString
        $plainPassword = [System.Net.NetworkCredential]::new('', $secret).Password
    }
    $ssidBytes = [System.Text.Encoding]::UTF8.GetByteCount($ssid)
    $passwordBytes = [System.Text.Encoding]::UTF8.GetByteCount($plainPassword)
    if ($ssidBytes -lt 1 -or $ssidBytes -gt 32 -or $ssid -match '[\r\n]' -or
        $passwordBytes -lt 8 -or $passwordBytes -gt 63 -or $plainPassword -match '[\r\n]') {
        throw 'SSID must be 1..32 UTF-8 bytes and password 8..63 bytes, without line breaks.'
    }
    $serial.Open()
    $serial.DiscardInBuffer()
    $serial.WriteLine('wifi-setup')
    Wait-DeviceLine 'WIFI_SSID>' | Out-Null
    $serial.WriteLine($ssid)
    Wait-DeviceLine 'WIFI_PASSWORD>' | Out-Null
    $serial.WriteLine($plainPassword)
    $plainPassword = $null
    Wait-DeviceLine 'WIFI_SAVED:' | Write-Output
    Write-Output 'Waiting for Wi-Fi and Matter (up to 60 seconds)...'
    $deadline = [DateTime]::UtcNow.AddSeconds(60)
    $gotCode = $false
    while ([DateTime]::UtcNow -lt $deadline) {
        try {
            $reply = $serial.ReadLine().TrimEnd("`r")
            if ($reply.StartsWith('MATTER_') -or $reply.StartsWith('WIFI_') -or $reply.StartsWith('ERROR:') -or $reply.StartsWith('ALREADY_COMMISSIONED:')) {
                Write-Output $reply
            }
            if ($reply.StartsWith('MATTER_MANUAL_CODE=')) { $gotCode = $true }
            if (($gotCode -and $reply.StartsWith('MATTER_QR_URL=')) -or $reply.StartsWith('ALREADY_COMMISSIONED:')) { return }
        } catch [System.TimeoutException] { }
    }
    Write-Warning 'No complete pairing output yet. Check WLAN, then use monitor-windows.ps1 and the status/pairing commands.'
} finally {
    $plainPassword = $null
    if ($secret) { $secret.Dispose() }
    if ($serial.IsOpen) { $serial.Close() }
    $serial.Dispose()
}
