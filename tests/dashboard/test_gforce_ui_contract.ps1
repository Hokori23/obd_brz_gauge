$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$homeRuntime = Get-Content $homeRuntimePath -Raw

if ($homeRuntime -notmatch 'gforce_outer_ring') {
    throw "ui_home_runtime.c must keep the circular outer ring for the G-force plot"
}

if ($homeRuntime -notmatch 'gforce_axis_h' -or $homeRuntime -notmatch 'gforce_axis_v') {
    throw "ui_home_runtime.c must keep the cross-axis layout for the G-force plot"
}

if ($homeRuntime -notmatch 'gforce_dot') {
    throw "ui_home_runtime.c must keep the single red current-G marker"
}

if ($homeRuntime -notmatch 'gforce_dot_glow') {
    throw "ui_home_runtime.c must keep a dedicated glow layer around the current G marker"
}

if ($homeRuntime -notmatch 'delta_mag' -or $homeRuntime -notmatch 'follow = 0\.14f') {
    throw "ui_home_runtime.c must keep the inertia-based smoothing path for the red dot"
}

if ($homeRuntime -match 'ACC\\nLEFT' -or $homeRuntime -match 'BRAKE\\nRIGHT') {
    throw "ui_home_runtime.c must not keep the old quadrant helper labels on the simplified G-force page"
}

if ($homeRuntime -match 'lv_draw_polygon') {
    throw "ui_home_runtime.c must not keep the old polygon history fill on the simplified G-force page"
}

if ($homeRuntime -notmatch 'lv_obj_set_style_bg_opa\(rt->gforce_plot,\s*LV_OPA_TRANSP') {
    throw "ui_home_runtime.c must keep the G-force plot background transparent after the visual simplification"
}

Write-Output "G-force UI contract checks passed"
