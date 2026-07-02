$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$dashboardConfigPath = Join-Path $repoRoot "main\export_path\ui_dashboard_config.c"
$dashboardConfig = Get-Content $dashboardConfigPath -Raw

if ($dashboardConfig -notmatch 'static void ui_dashboard_config_safe_span_for_band\(') {
    throw "Dashboard config must keep the safe-span helper for round-screen horizontal bounds"
}

if ($dashboardConfig -notmatch 'safe_margin = ui_safe_margin\(\);' -or
    $dashboardConfig -notmatch 'body_h = \(lv_coord_t\)\(ui_screen_height\(\) - bottom_pad - body_top\);') {
    throw "Dashboard config must derive its body height from the runtime screen bounds"
}

if ($dashboardConfig -notmatch 'row_h = \(lv_coord_t\)\(\(body_h - \(\(total_row_count - 1u\) \* body_row_gap\)\) / total_row_count\);') {
    throw "Dashboard config must compute row height from available space instead of hard-coded slot Y positions"
}

if ($dashboardConfig -notmatch 'ui_dashboard_config_safe_span_for_band\(body_top,\s*body_h,\s*body_inset,\s*&body_left,\s*&body_right\);') {
    throw "Dashboard config must fit its control body within the round-screen safe span"
}

if ($dashboardConfig -notmatch 'lv_label_set_text\(err_label,\s*"No page config"\)') {
    throw "Dashboard config must keep an explicit 'No page config' fallback for invalid dashboard edit entry"
}

if ($dashboardConfig -notmatch 'lv_label_set_text\(back_label,\s*"BACK"\)') {
    throw "Dashboard config fallback must expose a visible BACK action"
}

if ($dashboardConfig -notmatch 'ui_dashboard_config_error_back' -or
    $dashboardConfig -notmatch 'ui_dashboard_config_close_to_home\(LV_SCR_LOAD_ANIM_NONE\);') {
    throw "Dashboard config fallback BACK action must return through the shared home rebuild path"
}

if ($dashboardConfig -notmatch 'if \(s_dashboard_config_gauge_index < dashboard->gauge_page_count\) {\s*return \(uint8_t\)\(s_dashboard_config_gauge_index \+ 1u\);\s*}\s*return UI_HOME_PAGE_MENU_ID;') {
    throw "Dashboard config return-page logic must prefer the originating gauge page and fall back to MENU when invalid"
}

Write-Output "dashboard config layout contract checks passed"
