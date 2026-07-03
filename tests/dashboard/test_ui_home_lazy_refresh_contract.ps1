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

$itemChangeIndex = $homeRuntime.IndexOf('if (rt->item_cache[i] != (uint8_t)item)')
if ($itemChangeIndex -lt 0) {
    throw "ui_home_runtime.c must keep a dedicated item-change branch for metric layout updates"
}

$textUpdateIndex = $homeRuntime.IndexOf('disp_item_set_text', $itemChangeIndex)
if ($textUpdateIndex -lt 0) {
    throw "ui_home_runtime.c must update metric value text after the item-change branch"
}

$itemChangeRegion = $homeRuntime.Substring($itemChangeIndex, $textUpdateIndex - $itemChangeIndex)
if ($itemChangeRegion -notmatch 'disp_item_sync_meta') {
    throw "ui_home_runtime.c must update metric label/unit metadata only inside the item-change branch"
}

if ($itemChangeRegion -notmatch 'ui_home_runtime_widgets_apply_dense_slot_style' -or
    $itemChangeRegion -notmatch 'ui_home_runtime_widgets_apply_slot_typography') {
    throw "ui_home_runtime.c must re-apply dense or box metric layout only when the slot item changes"
}

$afterTextUpdate = $homeRuntime.Substring($textUpdateIndex)
$nextLoopIndex = $afterTextUpdate.IndexOf('for (')
if ($nextLoopIndex -gt 0) {
    $afterTextUpdate = $afterTextUpdate.Substring(0, $nextLoopIndex)
}
if ($afterTextUpdate -match 'ui_home_runtime_widgets_apply_dense_slot_style' -or
    $afterTextUpdate -match 'ui_home_runtime_widgets_apply_slot_typography') {
    throw "ui_home_runtime.c must not re-apply metric typography on every refresh when the slot item has not changed"
}

Write-Output "ui home lazy refresh contract checks passed"
