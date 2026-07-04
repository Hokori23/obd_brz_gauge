$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$nvsStoragePath = Join-Path $repoRoot "main\bsp_obd_dsp\nvs_storage.c"
$nvsStorageHeaderPath = Join-Path $repoRoot "main\bsp_obd_dsp\nvs_storage.h"
$appBootstrapPath = Join-Path $repoRoot "main\app_bootstrap.c"
$elmPath = Join-Path $repoRoot "main\bsp_obd_dsp\elm327_ble_client.c"
$racechronoPath = Join-Path $repoRoot "main\bsp_obd_dsp\racechrono_ble_diy.c"

$nvsStorage = Get-Content $nvsStoragePath -Raw
$nvsStorageHeader = Get-Content $nvsStorageHeaderPath -Raw
$appBootstrap = Get-Content $appBootstrapPath -Raw
$elm = Get-Content $elmPath -Raw
$racechrono = Get-Content $racechronoPath -Raw

if ($nvsStorageHeader -notmatch '#define NVS_ERROR_LOG_CAPACITY\s+20u') {
    throw "Persistent error-log workflow must keep a public 20-entry capacity contract"
}

if ($nvsStorage -notmatch '#define NS_DIAG "diag"' -or
    $nvsStorage -notmatch '#define KEY_ERR_LOG "errors"') {
    throw "Persistent error-log workflow must store diagnostic entries in the dedicated diag/errors namespace"
}

if ($nvsStorage -notmatch '(?s)if \(load_blob_or_create\(NS_DIAG,\s*KEY_ERR_LOG,\s*&s_error_log,\s*sizeof\(s_error_log\)\) != ESP_OK\)') {
    throw "Persistent error-log workflow must restore the previous ring buffer at boot"
}

if ($nvsStorage -notmatch '(?s)void nvs_error_log_record\(const char \*tag,\s*esp_err_t err,\s*const char \*message\).*error_log_append_locked') {
    throw "Persistent error-log workflow must funnel writes through nvs_error_log_record()"
}

if ($appBootstrap -notmatch '(?s)app_bootstrap_dump_error_log\(void\).*nvs_error_log_copy\(&log\).*log\.count == 0u.*NVS_ERROR_LOG_CAPACITY') {
    throw "Persistent error-log workflow must support boot-time dumping from the restored ring buffer"
}

if ($nvsStorage -notmatch '(?s)while \(1\) \{\s*esp_err_t error_log_err = ESP_OK;.*nvs_storage_collect_flush_snapshot_locked\((?:&snapshot|snapshot),\s*stat_elapsed_ms >= STAT_FLUSH_PERIOD_MS\);.*save_blob\(NS_DIAG,\s*KEY_ERR_LOG,\s*&(?:snapshot|snapshot->)error_log,\s*sizeof\((?:snapshot|snapshot->)error_log\)\);.*nvs_storage_commit_flush_snapshot\((?:&snapshot|snapshot),\s*error_log_err,\s*stat_err\);') {
    throw "Persistent error-log workflow must flush dirty diagnostic entries from the background task via snapshot save flow"
}

$elmCount = ([regex]::Matches($elm, 'nvs_error_log_record\(')).Count
if ($elmCount -lt 5) {
    throw "ELM327 workflow must persist multiple critical failure paths into the error log"
}

$racechronoCount = ([regex]::Matches($racechrono, 'nvs_error_log_record\(')).Count
if ($racechronoCount -lt 4) {
    throw "RaceChrono workflow must persist multiple critical failure paths into the error log"
}

Write-Output "error log workflow integration checks passed"
