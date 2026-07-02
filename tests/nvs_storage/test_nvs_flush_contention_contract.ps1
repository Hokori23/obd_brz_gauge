$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$nvsPath = Join-Path $repoRoot "main\bsp_obd_dsp\nvs_storage.c"
$nvsSource = Get-Content $nvsPath -Raw
$statFlushMatch = [regex]::Match(
    $nvsSource,
    'static void stat_flush_task\(void \*arg\)(?s:.*?)^}\r?$',
    [System.Text.RegularExpressions.RegexOptions]::Multiline
)

if ($nvsSource -notmatch 'static void nvs_storage_collect_flush_snapshot_locked\(') {
    throw "nvs_storage.c must keep a snapshot helper for flush batching"
}

if ($nvsSource -notmatch 'static void nvs_storage_commit_flush_snapshot\(') {
    throw "nvs_storage.c must keep a post-save commit helper to clear dirty flags safely"
}

if (-not $statFlushMatch.Success) {
    throw "Unable to locate stat_flush_task() for flush-contention contract checks"
}

if ($statFlushMatch.Value -match '(?s)xSemaphoreTake\(s_mux, portMAX_DELAY\).*?save_blob\(') {
    throw "stat_flush_task() must not hold s_mux across save_blob() calls"
}

Write-Output "nvs flush contention contract checks passed"
