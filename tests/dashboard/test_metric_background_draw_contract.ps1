$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$homeRuntime = Get-Content $homeRuntimePath -Raw

if ($homeRuntime -notmatch 'static void ui_home_metric_bg_draw_event\(') {
    throw "ui_home_runtime.c must expose a dedicated metric background draw callback"
}

if ($homeRuntime -notmatch 'metric_bg_draw_obj = lv_obj_create\(parent\)') {
    throw "ui_home_runtime.c must create one page-level draw object for metric dashboard backgrounds"
}

if ($homeRuntime -notmatch 'lv_obj_add_event_cb\(rt->metric_bg_draw_obj,\s*ui_home_metric_bg_draw_event,\s*LV_EVENT_DRAW_MAIN,\s*rt\)') {
    throw "ui_home_runtime.c must bind the metric background draw object to the draw-main callback"
}

if ($homeRuntime -match 'ui_round_shell_create_divider\(parent,\s*divider_x') {
    throw "ui_home_build_gauge_layout() must no longer create per-divider objects for metric backgrounds"
}

Write-Output "metric background draw contract checks passed"

