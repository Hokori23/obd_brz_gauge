$headerPath = Join-Path $PSScriptRoot "..\..\main\bsp_obd_dsp\nvs_storage.h"
$sourcePath = Join-Path $PSScriptRoot "..\..\main\bsp_obd_dsp\nvs_storage.c"

$header = Get-Content $headerPath -Raw
$source = Get-Content $sourcePath -Raw

if ($header -notmatch '#define UI_DASHBOARD_CFG_VERSION\s+1u') {
    throw "nvs_storage.h must keep dashboard config version 1 for migration compatibility"
}

if ($header -notmatch '#define UI_DASHBOARD_MAX_PAGES\s+8u') {
    throw "nvs_storage.h must keep the agreed 8-page dashboard limit"
}

if ($header -notmatch '#define UI_DASHBOARD_MAX_SLOTS\s+6u') {
    throw "nvs_storage.h must keep the agreed 6-slot dashboard limit"
}

if ($header -notmatch 'uint8_t slot_items\[UI_DASHBOARD_MAX_SLOTS\];') {
    throw "nvs_storage.h must store dashboard slot items with UI_DASHBOARD_MAX_SLOTS"
}

if ($header -notmatch 'ui_dashboard_page_cfg_t pages\[UI_DASHBOARD_MAX_PAGES\];') {
    throw "nvs_storage.h must store dashboard pages with UI_DASHBOARD_MAX_PAGES"
}

if ($header -notmatch 'ui_dashboard_cfg_t dashboard_cfg;') {
    throw "nvs_storage.h must embed dashboard_cfg into nvs_user_cfg_t"
}

if ($source -notmatch 'cfg->gauge_page_count = 1;') {
    throw "nvs_storage.c must keep a one-page default dashboard migration target"
}

if ($source -notmatch 'cfg->pages\[0\]\.slot_count = 3;') {
    throw "nvs_storage.c must keep the default migrated dashboard at 3 slots"
}

if ($source -notmatch 'cfg->pages\[0\]\.slot_items\[0\] = 5;') {
    throw "nvs_storage.c must keep RPM as the first migrated dashboard slot"
}

if ($source -notmatch 'cfg->pages\[0\]\.slot_items\[1\] = 6;') {
    throw "nvs_storage.c must keep SPEED as the second migrated dashboard slot"
}

if ($source -notmatch 'cfg->pages\[0\]\.slot_items\[2\] = 2;') {
    throw "nvs_storage.c must keep OIL as the third migrated dashboard slot"
}

Write-Output "nvs storage dashboard contract checks passed"
