$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$dashboardConfigPath = Join-Path $repoRoot "main\export_path\ui_dashboard_config.c"
$dashboardConfig = Get-Content $dashboardConfigPath -Raw

if ($dashboardConfig -notmatch 'row_centers\[0\]\s*=\s*ui_layout_px\(-76\);' -or
    $dashboardConfig -notmatch 'row_centers\[3\]\s*=\s*ui_layout_px\(98\);') {
    throw "Dashboard config must keep the fixed four-row anchor layout used to avoid title/body overlap on the round screen"
}

if ($dashboardConfig -notmatch 'row_h = ui_layout_px\(44\);' -or
    $dashboardConfig -notmatch 'row_w = ui_layout_px\(248\);') {
    throw "Dashboard config must keep the stabilized fixed row size budget for the round-screen editor"
}

if ($dashboardConfig -notmatch 'lv_obj_align\(s_dashboard_cfg_type_row,\s*LV_ALIGN_CENTER,\s*0,\s*row_centers\[0\]\);' -or
    $dashboardConfig -notmatch 'lv_obj_align\(s_dashboard_cfg_slot_item_row,\s*LV_ALIGN_CENTER,\s*0,\s*row_centers\[3\]\);') {
    throw "Dashboard config must anchor each editor row explicitly instead of relying on the broken compressed flex stack"
}

if ($dashboardConfig -match 'lv_obj_set_flex_flow\(s_dashboard_cfg_body,\s*LV_FLEX_FLOW_COLUMN\)' -or
    $dashboardConfig -match 'lv_obj_set_scroll_dir\(s_dashboard_cfg_body,\s*LV_DIR_VER\)') {
    throw "Dashboard config must not keep a page-level vertical scroll container that can fight the return interaction"
}

if ($dashboardConfig -notmatch 'EDIT SLOT' -or
    $dashboardConfig -notmatch 's_dashboard_cfg_active_slot_index') {
    throw "Dashboard config must keep the single-slot editing model so metric configuration fits without vertical scrolling"
}

if ($dashboardConfig -match 'ui_round_shell_create_header_button\(s_dashboard_config_screen,\s*"HOME"' -or
    $dashboardConfig -match 'ui_dashboard_config_home_click') {
    throw "Dashboard config must not expose a separate HOME button once round pages unify on gesture/BACK return"
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

if ($dashboardConfig -notmatch 'ui_round_shell_create_ring\(s_dashboard_config_screen,\s*&layout\.shell\)') {
    throw "Dashboard config must share the same round-shell chrome as the other runtime pages"
}

if ($dashboardConfig -notmatch 'if \(s_dashboard_config_gauge_index < dashboard->gauge_page_count\) {\s*return \(uint8_t\)\(s_dashboard_config_gauge_index \+ 1u\);\s*}\s*return UI_HOME_PAGE_MENU_ID;') {
    throw "Dashboard config return-page logic must prefer the originating gauge page and fall back to MENU when invalid"
}

Write-Output "dashboard config layout contract checks passed"
