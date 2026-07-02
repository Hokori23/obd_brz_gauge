$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$elmPath = Join-Path $repoRoot "main\bsp_obd_dsp\elm327_ble_client.c"
$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$elm = Get-Content $elmPath -Raw
$homeRuntime = Get-Content $homeRuntimePath -Raw

if ($elm -notmatch '#include "app_obd_dsp/zc6_gear_monitor_decode\.h"') {
    throw "elm327_ble_client.c must include zc6_gear_monitor_decode.h for ZC6 real gear capture"
}

if ($elm -notmatch 'ATCRA141\\r') {
    throw "elm327_ble_client.c must configure the 0x141 CAN filter for ZC6 gear monitor mode"
}

if ($elm -notmatch 'Entered ZC6 gear OBD monitor mode for CAN 0x141') {
    throw "elm327_ble_client.c must log ZC6 gear monitor mode entry"
}

if ($elm -notmatch 'aux_sensor_demand_is_zc6_gear_obd_enabled\(\)') {
    throw "elm327_ble_client.c must query ZC6 gear monitor demand"
}

if ($elm -notmatch 'obd_data_set_actual_gear\(gear\);') {
    throw "elm327_ble_client.c must publish decoded ZC6 gear values into the shared cache"
}

if ($homeRuntime -notmatch 'vehicle_profile_is_active_zc6\(\)' -or
    $homeRuntime -notmatch 'obd_data_get_actual_gear\(&gear\)') {
    throw "ui_home_runtime.c must prefer actual ZC6 gear data on the gear page"
}

Write-Output "ZC6 gear OBD monitor contract checks passed"
