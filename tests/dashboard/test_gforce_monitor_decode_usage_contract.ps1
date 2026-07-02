$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$elmPath = Join-Path $repoRoot "main\bsp_obd_dsp\elm327_ble_client.c"
$elm = Get-Content $elmPath -Raw

if ($elm -notmatch '#include "app_obd_dsp/zc6_gforce_monitor_decode\.h"') {
    throw "elm327_ble_client.c must include zc6_gforce_monitor_decode.h for the extracted 0x0D0 monitor decode helper"
}

if ($elm -notmatch 'zc6_gforce_decode_monitor_line\(line, &lat_x100, &lon_x100\)') {
    throw "elm327_ble_client.c must route passive 0x0D0 monitor lines through zc6_gforce_decode_monitor_line()"
}

if ($elm -notmatch 'static bool elm327_parse_gforce_monitor_line') {
    throw "elm327_ble_client.c must keep a dedicated monitor-line parser boundary for G-force OBD capture"
}

Write-Output "G-force monitor decode usage contract checks passed"
