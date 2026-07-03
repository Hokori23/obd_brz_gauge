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

if ($homeRuntime -match 'lv_tileview_create\(ui_ScreenPageHome\)') {
    throw "Home navigation workflow must replace the home tileview with the dedicated pager container"
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

if ($settings -notmatch 'lv_tileview_create\(ui_ScreenPageSettings\)' -or
    $settings -notmatch '(?s)static void ui_settings_gesture_event\(lv_event_t \*e\).*dir != LV_DIR_TOP.*ui_settings_close_to_home\(\);') {
    throw "Settings workflow must use unified horizontal pages and top-swipe home return"
}

if ($settings -notmatch '(?s)static void ui_settings_close_to_home\(void\).*ui_home_runtime_show_page\(UI_HOME_PAGE_MENU_ID,\s*LV_SCR_LOAD_ANIM_NONE\);') {
    throw "Settings workflow must still return through the home runtime when closing settings"
}

if ($settings -match 'ui_round_shell_create_header_button\(ui_ScreenPageSettings,\s*"HOME"') {
    throw "Settings workflow should not render a separate HOME button once swipe-back is the unified return model"
}

Write-Output "home navigation workflow integration checks passed"
