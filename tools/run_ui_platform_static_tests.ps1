$toolchain = "C:\Users\Hokori\.espressif\tools\xtensa-esp-elf\esp-14.2.0_20241119\xtensa-esp-elf\bin\xtensa-esp32s3-elf-gcc.exe"
$outputDir = Join-Path $PSScriptRoot "..\build-tests\host-tests"

$tests = @(
    @{
        Source = Join-Path $PSScriptRoot "..\tests\ui_platform\test_ui_platform.c"
        Output = Join-Path $outputDir "test_ui_platform.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\ui_layout\test_ui_layout.c"
        Output = Join-Path $outputDir "test_ui_layout.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\ui_font_profile\test_ui_font_profile.c"
        Output = Join-Path $outputDir "test_ui_font_profile.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\board_ws_175_amoled\test_board_ws_175_amoled_spec.c"
        Output = Join-Path $outputDir "test_board_ws_175_amoled_spec.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\board_ws_185\test_board_ws_185_spec.c"
        Output = Join-Path $outputDir "test_board_ws_185_spec.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\board_display\test_board_display_profiles.c"
        Output = Join-Path $outputDir "test_board_display_profiles.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\board_runtime_touch\test_board_runtime_touch.c"
        Output = Join-Path $outputDir "test_board_runtime_touch.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\nvs_storage\test_nvs_storage_contract.c"
        Output = Join-Path $outputDir "test_nvs_storage_contract.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\nvs_storage\test_nvs_error_log_contract.c"
        Output = Join-Path $outputDir "test_nvs_error_log_contract.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\nvs_storage\test_nvs_error_log_logic.c"
        Output = Join-Path $outputDir "test_nvs_error_log_logic.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\lvgl_buffer_profile\test_lvgl_buffer_profile.c"
        Output = Join-Path $outputDir "test_lvgl_buffer_profile.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\ble_scan_buffer_profile\test_ble_scan_buffer_profile.c"
        Output = Join-Path $outputDir "test_ble_scan_buffer_profile.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\dashboard\test_zc6_gforce_decode.c"
        Output = Join-Path $outputDir "test_zc6_gforce_decode.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\dashboard\test_aux_sensor_demand_logic.c"
        Output = Join-Path $outputDir "test_aux_sensor_demand_logic.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\dashboard\test_obd_poll_schedule_logic.c"
        Output = Join-Path $outputDir "test_obd_poll_schedule_logic.o"
    },
    @{
        Source = Join-Path $PSScriptRoot "..\tests\dashboard\test_zc6_gforce_monitor_decode.c"
        Output = Join-Path $outputDir "test_zc6_gforce_monitor_decode.o"
    }
)

if (-not (Test-Path $toolchain)) {
    throw "xtensa-esp32s3-elf-gcc not found: $toolchain"
}

New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

foreach ($test in $tests) {
     & $toolchain `
         -std=c11 `
         -Wall `
         -Wextra `
         -Werror `
         -I (Join-Path $PSScriptRoot "..\tests\include") `
         -I (Join-Path $PSScriptRoot "..") `
         -I (Join-Path $PSScriptRoot "..\main") `
         -c $test.Source `
         -o $test.Output

    if ($LASTEXITCODE -ne 0) {
        throw "static tests failed for $($test.Source)"
    }
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\tooling\test_flash_ws175_script.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "tooling tests failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\tooling\test_clean_default_build_script.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "tooling clean-build script tests failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\tooling\test_check_ws175_debug_script.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "tooling debug script tests failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\board_ws_175_amoled\test_board_ws_175_amoled_init_path.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "board_ws_175_amoled init-path test failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\ui_navigation\test_ui_navigation_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "ui navigation contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\ui_navigation\test_settings_layout_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "settings layout contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\ui_navigation\test_dashboard_config_layout_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "dashboard config layout contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\ui_font_profile\test_ui_font_profile_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "ui font profile contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\nvs_storage\test_nvs_storage_dashboard_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "nvs storage dashboard contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\nvs_storage\test_nvs_storage_stat_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "nvs storage stat contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\nvs_storage\test_nvs_storage_load_blob_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "nvs storage load_blob contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\nvs_storage\test_nvs_flush_contention_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "nvs flush contention contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\dashboard\test_aux_sensor_demand_obd_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "aux sensor demand OBD contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\dashboard\test_dashboard_vehicle_support_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "dashboard vehicle support contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\dashboard\test_obd_poll_demand_schedule_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "OBD poll demand schedule contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\dashboard\test_gforce_obd_monitor_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "G-force OBD monitor contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\dashboard\test_gforce_monitor_decode_usage_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "G-force monitor decode usage contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\dashboard\test_gforce_ui_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "G-force UI contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\dashboard\test_add_page_plus_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "ADD page plus contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\dashboard\test_ui_home_lazy_refresh_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "ui home lazy refresh contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\dashboard\test_ui_home_refresh_cadence_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "ui home refresh cadence contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\dashboard\test_racechrono_active_channel_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "RaceChrono active-channel contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\dashboard\test_qmi8658_gforce_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "QMI8658 G-force contract checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\integration\test_home_navigation_workflow_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "home navigation workflow integration checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\integration\test_dashboard_edit_workflow_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "dashboard edit workflow integration checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\integration\test_demand_driven_sensor_workflow_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "demand-driven sensor workflow integration checks failed"
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "..\tests\integration\test_error_log_workflow_contract.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "error log workflow integration checks failed"
}

$mainCMake = Get-Content (Join-Path $PSScriptRoot "..\main\CMakeLists.txt") -Raw
if ($mainCMake -notmatch 'CONFIG_OBD_BOARD_WS_175_AMOLED') {
    throw "main/CMakeLists.txt must gate WS175-specific sources"
}
if ($mainCMake -notmatch 'ads1115_oil_pressure\.c') {
    throw "main/CMakeLists.txt must explicitly handle ADS1115 source selection"
}

Write-Output "static tests passed"
