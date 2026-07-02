$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$nvsPath = Join-Path $repoRoot "main\bsp_obd_dsp\nvs_storage.c"
$nvsSource = Get-Content $nvsPath -Raw

$stackMatch = [regex]::Match($nvsSource, '#define NVS_FLUSH_TASK_STACK_BYTES\s+(\d+)')
if (-not $stackMatch.Success) {
    throw "nvs_storage.c must define NVS_FLUSH_TASK_STACK_BYTES for the flush task"
}

$stackBytes = [int]$stackMatch.Groups[1].Value
if ($stackBytes -lt 3072) {
    throw "NVS_FLUSH_TASK_STACK_BYTES must be at least 3072 bytes after the larger flush snapshot refactor"
}

if ($nvsSource -notmatch 'static nvs_flush_snapshot_t s_flush_snapshot;') {
    throw "nvs_storage.c must keep the flush snapshot out of the tiny task stack"
}

$statFlushMatch = [regex]::Match(
    $nvsSource,
    'static void stat_flush_task\(void \*arg\)(?s:.*?)^}\r?$',
    [System.Text.RegularExpressions.RegexOptions]::Multiline
)

if (-not $statFlushMatch.Success) {
    throw "Unable to locate stat_flush_task() for stack contract checks"
}

if ($statFlushMatch.Value -match 'nvs_flush_snapshot_t\s+snapshot\s*;') {
    throw "stat_flush_task() must not allocate nvs_flush_snapshot_t on its local stack"
}

if ($nvsSource -notmatch 'xTaskCreate\(\s*stat_flush_task,\s*"nvs_flush",\s*NVS_FLUSH_TASK_STACK_BYTES,') {
    throw "nvs_storage.c must create stat_flush_task with NVS_FLUSH_TASK_STACK_BYTES"
}

Write-Output "nvs flush task stack contract checks passed"
