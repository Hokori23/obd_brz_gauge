$scriptPath = Join-Path $PSScriptRoot "..\..\tools\flash_ws175.ps1"
$content = Get-Content $scriptPath -Raw

if ($content -notmatch 'sdkconfig\.defaults;sdkconfig\.defaults\.esp32s3;sdkconfig\.defaults\.ws175') {
    throw "flash_ws175.ps1 must keep WS175 sdkconfig defaults chain"
}

if ($content -match 'SDKCONFIG=sdkconfig\.ws175') {
    throw "flash_ws175.ps1 should rely on the default sdkconfig once WS175 becomes the repository default target"
}

if ($content -notmatch 'python -m esptool --chip esp32s3 --port \$Port --baud \$FlashBaud --before default_reset --after hard_reset write_flash "@flash_args"') {
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

if ($content -notmatch 'while \(\$attempt -lt \$RetryCount -or \(Get-Date\) -lt \$deadline\)') {
    throw "flash_ws175.ps1 must keep waiting through device re-enumeration, not only a fixed small retry count"
}

if ($content -notmatch 'Available COM ports:') {
    throw "flash_ws175.ps1 diagnostics must enumerate COM ports"
}

if ($content -notmatch 'Hold BOOT') {
    throw "flash_ws175.ps1 diagnostics must include manual download-mode guidance"
}

if ($content -notmatch 'Serial port \$SerialPortName is not ready after \$attempt attempts across \$TimeoutSec seconds') {
    throw "flash_ws175.ps1 must report bounded serial wait failures clearly"
}

Write-Output "flash_ws175 script test passed"
