$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$logicPath = Join-Path $repoRoot "main\app_obd_dsp\ui_gforce_plot_logic.c"
$homeRuntime = Get-Content $homeRuntimePath -Raw
$logic = Get-Content $logicPath -Raw

if ($homeRuntime -notmatch 'ui_home_gforce_draw_event') {
    throw "ui_home_runtime.c must render G-force through a dedicated draw-main callback"
}

if ($homeRuntime -notmatch 'lv_obj_add_event_cb\(rt->gforce_plot,\s*ui_home_gforce_draw_event,\s*LV_EVENT_DRAW_MAIN,\s*rt\)') {
    throw "ui_home_runtime.c must register the G-force plot draw callback on the plot object"
}

if ($homeRuntime -notmatch 'ui_gforce_plot_step\(&rt->gforce_plot_state') {
    throw "ui_home_runtime.c must drive G-force display state through the extracted pure logic helper"
}

if ($homeRuntime -notmatch 'lv_obj_invalidate\(rt->gforce_plot\)') {
    throw "ui_home_runtime.c must invalidate the single G-force draw object after updating state"
}

if ($logic -notmatch 'follow = 0\.14f' -or $logic -notmatch 'delta_mag > 0\.40f') {
    throw "ui_gforce_plot_logic.c must keep the inertia-based smoothing path for the red dot"
}

if ($homeRuntime -match 'ACC\\nLEFT' -or $homeRuntime -match 'BRAKE\\nRIGHT') {
    throw "ui_home_runtime.c must not keep the old quadrant helper labels on the simplified G-force page"
}

if ($homeRuntime -notmatch 'lv_draw_line' -or $homeRuntime -notmatch 'lv_draw_rect') {
    throw "ui_home_runtime.c must self-draw the G-force plot primitives inside the draw callback"
}

if ($homeRuntime -notmatch 'lv_obj_set_style_bg_opa\(rt->gforce_plot,\s*LV_OPA_TRANSP') {
    throw "ui_home_runtime.c must keep the G-force plot background transparent after the visual simplification"
}

Write-Output "G-force UI contract checks passed"
