$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$gearHeaderPath = Join-Path $repoRoot "main\app_obd_dsp\obd_data_cache.h"
$decodePath = Join-Path $repoRoot "main\app_obd_dsp\zc6_gear_monitor_decode.h"
$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"

$gearHeader = Get-Content $gearHeaderPath -Raw
$decode = Get-Content $decodePath -Raw
$homeRuntime = Get-Content $homeRuntimePath -Raw

if ($gearHeader -notmatch 'GEAR_REVERSE') {
    throw "obd_data_cache.h must define GEAR_REVERSE for real CAN-reported reverse gear"
}

if ($decode -notmatch 'case 7u:\s*\r?\n\s*\*gear_out = GEAR_REVERSE;') {
    throw "zc6_gear_monitor_decode.h must map raw gear 7 to GEAR_REVERSE"
}

if ($homeRuntime -notmatch 'case GEAR_REVERSE:\s*gear_text = "R";') {
    throw "ui_home_runtime.c must render GEAR_REVERSE as R on the gear page"
}

Write-Output "ZC6 reverse gear contract checks passed"
