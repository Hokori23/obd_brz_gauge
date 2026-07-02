$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$uiPath = Join-Path $repoRoot "main\export_path\ui.c"
$dashboardConfigPath = Join-Path $repoRoot "main\export_path\ui_dashboard_config.c"
$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$helperPath = Join-Path $repoRoot "main\export_path\ui_helpers.c"
$bleScanPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageBLEScan.c"
$settingsPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageSettings.c"
$homeRuntimeHeaderPath = Join-Path $repoRoot "main\export_path\ui_home_runtime.h"
$dashboardConfigHeaderPath = Join-Path $repoRoot "main\export_path\ui_dashboard_config.h"

$uiSource = Get-Content $uiPath -Raw
$dashboardConfigSource = Get-Content $dashboardConfigPath -Raw
$homeRuntimeSource = Get-Content $homeRuntimePath -Raw
$helperSource = Get-Content $helperPath -Raw
$bleScanSource = Get-Content $bleScanPath -Raw
$settingsSource = Get-Content $settingsPath -Raw
$homeRuntimeHeader = Get-Content $homeRuntimeHeaderPath -Raw
$dashboardConfigHeader = Get-Content $dashboardConfigHeaderPath -Raw

if ($uiSource -match 'ui_ScreenPageODBProtocal' -or
    $uiSource -match 'ui_event_obd_prot_background' -or
    $uiSource -match 'click_cnt') {
    throw "ui.c should no longer keep the legacy standalone OBD protocol screen or its logo double-click path"
}

if ($uiSource -match 'void ui_show_home_page\(uint8_t page_id, lv_scr_load_anim_t anim\)') {
    throw "ui.c should not keep a redundant home-page wrapper once callers use the home runtime API directly"
}

if ($uiSource -match 'void ui_event_ble_scan_background\(lv_event_t \* e\)' -or
    $uiSource -match 'void ui_event_settings_background\(lv_event_t \* e\)') {
    throw "ui.c should not own BLE/settings home-return gesture handlers after screen-local extraction"
}

if ($uiSource -match 'ui_home_runtime_refresh_active_tile\(\);') {
    throw "ui.c should no longer directly drive dynamic-home tile refreshes from my_timerMain"
}

if ($uiSource -match 'ui_ScreenPageNeedle != NULL && lv_scr_act\(\) == ui_ScreenPageNeedle' -or
    $uiSource -match 'ui_ScreenPageOilPressure != NULL && lv_scr_act\(\) == ui_ScreenPageOilPressure' -or
    $uiSource -match 'ui_ScreenPageBrakeTemp != NULL && lv_scr_act\(\) == ui_ScreenPageBrakeTemp') {
    throw "ui.c should not branch my_timerMain on legacy fixed-home page visibility anymore"
}

if ($uiSource -match 'ui_ScreenPageBrakeWarn_screen_init\(\);' -or
    $uiSource -match 'ui_ScreenPageOilWarn_screen_init\(\);') {
    throw "ui.c should not eagerly initialize legacy warning screens during ui_init"
}

if ($homeRuntimeSource -notmatch 'LV_EVENT_LONG_PRESSED') {
    throw "ui_home_runtime.c must keep a long-press entry path for editable dashboard pages"
}

if ($homeRuntimeSource -notmatch 'Delete this dashboard page\?') {
    throw "ui_home_runtime.c must keep a delete confirmation prompt for editable dashboard pages"
}

if ($homeRuntimeSource -notmatch 'lv_obj_set_size\(zone_edit, LV_PCT\(50\), LV_PCT\(50\)\)' -or
    $homeRuntimeSource -notmatch 'lv_obj_align\(zone_edit, LV_ALIGN_TOP_LEFT, 0, 0\)' -or
    $homeRuntimeSource -notmatch 'lv_obj_set_size\(zone_del, LV_PCT\(50\), LV_PCT\(50\)\)' -or
    $homeRuntimeSource -notmatch 'lv_obj_align\(zone_del, LV_ALIGN_TOP_RIGHT, 0, 0\)' -or
    $homeRuntimeSource -notmatch 'lv_obj_set_size\(zone_back, LV_PCT\(100\), LV_PCT\(50\)\)' -or
    $homeRuntimeSource -notmatch 'lv_obj_align\(zone_back, LV_ALIGN_BOTTOM_MID, 0, 0\)') {
    throw "ui_home_runtime.c must keep the top-left EDIT, top-right DELETE, bottom BACK semicircle overlay geometry for dashboard edit mode"
}

