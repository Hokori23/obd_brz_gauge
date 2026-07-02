$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$dashboardConfigPath = Join-Path $repoRoot "main\export_path\ui_dashboard_config.c"

$homeRuntime = Get-Content $homeRuntimePath -Raw
$dashboardConfig = Get-Content $dashboardConfigPath -Raw

if ($homeRuntime -notmatch 'LV_EVENT_LONG_PRESSED') {
    throw "Dashboard edit workflow must keep long-press entry from a home gauge tile"
}

if ($homeRuntime -notmatch 'Delete this dashboard page\?') {
    throw "Dashboard edit workflow must keep a delete confirmation step"
}

if ($homeRuntime -notmatch 'Maximum 8 dashboard pages\.') {
    throw "Dashboard add-page workflow must keep an upper-bound guard"
}

if ($homeRuntime -notmatch '(?s)ui_home_edit_config\(lv_event_t \*e\).*ui_dashboard_config_open\(') {
    throw "Dashboard edit workflow must route EDIT into the dashboard configuration screen"
}

if ($dashboardConfig -notmatch 'lv_label_set_text\(type_label,\s*"TYPE"\)') {
    throw "Dashboard configuration workflow must expose a page-type selector"
}

if ($dashboardConfig -notmatch 'lv_label_set_text\(count_label,\s*"SLOTS"\)') {
    throw "Dashboard configuration workflow must expose slot-count editing for metric pages"
}

if ($dashboardConfig -notmatch '(?s)static void ui_dashboard_config_close_to_home\(.*ui_home_runtime_rebuild_and_load\(') {
    throw "Dashboard configuration workflow must rebuild and return to the home runtime after edits"
}

Write-Output "dashboard edit workflow integration checks passed"
