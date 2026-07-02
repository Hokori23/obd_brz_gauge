$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$homeRuntime = Get-Content $homeRuntimePath -Raw

if ($homeRuntime -notmatch 'gforce_outer_ring') {
    throw "ui_home_runtime.c must keep the circular outer ring for the G-force plot"
}

if ($homeRuntime -notmatch 'gforce_axis_h' -or $homeRuntime -notmatch 'gforce_axis_v') {
    throw "ui_home_runtime.c must keep the cross-axis layout for the G-force plot"
}

if ($homeRuntime -notmatch 'gforce_history_fill') {
    throw "ui_home_runtime.c must keep a dedicated filled historical G envelope layer"
}

if ($homeRuntime -notmatch 'lv_draw_polygon') {
    throw "ui_home_runtime.c must draw the historical G envelope as a polygon fill"
}

if ($homeRuntime -notmatch 'gforce_dot') {
    throw "ui_home_runtime.c must keep the single red current-G marker"
}

if ($homeRuntime -notmatch 'delta_mag' -or $homeRuntime -notmatch 'follow = 0\.14f') {
    throw "ui_home_runtime.c must keep the inertia-based smoothing path for the red dot"
}

if ($homeRuntime -notmatch 'history_decay =') {
    throw "ui_home_runtime.c must keep the historical G envelope decay logic"
}

if ($homeRuntime -notmatch 'quadrant_radius' -or $homeRuntime -notmatch 'quadrant_center_x') {
    throw "ui_home_runtime.c must keep quadrant labels positioned from the circular plot geometry"
}

Write-Output "G-force UI contract checks passed"
