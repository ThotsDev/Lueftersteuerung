$ErrorActionPreference = 'Stop'
Get-PnpDevice -PresentOnly | Where-Object {
    $_.FriendlyName -match 'CP210|CH340|CH910|ESP32|USB Serial|USB JTAG'
} | ForEach-Object {
    $problem = Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_ProblemCode -ErrorAction SilentlyContinue
    [pscustomobject]@{ Device=$_.FriendlyName; Status=$_.Status; ProblemCode=$problem.Data; Id=$_.InstanceId }
}
Get-CimInstance Win32_SerialPort | Select-Object DeviceID,Name
foreach ($name in @('git.exe','python.exe','cmake.exe','ninja.exe','wsl.exe','usbipd.exe')) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    [pscustomobject]@{Tool=$name; Path=$cmd.Source}
}
Write-Output 'Problem code 28 means: Windows has no driver installed.'
