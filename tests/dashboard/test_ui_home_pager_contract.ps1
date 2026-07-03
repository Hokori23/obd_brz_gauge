$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$pagerPath = Join-Path $repoRoot "main\export_path\ui_home_pager.c"
$pagerHeaderPath = Join-Path $repoRoot "main\export_path\ui_home_pager.h"

$homeRuntime = Get-Content $homeRuntimePath -Raw
$pager = Get-Content $pagerPath -Raw
$pagerHeader = Get-Content $pagerHeaderPath -Raw

if ($homeRuntime -match 'lv_tileview_create' -or
    $homeRuntime -match 'lv_tileview_get_tile_act' -or
    $homeRuntime -match 'lv_obj_set_tile_id') {
    throw "ui_home_runtime.c must stop depending on LVGL tileview primitives on the home path"
}

if ($homeRuntime -notmatch 'static void ui_home_sync_tile_mounts\(') {
    throw "ui_home_runtime.c must keep the dedicated neighbor-mount policy helper"
}

if ($homeRuntime -notmatch 'static void ui_home_runtime_refresh_visible_tiles\(') {
    throw "ui_home_runtime.c must keep the visible-tile refresh helper"
}

if ($homeRuntime -notmatch 'static void ui_home_open_menu_overlay\(lv_dir_t dir\)') {
    throw "ui_home_runtime.c must keep menu overlay routing isolated in ui_home_open_menu_overlay()"
}

if ($homeRuntime -notmatch 'ui_home_pager_set_locked\(&s_home_pager,\s*true\)' -or
    $homeRuntime -notmatch 'ui_home_pager_set_locked\(&s_home_pager,\s*false\)') {
    throw "Home edit mode must explicitly lock and unlock the dedicated pager"
}

if ($pagerHeader -notmatch 'ui_home_pager_axis_from_delta' -or
    $pagerHeader -notmatch 'ui_home_pager_target_page_from_release') {
    throw "ui_home_pager.h must expose testable pure logic helpers for axis locking and release targeting"
}

if ($pager -notmatch 'UI_HOME_PAGER_AXIS_X' -or
    $pager -notmatch 'LV_EVENT_PRESSING' -or
    $pager -notmatch 'LV_EVENT_PRESS_LOST' -or
    $pager -notmatch 'lv_anim_path_ease_out') {
    throw "ui_home_pager.c must implement pointer-driven axis locking and ease-out settle animation"
}

Write-Output "ui home pager contract checks passed"
