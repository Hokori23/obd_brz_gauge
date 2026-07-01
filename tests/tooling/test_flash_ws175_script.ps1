$scriptPath = Join-Path $PSScriptRoot "..\..\tools\flash_ws175.ps1"
$content = Get-Content $scriptPath -Raw

if ($content -notmatch 'sdkconfig\.defaults;sdkconfig\.defaults\.esp32s3;sdkconfig\.defaults\.ws175') {
    throw "flash_ws175.ps1 must keep WS175 sdkconfig defaults chain"
}

if ($content -notmatch 'idf5\.5_py3\.11_env') {
    throw "flash_ws175.ps1 must use the ESP-IDF 5.5 Python environment"
}

if ($content -notmatch 'D:\\esp\\v5\.5\.4-clean\\tools\\activate\.py') {
    throw "flash_ws175.ps1 must activate the clean ESP-IDF 5.5.4 tree"
}

if ($content -match 'SDKCONFIG=sdkconfig\.ws175') {
    throw "flash_ws175.ps1 should rely on the default sdkconfig once WS175 becomes the repository default target"
}

if ($content -notmatch 'python -m esptool --chip esp32s3 --port \$resolvedPort --baud \$FlashBaud --before default_reset --after hard_reset write_flash "@flash_args"') {
    throw "flash_ws175.ps1 must flash WS175 via direct esptool flash_args"
}

if ($content -notmatch '\.\.\\build\\flash_args') {
    throw "flash_ws175.ps1 must flash from the default build directory"
}

if ($content -notmatch '\[switch\]\$ProbeOnly') {
    throw "flash_ws175.ps1 must expose a probe-only mode for serial bring-up"
}

if ($content -notmatch '\[switch\]\$DiagnoseOnly') {
    throw "flash_ws175.ps1 must expose a diagnose-only mode for serial bring-up"
}

if ($content -notmatch '\[int\]\$ProbeRetries = 6') {
    throw "flash_ws175.ps1 must keep probe retries configurable"
}

if ($content -notmatch '\[int\]\$ProbeTimeoutSec = 45') {
    throw "flash_ws175.ps1 must keep a bounded serial wait window for board recovery"
}

if ($content -notmatch 'function Write-SerialDiagnostics') {
    throw "flash_ws175.ps1 must provide serial diagnostics for WS175 bring-up"
}

if ($content -notmatch 'function Resolve-Ws175SerialPort') {
    throw "flash_ws175.ps1 must resolve the WS175 serial port before probe or flash"
}

if ($content -notmatch 'Preferred port \$PreferredPort not present; auto-selected') {
    throw "flash_ws175.ps1 must explain when it auto-selects a different serial port"
}

if ($content -notmatch 'while \(\$attempt -lt \$RetryCount -or \(Get-Date\) -lt \$deadline\)') {
    throw "flash_ws175.ps1 must keep waiting through device re-enumeration, not only a fixed small retry count"
}

if ($content -notmatch 'Available COM ports:') {
    throw "flash_ws175.ps1 diagnostics must enumerate COM ports"
}

if ($content -notmatch 'Matching PnP devices:') {
    throw "flash_ws175.ps1 diagnostics must inspect matching PnP devices"
}

if ($content -notmatch 'PnP-only state :') {
    throw "flash_ws175.ps1 diagnostics must distinguish PnP-only USB enumeration from a ready serial port"
}

if ($content -notmatch 'USB device is visible to Plug and Play, but the serial port is not ready for Win32 access yet') {
    throw "flash_ws175.ps1 must explain the likely meaning of a PnP-only serial state"
}

if ($content -notmatch 'Hold BOOT') {
    throw "flash_ws175.ps1 diagnostics must include manual download-mode guidance"
}

if ($content -notmatch 'Serial port \$SerialPortName is not ready after \$attempt attempts across \$TimeoutSec seconds') {
    throw "flash_ws175.ps1 must report bounded serial wait failures clearly"
}

Write-Output "flash_ws175 script test passed"
