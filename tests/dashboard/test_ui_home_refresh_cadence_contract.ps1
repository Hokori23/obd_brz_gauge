$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$homeRuntime = Get-Content $homeRuntimePath -Raw

if ($homeRuntime -notmatch 'UI_HOME_REFRESH_PERIOD_MENU_MS' -or
    $homeRuntime -notmatch 'UI_HOME_REFRESH_PERIOD_GFORCE_MS') {
    throw "ui_home_runtime.c must define page-type-aware refresh period constants"
}

if ($homeRuntime -notmatch 'static uint32_t ui_home_refresh_period_ms_for_page\(uint8_t page_id\)') {
    throw "ui_home_runtime.c must compute refresh period from the active page type"
}

if ($homeRuntime -notmatch 'lv_timer_set_period\(s_home_refresh_timer,\s*ui_home_refresh_period_ms_for_page\(page_id\)\);') {
    throw "ui_home_runtime.c must retune the home refresh timer when the active page changes"
}

if ($homeRuntime -notmatch 'static void ui_home_runtime_refresh_tile\(uint8_t tile_id\)') {
    throw "ui_home_runtime.c must centralize active-page refresh behind a single-tile helper"
}

if ($homeRuntime -notmatch 'ui_home_runtime_refresh_tile\(s_home_active_page\);') {
    throw "ui_home_runtime.c must refresh only the active page on timer ticks"
}

if ($homeRuntime -match 'UI_HOME_NEIGHBOR_REFRESH_INTERVAL_TICKS' -or
    $homeRuntime -match 'include_neighbor_tiles') {
    throw "ui_home_runtime.c must not keep neighbor-refresh cadence logic once adjacent pages stop requesting data"
}

Write-Output "ui home refresh cadence contract checks passed"
