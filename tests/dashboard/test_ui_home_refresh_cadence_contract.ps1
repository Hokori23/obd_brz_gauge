$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$homeRuntime = Get-Content $homeRuntimePath -Raw

if ($homeRuntime -notmatch 'UI_HOME_NEIGHBOR_REFRESH_INTERVAL_TICKS') {
    throw "ui_home_runtime.c must define a neighbor refresh cadence constant"
}

if ($homeRuntime -notmatch 'static void ui_home_runtime_refresh_tiles\(bool include_neighbor_tiles\)') {
    throw "ui_home_runtime.c must centralize tile refresh behind an include_neighbor_tiles helper"
}

if ($homeRuntime -notmatch 'if \(!include_neighbor_tiles && tile_id != s_home_active_page\)') {
    throw "ui_home_runtime.c must skip mounted neighbor tiles on non-neighbor refresh ticks"
}

if ($homeRuntime -notmatch 'ui_home_runtime_refresh_tiles\(include_neighbor_tiles\);') {
    throw "ui_home_refresh_timer_cb() must use the shared tile refresh helper"
}

Write-Output "ui home refresh cadence contract checks passed"
