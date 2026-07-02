$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$homeRuntimeHeaderPath = Join-Path $repoRoot "main\export_path\ui_home_runtime.h"
$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$dashboardConfigPath = Join-Path $repoRoot "main\export_path\ui_dashboard_config.c"
$bleScanPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageBLEScan.c"
$settingsPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageSettings.c"
$auxDemandPath = Join-Path $repoRoot "main\app_obd_dsp\aux_sensor_demand.c"
$racechronoPath = Join-Path $repoRoot "main\bsp_obd_dsp\racechrono_ble_diy.c"

$homeRuntimeHeader = Get-Content $homeRuntimeHeaderPath -Raw
$homeRuntime = Get-Content $homeRuntimePath -Raw
$dashboardConfig = Get-Content $dashboardConfigPath -Raw
$bleScan = Get-Content $bleScanPath -Raw
$settings = Get-Content $settingsPath -Raw
$auxDemand = Get-Content $auxDemandPath -Raw
$racechrono = Get-Content $racechronoPath -Raw

if ($homeRuntimeHeader -notmatch 'bool ui_home_runtime_active_page_uses_item\(disp_item_t item\);' -or
    $homeRuntimeHeader -notmatch 'bool ui_home_runtime_active_page_uses_type\(ui_dashboard_page_type_t page_type\);') {
    throw "Demand-driven workflow must expose active home page usage queries to sibling modules"
}

if ($auxDemand -notmatch '(?s)void aux_sensor_demand_refresh\(void\).*ui_home_runtime_active_page_uses_item\(DISP_ITEM_BKT\).*racechrono_ble_diy_is_pid_enabled\(RC_PID_BRAKE_X10\).*qmi8658_gforce_set_enabled\(imu_needed\);') {
    throw "aux_sensor_demand_refresh() must merge home-page demand, RaceChrono demand, RS485 brake demand, and IMU demand"
}

if ($homeRuntime -notmatch '(?s)void ui_home_runtime_rebuild_and_load\(uint8_t page_id,\s*lv_scr_load_anim_t anim\).*aux_sensor_demand_refresh\(\);') {
    throw "Home rebuild workflow must refresh sensor demand after page topology changes"
}

if ($homeRuntime -notmatch '(?s)void ui_home_runtime_show_page\(uint8_t page_id,\s*lv_scr_load_anim_t anim\).*aux_sensor_demand_refresh\(\);') {
    throw "Home page switch workflow must refresh sensor demand when the active page changes"
}

if ($dashboardConfig -notmatch 'aux_sensor_demand_refresh\(\);') {
    throw "Dashboard configuration workflow must refresh sensor demand after edits"
}

if ($bleScan -notmatch 'aux_sensor_demand_refresh\(\);') {
    throw "BLE scan workflow must refresh sensor demand when entering the scan screen"
}

if ($settings -notmatch 'aux_sensor_demand_refresh\(\);') {
    throw "Settings workflow must refresh sensor demand when entering the settings screen"
}

if ($racechrono -notmatch 'rebuild_active_channels\(\);') {
    throw "RaceChrono workflow must rebuild active channels when stream rules or connection state changes"
}

Write-Output "demand-driven sensor workflow integration checks passed"
