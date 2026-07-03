$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$elmPath = Join-Path $repoRoot "main\bsp_obd_dsp\elm327_ble_client.c"
$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"

$elm = Get-Content $elmPath -Raw
$homeRuntime = Get-Content $homeRuntimePath -Raw

if ($elm -notmatch 'ATCRA0D0\\r') {
    throw "elm327_ble_client.c must configure the 0x0D0 CAN filter for G-force monitor mode"
}

if ($elm -notmatch 'ATMA\\r') {
    throw "elm327_ble_client.c must enter monitor-all mode for passive CAN capture"
}

if ($elm -notmatch 'elm327_feed_gforce_monitor_bytes') {
    throw "elm327_ble_client.c must stream notify bytes through the G-force monitor parser"
}

if ($elm -notmatch 'obd_data_set_lat_accel_x100') {
    throw "elm327_ble_client.c must decode lateral acceleration into the OBD cache"
}

if ($elm -notmatch 'obd_data_set_lon_accel_x100') {
    throw "elm327_ble_client.c must decode longitudinal acceleration into the OBD cache"
}

if ($homeRuntime -notmatch 'ui_home_gforce_draw_event') {
    throw "ui_home_runtime.c must keep a dedicated draw pipeline for the G-force page"
}

if ($homeRuntime -notmatch 'ui_gforce_plot_step') {
    throw "ui_home_runtime.c must keep the extracted G-force plot state update path"
}

Write-Output "G-force OBD monitor contract checks passed"
