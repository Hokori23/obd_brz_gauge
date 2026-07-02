$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$headerPath = Join-Path $repoRoot "main\app_obd_dsp\aux_sensor_demand.h"
$sourcePath = Join-Path $repoRoot "main\app_obd_dsp\aux_sensor_demand.c"
$logicPath = Join-Path $repoRoot "main\app_obd_dsp\aux_sensor_demand_logic.h"

$header = Get-Content $headerPath -Raw
$source = Get-Content $sourcePath -Raw
$logic = Get-Content $logicPath -Raw

if ($header -notmatch 'AUX_OBD_DEMAND_TPS') {
    throw "aux_sensor_demand.h must declare the OBD demand mask enum"
}

if ($header -notmatch 'uint32_t aux_sensor_demand_get_obd_mask\(void\);') {
    throw "aux_sensor_demand.h must expose the OBD demand snapshot getter"
}

if ($header -notmatch 'bool aux_sensor_demand_is_gforce_obd_enabled\(void\);') {
    throw "aux_sensor_demand.h must expose the G-force OBD monitor demand getter"
}

if ($header -notmatch 'bool aux_sensor_demand_is_zc6_gear_obd_enabled\(void\);') {
    throw "aux_sensor_demand.h must expose the ZC6 gear OBD monitor demand getter"
}

if ($logic -notmatch 'AUX_SENSOR_DEMAND_MASK_COMPOSE' -or
    $logic -notmatch '\? \(AUX_OBD_DEMAND_RPM \| AUX_OBD_DEMAND_SPEED\)') {
    throw "gear page must request RPM and speed together"
}

if ($logic -notmatch '\(uses_gforce_obd_page\)' -or
    $logic -notmatch '\?\s*0u') {
    throw "G-force OBD page must stop normal PID polling and rely on monitor mode"
}

if ($source -notmatch 's_gforce_obd_enabled = gforce_obd_needed;') {
    throw "aux_sensor_demand.c must publish the G-force OBD demand flag"
}

if ($source -notmatch 's_zc6_gear_obd_enabled = zc6_gear_obd_needed;') {
    throw "aux_sensor_demand.c must publish the ZC6 gear OBD demand flag"
}

Write-Output "aux sensor demand OBD contract checks passed"
