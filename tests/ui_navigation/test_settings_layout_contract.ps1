$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$settingsPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageSettings.c"
$layoutHeaderPath = Join-Path $repoRoot "main\export_path\ui_layout.h"

$settings = Get-Content $settingsPath -Raw
$layoutHeader = Get-Content $layoutHeaderPath -Raw

if ($settings -notmatch 'ui_home_runtime_rebuild_and_load\(UI_HOME_PAGE_MENU_ID,\s*LV_SCR_LOAD_ANIM_NONE\);') {
    throw "Settings must rebuild the home runtime when a vehicle/profile-sensitive change requires it"
}

if ($settings -notmatch 'cfg\.rsv\[1\] = \(uint8_t\)lv_roller_get_selected\(s_roller_rotation\);') {
    throw "Settings must persist the display rotation selection into NVS"
}

if ($settings -notmatch 'DEFAULT\\nNORMAL\\nROTATE 180') {
    throw "Settings must expose the WS175 display rotation choices in the roller options"
}

if ($settings -notmatch 'static void ui_settings_safe_span_for_band\(' -or
    $settings -notmatch 'ui_settings_safe_span_for_band\(content_top,\s*content_h,') {
    throw "Settings must compute a round-screen-safe content span instead of relying on overflowing absolute rows"
}

if ($settings -notmatch 'ui_settings_create_category_card\(s_settings_content,\s*"DISPLAY"' -or
    $settings -notmatch 'ui_settings_create_category_card\(s_settings_content,\s*"DASHBOARD"' -or
    $settings -notmatch 'ui_settings_create_category_card\(s_settings_content,\s*"VEHICLE"') {
    throw "Settings root must expose DISPLAY, DASHBOARD, and VEHICLE category cards"
}

if ($settings -notmatch 'ui_settings_set_header\("DISPLAY",\s*"Screen behavior",\s*true\);' -or
    $settings -notmatch 'ui_settings_set_header\("DASHBOARD",\s*"Startup and polling",\s*true\);' -or
    $settings -notmatch 'ui_settings_set_header\("VEHICLE",\s*"Profile-aware dashboard support",\s*true\);') {
    throw "Settings detail pages must update their header title/subtitle and expose a back action"
}

if ($settings -notmatch 'lv_obj_add_flag\(s_settings_back_btn,\s*LV_OBJ_FLAG_HIDDEN\);' -or
    $settings -notmatch 'ui_settings_back_click') {
    throw "Settings must keep a dedicated back button for second-level pages"
}

Write-Output "settings layout contract checks passed"