if ((($homeRuntimeSource -notmatch 'lv_label_set_text\(label, "BACK"\)') -and
     ($homeRuntimeSource -notmatch '"BACK"')) -or
    ((($homeRuntimeSource -notmatch 'lv_label_set_text\(label, "DELETE"\)') -and
      ($homeRuntimeSource -notmatch '"DELETE"'))) -or
    ((($homeRuntimeSource -notmatch 'lv_label_set_text\(label, "EDIT"\)') -and
      ($homeRuntimeSource -notmatch '"EDIT"')))) {
    throw "ui_home_runtime.c must keep BACK/DELETE/EDIT affordances in dashboard edit mode"
}

if ($homeRuntimeSource -notmatch 'Maximum 8 dashboard pages\.') {
    throw "ui_home_runtime.c must keep an add-page limit prompt once the dashboard reaches 8 pages"
}

if ($homeRuntimeSource -notmatch '#include "ui_dashboard_config\.h"') {
    throw "ui_home_runtime.c must use the dedicated dashboard configuration module"
}

if ($homeRuntimeSource -notmatch 'lv_timer_create\(ui_home_refresh_timer_cb,\s*ui_home_refresh_period_ms_for_page\(s_home_active_page\),\s*NULL\)') {
    throw "ui_home_runtime.c must own its periodic tile refresh timer and seed it from the active page profile"
}

if ($dashboardConfigSource -notmatch 'ui_round_shell_create_title_block\(s_dashboard_config_screen,\s*"DASHBOARD"' -and
    $dashboardConfigSource -notmatch 'lv_label_set_text\(title, "DASHBOARD"\)') {
    throw "ui_dashboard_config.c must own the dedicated dashboard configuration screen"
}

if ($dashboardConfigSource -notmatch 'ui_home_runtime_rebuild_and_load\(') {
    throw "ui_dashboard_config.c must route back through the dynamic home rebuild path"
}

if ($dashboardConfigHeader -notmatch 'void ui_dashboard_config_open\(uint8_t gauge_index\);') {
    throw "ui_dashboard_config.h must expose the dashboard configuration entrypoint"
}

if ($homeRuntimeHeader -notmatch '#define UI_HOME_PAGE_MENU_ID 0u') {
    throw "ui_home_runtime.h must own the menu-page identifier for the dynamic home runtime"
}

if ($homeRuntimeHeader -notmatch 'void ui_home_runtime_show_page\(uint8_t page_id, lv_scr_load_anim_t anim\);') {
    throw "ui_home_runtime.h must expose the dynamic home screen entrypoint"
}

if ($homeRuntimeHeader -notmatch 'void ui_home_runtime_rebuild_and_load\(uint8_t page_id, lv_scr_load_anim_t anim\);') {
    throw "ui_home_runtime.h must expose the dynamic home rebuild entrypoint for sibling UI modules"
}

if ($bleScanSource -notmatch '#include \"\.\./ui_home_runtime\.h\"') {
    throw "ui_ScreenPageBLEScan.c must include ui_home_runtime.h once dynamic home runtime is extracted"
}

if ($bleScanSource -notmatch 'LV_EVENT_SCREEN_UNLOADED') {
    throw "ui_ScreenPageBLEScan.c must delete and reset its screen on unload so repeated opens rebuild fresh UI state"
}

if ($bleScanSource -notmatch '(?s)ui_ble_scan_screen_deleted\(lv_event_t \*e\).*elm327_ble_scan_only_stop\(\)') {
    throw "ui_ScreenPageBLEScan.c must stop background scan activity when the BLE scan screen is deleted"
}

if ($bleScanSource -notmatch 'ui_home_runtime_show_page\(UI_HOME_PAGE_MENU_ID, LV_SCR_LOAD_ANIM_FADE_ON\);') {
    throw "ui_ScreenPageBLEScan.c must return through the home runtime entrypoint"
}

