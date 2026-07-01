$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$nvsHeaderPath = Join-Path $repoRoot "main\bsp_obd_dsp\nvs_storage.h"
$nvsSourcePath = Join-Path $repoRoot "main\bsp_obd_dsp\nvs_storage.c"
$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$dashboardConfigPath = Join-Path $repoRoot "main\export_path\ui_dashboard_config.c"
$settingsPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageSettings.c"

$nvsHeader = Get-Content $nvsHeaderPath -Raw
$nvsSource = Get-Content $nvsSourcePath -Raw
$homeRuntime = Get-Content $homeRuntimePath -Raw
$dashboardConfig = Get-Content $dashboardConfigPath -Raw
$settingsSource = Get-Content $settingsPath -Raw

if ($nvsHeader -notmatch 'bool ui_dashboard_item_supported_for_vehicle\(uint8_t vehicle_profile_idx, uint8_t item\);') {
    throw "nvs_storage.h must expose a per-vehicle dashboard item support helper"
}

if ($nvsHeader -notmatch 'void ui_dashboard_cfg_format_for_vehicle\(ui_dashboard_cfg_t \*cfg, uint8_t vehicle_profile_idx\);') {
    throw "nvs_storage.h must expose dashboard formatting for the active vehicle"
}

if ($nvsHeader -notmatch 'bool ui_dashboard_page_slot_is_unsupported\(const ui_dashboard_page_cfg_t \*page, uint8_t slot_index\);') {
    throw "nvs_storage.h must expose unsupported-slot queries for runtime filtering"
}

if ($nvsSource -notmatch 'case DISP_ITEM_BOOST:\s*return profile->has_boost;') {
    throw "nvs_storage.c must filter boost support from the vehicle profile"
}

if ($nvsSource -notmatch 'case DISP_ITEM_OIL:\s*return profile->oil_temp_strategy.primary != OIL_TEMP_MODE_NONE') {
    throw "nvs_storage.c must filter oil temp support from the vehicle oil-temp strategy"
}

if ($nvsSource -notmatch 'ui_dashboard_cfg_format_for_vehicle\(&cfg->dashboard_cfg, cfg->vehicle_profile_idx\);') {
    throw "nvs_storage.c must reformat saved dashboard pages for the selected vehicle during sanitize"
}

if ($homeRuntime -notmatch 'ui_home_collect_visible_items') {
    throw "ui_home_runtime.c must collect only supported dashboard items before rendering"
}

if ($homeRuntime -notmatch 'NO SUPPORTED ITEM') {
    throw "ui_home_runtime.c must handle the edge case where a page has no supported items"
}

if ($homeRuntime -notmatch 'ui_dashboard_page_slot_is_unsupported') {
    throw "ui_home_runtime.c must respect persisted unsupported-slot flags"
}

if ($dashboardConfig -notmatch 'ui_dashboard_config_rebuild_supported_items') {
    throw "ui_dashboard_config.c must rebuild the supported item list for the active vehicle"
}

if ($dashboardConfig -notmatch 'ui_dashboard_item_supported_for_vehicle') {
    throw "ui_dashboard_config.c must filter slot item options by vehicle support"
}

if ($settingsSource -notmatch 'ui_home_runtime_rebuild_and_load\(UI_HOME_PAGE_MENU_ID, LV_SCR_LOAD_ANIM_NONE\);') {
    throw "ui_ScreenPageSettings.c must rebuild the home runtime when leaving settings after a vehicle change"
}

Write-Output "dashboard vehicle support contract checks passed"
