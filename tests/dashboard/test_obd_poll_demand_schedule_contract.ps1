$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$logicPath = Join-Path $repoRoot "main\app_obd_dsp\obd_poll_schedule_logic.h"
$elmPath = Join-Path $repoRoot "main\bsp_obd_dsp\elm327_ble_client.c"

$logic = Get-Content $logicPath -Raw
$elm = Get-Content $elmPath -Raw

if ($logic -notmatch 'obd_poll_schedule_find_next_active_index') {
    throw "OBD poll scheduling must expose a pure helper for demanded-slot selection"
}

if ($elm -notmatch 'obd_poll_schedule_find_next_active_index\(') {
    throw "elm327_ble_client.c must use the demanded-slot scheduling helper"
}

if ($elm -notmatch 'if \(!obd_poll_schedule_find_next_active_index\(') {
    throw "elm327_ble_client.c must short-circuit when no demanded OBD slot is active"
}

if ($elm -notmatch 'tick_count = \(uint32_t\)poll_index;') {
    throw "elm327_ble_client.c must jump directly to the next demanded slot instead of burning delay on skipped slots"
}

Write-Output "OBD poll demand schedule contract checks passed"
