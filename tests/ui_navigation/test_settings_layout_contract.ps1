$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$settingsPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageSettings.c"
$layoutHeaderPath = Join-Path $repoRoot "main\export_path\ui_layout.h"

$settings = Get-Content $settingsPath -Raw
$layoutHeader = Get-Content $layoutHeaderPath -Raw

if ($settings -notmatch 'ui_home_runtime_rebuild_and_load\(UI_HOME_PAGE_MENU_ID,\s*LV_SCR_LOAD_ANIM_NONE\);') {
    throw "Settings must rebuild the home runtime when a vehicle/profile-sensitive change requires it"
}

if ($settings -notmatch 'cfg\.rsv\[1\] = s_settings_pending_reboot_value;' -or
    $settings -notmatch 'UI_SETTINGS_REBOOT_ROTATION') {
    throw "Settings must persist the display rotation selection through the reboot-confirm flow"
}

if ($settings -notmatch 'ui_settings_rotation_options\(' -or
    $settings -notmatch 'NORMAL\\nROTATE 180' -or
    $settings -notmatch 'ui_settings_rotation_roller_index_from_mode') {
    throw "Settings must expose explicit NORMAL and ROTATE 180 outcomes so the selector matches the actual reboot result"
}

if (($settings -notmatch 'ui_round_shell_safe_span_for_band\(panel_sample_y,\s*panel_sample_h,' -and
     $settings -notmatch 'ui_round_shell_safe_span_for_band\(content_top,\s*content_h,' -and
     $settings -notmatch 'ui_settings_safe_span_for_band\(content_top,\s*content_h,')) {
    throw "Settings must compute a round-screen-safe content span, preferably from the actual middle interaction band instead of the whole page worst case"
}

if ($settings -notmatch 'UI_SETTINGS_PAGE_DISPLAY' -or
    $settings -notmatch 'UI_SETTINGS_PAGE_DASHBOARD' -or
    $settings -notmatch 'UI_SETTINGS_PAGE_VEHICLE' -or
    $settings -notmatch 'UI_SETTINGS_PAGE_OBD') {
    throw "Settings must define DISPLAY, DASHBOARD, VEHICLE, and OBD as unified horizontal pages"
}

if ($settings -notmatch 'lv_tileview_create\(ui_ScreenPageSettings\)' -or
    $settings -notmatch 'lv_tileview_add_tile\(s_settings_tileview,\s*i,\s*0,\s*dir\)') {
    throw "Settings must use a horizontal tileview to unify page switching with the home interaction model"
}

if ($settings -notmatch 's_settings_page_dots' -or
    $settings -notmatch 'ui_settings_update_page_dots') {
    throw "Settings must expose page-position feedback for horizontal category switching"
}

if ($settings -notmatch 'BOOT PAGE' -or
    $settings -notmatch 'RACECHRONO' -or
    $settings -notmatch 'OIL PRESS' -or
    $settings -notmatch 'PROFILE' -or
    $settings -notmatch 'PROTOCOL') {
    throw "Settings must keep compact inline selectors for dashboard, vehicle, and protocol configuration"
}

if ($settings -match 'ui_round_shell_create_header_button\(ui_ScreenPageSettings,\s*"HOME"' -or
    $settings -match 'ui_settings_home_click') {
    throw "Settings should rely on the unified swipe-back interaction instead of rendering a HOME button"
}

Write-Output "settings layout contract checks passed"
