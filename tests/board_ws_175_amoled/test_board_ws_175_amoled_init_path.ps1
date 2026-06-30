$sourcePath = Join-Path $PSScriptRoot "..\..\main\bsp_obd_dsp\boards\board_ws_175_amoled.c"
$content = Get-Content $sourcePath -Raw

if ($content -notmatch 'esp_err_t board_ws_175_amoled_init\(void\)\s*\{\s*return ESP_OK;\s*\}') {
    throw "board_ws_175_amoled_init must stay side-effect free so boot can reach app_main before touch I2C init"
}

if ($content -notmatch 'static esp_err_t board_ws_175_amoled_touch_init\(void\)') {
    throw "board_ws_175_amoled.c must keep a dedicated touch init path"
}

if ($content -notmatch 'ESP_RETURN_ON_ERROR\(board_ws_175_amoled_i2c_init\(\), TAG, "i2c prepare failed"\);') {
    throw "board_ws_175_amoled_touch_init must keep I2C bring-up scoped to touch init"
}

Write-Output "board_ws_175_amoled init-path test passed"
