$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$appMainPath = Join-Path $repoRoot "main\app_main.c"
$appBootstrapPath = Join-Path $repoRoot "main\app_bootstrap.c"
$appBootstrapHeaderPath = Join-Path $repoRoot "main\app_bootstrap.h"
$lvglPortPath = Join-Path $repoRoot "main\app_lvgl_port.c"
$lvglPortHeaderPath = Join-Path $repoRoot "main\app_lvgl_port.h"
$bleScanPath = Join-Path $repoRoot "main\export_path\screens\ui_ScreenPageBLEScan.c"
$kconfigPath = Join-Path $repoRoot "main\Kconfig.projbuild"

$appMain = Get-Content $appMainPath -Raw
$appBootstrap = Get-Content $appBootstrapPath -Raw
$appBootstrapHeader = Get-Content $appBootstrapHeaderPath -Raw
$lvglPort = Get-Content $lvglPortPath -Raw
$lvglPortHeader = Get-Content $lvglPortHeaderPath -Raw
$bleScan = Get-Content $bleScanPath -Raw
$kconfig = Get-Content $kconfigPath -Raw

if ($appMain -match 'extern void ui_init') {
    throw "app_main.c must include ui.h directly instead of forward-declaring ui_init()"
}

if ($appMain -notmatch '#include "app_bootstrap\.h"' -or
    $appMain -notmatch '#include "app_lvgl_port\.h"' -or
    $appMain -notmatch '#include "export_path/ui\.h"') {
    throw "app_main.c must depend on the dedicated bootstrap, LVGL port, and UI headers"
}

if (($appMain.IndexOf('app_bootstrap_init_storage_and_profile();') -lt 0) -or
    ($appMain.IndexOf('app_bootstrap_init_board_and_display(&board_ctx);') -lt $appMain.IndexOf('app_bootstrap_init_storage_and_profile();')) -or
    ($appMain.IndexOf('app_lvgl_port_init(&board_ctx)') -lt $appMain.IndexOf('app_bootstrap_init_board_and_display(&board_ctx);')) -or
    ($appMain.IndexOf('app_lvgl_port_start_task()') -lt $appMain.IndexOf('app_lvgl_port_init(&board_ctx)')) -or
    ($appMain.IndexOf('app_lvgl_lock(-1)') -lt $appMain.IndexOf('app_lvgl_port_start_task()')) -or
    ($appMain.IndexOf('ui_init();') -lt $appMain.IndexOf('app_lvgl_lock(-1)')) -or
    ($appMain.IndexOf('app_lvgl_unlock();') -lt $appMain.IndexOf('ui_init();')) -or
    ($appMain.IndexOf('app_bootstrap_start_runtime_services();') -lt $appMain.IndexOf('app_lvgl_unlock();'))) {
    throw "app_main.c must orchestrate storage, board/display, LVGL port, UI init, and runtime services in order"
}

if ($appBootstrapHeader -notmatch 'void app_bootstrap_init_storage_and_profile\(void\);' -or
    $appBootstrapHeader -notmatch 'void app_bootstrap_init_board_and_display\(board_display_context_t \*out_ctx\);' -or
    $appBootstrapHeader -notmatch 'void app_bootstrap_start_runtime_services\(void\);') {
    throw "app_bootstrap.h must expose the staged bootstrap entry points"
}

if ($appBootstrap -notmatch 'vehicle_profile_set_active' -or
    $appBootstrap -notmatch 'board_display_init' -or
    $appBootstrap -notmatch 'elm327_ble_start_default' -or
    $appBootstrap -notmatch 'vMileageDataStatisticTask') {
    throw "app_bootstrap.c must own storage/profile, board/display, and runtime service startup responsibilities"
}

if ($kconfig -notmatch 'config OBD_BOOT_PRINT_ERROR_LOG') {
    throw "Kconfig must expose a boot-time stored-error-log print switch"
}

if ($appBootstrap -notmatch '(?s)#if CONFIG_OBD_BOOT_PRINT_ERROR_LOG.*app_bootstrap_dump_error_log\(void\).*nvs_error_log_copy\(&log\).*#endif' -or
    $appBootstrap -notmatch '(?s)app_bootstrap_init_storage_and_profile\(void\).*#if CONFIG_OBD_BOOT_PRINT_ERROR_LOG\s*app_bootstrap_dump_error_log\(\);\s*#endif') {
    throw "app_bootstrap.c must gate boot-time stored-error-log dumping behind CONFIG_OBD_BOOT_PRINT_ERROR_LOG"
}

if ($lvglPortHeader -notmatch 'esp_err_t app_lvgl_port_init\(const board_display_context_t \*ctx\);' -or
    $lvglPortHeader -notmatch 'esp_err_t app_lvgl_port_start_task\(void\);') {
    throw "app_lvgl_port.h must expose explicit init and task-start entry points"
}

if ($lvglPort -notmatch 'void app_lvgl_perf_trace_begin' -or
    $lvglPort -notmatch 'static void lvgl_flush_cb' -or
    $lvglPort -notmatch 'static void lvgl_touch_cb' -or
    $lvglPort -notmatch 'static void lvgl_port_task') {
    throw "app_lvgl_port.c must own LVGL perf tracing, display flush, touch input, and background task handling"
}

if ($bleScan -match 'extern bool app_lvgl_lock' -or
    $bleScan -match 'extern void app_lvgl_unlock') {
    throw "BLE scan screen must stop depending on ad-hoc extern LVGL lock declarations"
}

if ($bleScan -notmatch '#include "app_lvgl_port\.h"') {
    throw "BLE scan screen must use the dedicated LVGL port header"
}

Write-Output "app bootstrap contract checks passed"
