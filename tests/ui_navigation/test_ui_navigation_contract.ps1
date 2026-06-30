$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$uiPath = Join-Path $repoRoot "main\export_path\ui.c"
$obdPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageODBProtocal.c"
$helperPath = Join-Path $repoRoot "main\export_path\ui_helpers.c"

$uiSource = Get-Content $uiPath -Raw
$obdSource = Get-Content $obdPath -Raw
$helperSource = Get-Content $helperPath -Raw

if ($uiSource -notmatch 'Logo double-click: open protocol page') {
    throw "ui.c must keep the original double-click protocol-page navigation log on the logo page"
}

if ($uiSource -notmatch 'if\(ui_ScreenPageODBProtocal == NULL\) ui_ScreenPageODBProtocal_screen_init\(\);') {
    throw "ui.c must still lazily initialize the OBD protocol page before loading it"
}

if ($uiSource -notmatch 'click_cnt') {
    throw "ui.c must retain the double-click counter for original logo navigation semantics"
}

if ($uiSource -notmatch 'Logo timeout: default_page=') {
    throw "ui.c must log the logo timeout default-page jump while we debug startup navigation"
}

if ($obdSource -notmatch 'lv_obj_add_flag\(ui_RollerODBProtocalChoose, LV_OBJ_FLAG_GESTURE_BUBBLE\)') {
    throw "ui_ScreenPageODBProtocal.c must keep gesture bubbling enabled on the protocol roller"
}

if ($obdSource -match 'lv_obj_clear_flag\(ui_RollerODBProtocalChoose, LV_OBJ_FLAG_GESTURE_BUBBLE\)') {
    throw "ui_ScreenPageODBProtocal.c must not disable gesture bubbling on the protocol roller"
}

if ($helperSource -notmatch 'screen_change: target_ptr=') {
    throw "ui_helpers.c must keep screen-change diagnostics while we debug page routing"
}

Write-Output "ui navigation contract checks passed"
