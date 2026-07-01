$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$uiPath = Join-Path $repoRoot "main\export_path\ui.c"
$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$obdPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageODBProtocal.c"
$helperPath = Join-Path $repoRoot "main\export_path\ui_helpers.c"
$bleScanPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageBLEScan.c"
$settingsPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageSettings.c"
$homeRuntimeHeaderPath = Join-Path $repoRoot "main\export_path\ui_home_runtime.h"

$uiSource = Get-Content $uiPath -Raw
$homeRuntimeSource = Get-Content $homeRuntimePath -Raw
$obdSource = Get-Content $obdPath -Raw
$helperSource = Get-Content $helperPath -Raw
$bleScanSource = Get-Content $bleScanPath -Raw
$settingsSource = Get-Content $settingsPath -Raw
$homeRuntimeHeader = Get-Content $homeRuntimeHeaderPath -Raw

if ($uiSource -notmatch 'if\(ui_ScreenPageODBProtocal == NULL\) ui_ScreenPageODBProtocal_screen_init\(\);') {
    throw "ui.c must still lazily initialize the OBD protocol page before loading it"
}

if ($uiSource -notmatch 'click_cnt') {
    throw "ui.c must retain the double-click counter for original logo navigation semantics"
}

if ($uiSource -match 'void ui_show_home_page\(uint8_t page_id, lv_scr_load_anim_t anim\)') {
    throw "ui.c should not keep a redundant home-page wrapper once callers use the home runtime API directly"
}

if ($uiSource -match 'void ui_event_ble_scan_background\(lv_event_t \* e\)' -or
    $uiSource -match 'void ui_event_settings_background\(lv_event_t \* e\)') {
    throw "ui.c should not own BLE/settings home-return gesture handlers after screen-local extraction"
}

if ($homeRuntimeSource -notmatch 'LV_EVENT_LONG_PRESSED') {
    throw "ui_home_runtime.c must keep a long-press entry path for editable dashboard pages"
}

if ($homeRuntimeSource -notmatch 'Delete this dashboard page\?') {
    throw "ui_home_runtime.c must keep a delete confirmation prompt for editable dashboard pages"
}

if ($homeRuntimeSource -notmatch 'lv_obj_set_size\(zone, LV_PCT\(50\), LV_PCT\(100\)\)') {
    throw "ui_home_runtime.c must keep the half-screen edit overlay hit zones for dashboard edit mode"
}

if ($homeRuntimeSource -notmatch 'lv_label_set_text\(label, "BACK"\)' -or
    $homeRuntimeSource -notmatch 'lv_label_set_text\(label, "DELETE"\)' -or
    $homeRuntimeSource -notmatch 'lv_label_set_text\(label, "EDIT"\)') {
    throw "ui_home_runtime.c must keep BACK/DELETE/EDIT affordances in dashboard edit mode"
}

if ($homeRuntimeSource -notmatch 'Maximum 8 dashboard pages\.') {
    throw "ui_home_runtime.c must keep an add-page limit prompt once the dashboard reaches 8 pages"
}

if ($homeRuntimeSource -notmatch 'lv_label_set_text\(title, "DASHBOARD"\)') {
    throw "ui_home_runtime.c must keep the dedicated dashboard configuration screen entrypoint"
}

if ($homeRuntimeHeader -notmatch '#define UI_HOME_PAGE_MENU_ID 0u') {
    throw "ui_home_runtime.h must own the menu-page identifier for the dynamic home runtime"
}

if ($homeRuntimeHeader -notmatch 'void ui_home_runtime_show_page\(uint8_t page_id, lv_scr_load_anim_t anim\);') {
    throw "ui_home_runtime.h must expose the dynamic home screen entrypoint"
}

if ($bleScanSource -notmatch '#include \"\.\./ui_home_runtime\.h\"') {
    throw "ui_ScreenPageBLEScan.c must include ui_home_runtime.h once dynamic home runtime is extracted"
}

if ($bleScanSource -notmatch 'ui_home_runtime_show_page\(UI_HOME_PAGE_MENU_ID, LV_SCR_LOAD_ANIM_FADE_ON\);') {
    throw "ui_ScreenPageBLEScan.c must return through the home runtime entrypoint"
}

if ($settingsSource -notmatch '#include \"\.\./ui_home_runtime\.h\"') {
    throw "ui_ScreenPageSettings.c must include ui_home_runtime.h once settings owns its home return gesture"
}

if ($settingsSource -notmatch 'ui_home_runtime_show_page\(UI_HOME_PAGE_MENU_ID, LV_SCR_LOAD_ANIM_MOVE_LEFT\);') {
    throw "ui_ScreenPageSettings.c must return through the home runtime entrypoint"
}

if ($obdSource -notmatch 'lv_obj_add_flag\(ui_RollerODBProtocalChoose, LV_OBJ_FLAG_GESTURE_BUBBLE\)') {
    throw "ui_ScreenPageODBProtocal.c must keep gesture bubbling enabled on the protocol roller"
}

if ($obdSource -match 'lv_obj_clear_flag\(ui_RollerODBProtocalChoose, LV_OBJ_FLAG_GESTURE_BUBBLE\)') {
    throw "ui_ScreenPageODBProtocal.c must not disable gesture bubbling on the protocol roller"
}

if ($helperSource -match 'screen_change:') {
    throw "ui_helpers.c should drop temporary screen-change diagnostics once page routing is stable"
}

Write-Output "ui navigation contract checks passed"
