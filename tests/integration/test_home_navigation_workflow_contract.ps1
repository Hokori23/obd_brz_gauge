$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$bleScanPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageBLEScan.c"
$settingsPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageSettings.c"

$homeRuntime = Get-Content $homeRuntimePath -Raw
$bleScan = Get-Content $bleScanPath -Raw
$settings = Get-Content $settingsPath -Raw

if ($homeRuntime -notmatch '(?s)static void ui_home_open_menu_overlay\(lv_dir_t dir\).*UI_ScreenPageBLEScan.*UI_ScreenPageSettings') {
    throw "Home navigation workflow must route vertical overlay gestures to both BLE scan and settings screens"
}

if ($bleScan -notmatch '(?s)static void start_scan\(void\).*elm327_ble_scan_only_start\(15,\s*scan_result_cb\);') {
    throw "BLE scan workflow must start a background scan when the scan screen is entered"
}

if ($bleScan -notmatch '(?s)static void on_ble_scan_background\(lv_event_t \*e\).*elm327_ble_scan_only_stop\(\).*ui_home_runtime_show_page\(UI_HOME_PAGE_MENU_ID,\s*LV_SCR_LOAD_ANIM_FADE_ON\);') {
    throw "BLE scan workflow must stop scanning and return to the home menu on back gesture"
}

if ($bleScan -notmatch '(?s)static void ui_ble_scan_screen_deleted\(lv_event_t \*e\).*elm327_ble_scan_only_stop\(\)') {
    throw "BLE scan workflow must stop scanning when the screen is deleted"
}

if ($settings -notmatch '(?s)static void on_settings_background\(lv_event_t \*e\).*if \(s_settings_section == UI_SETTINGS_SECTION_ROOT\) \{\s*ui_settings_close_to_home\(\);\s*\} else \{\s*ui_settings_show_section\(UI_SETTINGS_SECTION_ROOT\);\s*\}') {
    throw "Settings workflow must use top-swipe to leave the root page and to step back from second-level pages"
}

if ($settings -notmatch '(?s)static void ui_settings_close_to_home\(void\).*ui_home_runtime_show_page\(UI_HOME_PAGE_MENU_ID,\s*LV_SCR_LOAD_ANIM_MOVE_LEFT\);') {
    throw "Settings workflow must still return through the home runtime when closing settings"
}

Write-Output "home navigation workflow integration checks passed"
