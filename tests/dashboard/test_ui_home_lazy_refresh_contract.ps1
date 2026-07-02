$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$homeRuntime = Get-Content $homeRuntimePath -Raw

if ($homeRuntime -notmatch 'static bool ui_home_read_disp_item_value\(') {
    throw "ui_home_runtime.c must keep a dedicated lazy value reader for visible dashboard items"
}

if ($homeRuntime -notmatch 'static void ui_home_refresh_metric_tile\(') {
    throw "ui_home_runtime.c must split metric-tile refresh into its own helper"
}

if ($homeRuntime -match '(?s)void ui_home_runtime_refresh_active_tile\(.*?int16_t clt = obd_data_get_coolant_temp\(\);') {
    throw "ui_home_runtime_refresh_active_tile() must not eagerly read all OBD values before page-type dispatch"
}

if ($homeRuntime -match 'disp_item_read_value\(item,\s*clt,\s*iat,\s*oil,\s*load_pct,\s*tps,\s*bat_mv') {
    throw "ui_home_runtime.c must no longer route home-page refresh through the eager bulk disp_item_read_value() path"
}

Write-Output "ui home lazy refresh contract checks passed"
