$scriptPath = Join-Path $PSScriptRoot "..\..\tools\check_ws175_debug.ps1"
$content = Get-Content $scriptPath -Raw

if ($content -notmatch '\[string\]\$Port = "COM12"') {
    throw "check_ws175_debug.ps1 must default to COM12 for WS175 bring-up"
}

if ($content -notmatch '\[switch\]\$SmokeTestJtag') {
    throw "check_ws175_debug.ps1 must support an optional JTAG smoke test"
}

if ($content -notmatch 'factory-backup-esp32s3-\*\.bin') {
    throw "check_ws175_debug.ps1 must report latest factory backup visibility"
}

if ($content -notmatch 'Test-SerialPortOpen') {
    throw "check_ws175_debug.ps1 must validate CDC serial openability"
}

if ($content -notmatch 'USB JTAG/serial debug unit') {
    throw "check_ws175_debug.ps1 must inspect the built-in ESP32-S3 JTAG device"
}

if ($content -notmatch 'OpenOCD binary') {
    throw "check_ws175_debug.ps1 must report local OpenOCD availability"
}

if ($content -notmatch 'Invoke-OpenOcdSmokeTest') {
    throw "check_ws175_debug.ps1 must keep OpenOCD smoke-test support"
}

if ($content -notmatch 'dfu-util binary') {
    throw "check_ws175_debug.ps1 must report local dfu-util availability"
}

if ($content -notmatch 'USB_PHY_SEL eFuse') {
    throw "check_ws175_debug.ps1 must warn that DFU on ESP32-S3 may require the permanent USB_PHY_SEL eFuse path"
}

if ($content -notmatch 'avoid Set Target unless the build directory is fully idle') {
    throw "check_ws175_debug.ps1 must document the safer VS Code workflow for the default WS175 build"
}

if ($content -notmatch 'Hold BOOT') {
    throw "check_ws175_debug.ps1 must include concrete COM recovery guidance"
}

Write-Output "check_ws175_debug script test passed"