if ($settingsSource -notmatch '#include \"\.\./ui_home_runtime\.h\"') {
    throw "ui_ScreenPageSettings.c must include ui_home_runtime.h once settings owns its home return gesture"
}

if ($settingsSource -notmatch 'LV_EVENT_SCREEN_UNLOADED') {
    throw "ui_ScreenPageSettings.c must delete and reset its screen on unload so repeated opens rebuild fresh UI state"
}

if ($settingsSource -notmatch 'lv_tileview_create\(ui_ScreenPageSettings\)') {
    throw "ui_ScreenPageSettings.c must use a horizontal tileview container for unified settings navigation"
}

if ($settingsSource -notmatch 'UI_SETTINGS_PAGE_OBD' -or
    $settingsSource -notmatch 'PROTOCOL') {
    throw "ui_ScreenPageSettings.c must integrate the OBD protocol editor into the unified settings pages"
}

if ($settingsSource -match 'LV_DIR_VER' -and $settingsSource -match 's_settings_content') {
    throw "ui_ScreenPageSettings.c must not keep the old vertical-scroll settings container"
}

if ($settingsSource -notmatch 'esp_restart\(\)') {
    throw "ui_ScreenPageSettings.c must still apply protocol changes through a reboot path"
}

if ($settingsSource -notmatch 'ui_home_runtime_show_page\(UI_HOME_PAGE_MENU_ID, LV_SCR_LOAD_ANIM_NONE\);') {
    throw "ui_ScreenPageSettings.c must return through the home runtime entrypoint"
}

if ($settingsSource -notmatch 'strlcpy\(page_names, "MENU"') {
    throw "ui_ScreenPageSettings.c must build boot-page options from the dynamic home model"
}

if ($settingsSource -notmatch '"\\nG%u"') {
    throw "ui_ScreenPageSettings.c must expose gauge boot-page options beyond MENU"
}

if ($homeRuntimeSource -notmatch 'if \(default_page > page_count\) {\s*return UI_HOME_PAGE_MENU_ID;\s*}') {
    throw "ui_home_runtime.c must map an out-of-range saved boot page back to MENU"
}

if ($uiSource -match 'ui_ScreenPageTemp' -or
    $uiSource -match 'ui_ScreenPageInfo' -or
    $uiSource -match 'ui_ScreenPageNeedle' -or
    $uiSource -match 'ui_ScreenPageOilPressure' -or
    $uiSource -match 'ui_ScreenPageBrakeTemp' -or
    $uiSource -match 'ui_ScreenPageEasterEgg' -or
    $uiSource -match 'ui_ScreenPageTempCustom' -or
    $uiSource -match 'ui_ScreenPageInfoCustom' -or
    $uiSource -match 'ui_ScreenPageNeedleConfig' -or
    $uiSource -match 'ui_ScreenPageBrakeWarn' -or
    $uiSource -match 'ui_ScreenPageOilWarn') {
    throw "ui.c should no longer keep global legacy screen state once old fixed-home pages are removed"
}

if ($dashboardConfigHeader -match 'ui_ScreenPageNeedle' -or
    $homeRuntimeHeader -match 'ui_ScreenPageNeedle') {
    throw "public headers should not leak removed legacy screen types"
}

if ((Test-Path (Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageTemp.c")) -or
    (Test-Path (Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageInfo.c")) -or
    (Test-Path (Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageNeedle.c")) -or
    (Test-Path (Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageOilPressure.c")) -or
    (Test-Path (Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageBrakeTemp.c")) -or
    (Test-Path (Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageEasterEgg.c")) -or
    (Test-Path (Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageTempCustom.c")) -or
    (Test-Path (Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageInfoCustom.c")) -or
    (Test-Path (Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageNeedleConfig.c")) -or
    (Test-Path (Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageBrakeWarn.c")) -or
    (Test-Path (Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageOilWarn.c"))) {
    throw "removed legacy fixed-home screen files should no longer remain in main/export_path/screens"
}

if (Test-Path (Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageODBProtocal.c")) {
    throw "standalone OBD protocol screen file should be removed after integrating OBD into Settings"
}

if ($helperSource -match 'screen_change:') {
    throw "ui_helpers.c should drop temporary screen-change diagnostics once page routing is stable"
}

Write-Output "ui navigation contract checks passed"
