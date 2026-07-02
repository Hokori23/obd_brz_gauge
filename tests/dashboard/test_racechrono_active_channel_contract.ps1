$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$racechronoPath = Join-Path $repoRoot "main\bsp_obd_dsp\racechrono_ble_diy.c"
$racechrono = Get-Content $racechronoPath -Raw

if ($racechrono -notmatch 'typedef struct \{\s*uint8_t channel_idx;\s*int8_t rule_idx;\s*\} rc_active_channel_t;') {
    throw "racechrono_ble_diy.c must define an active-channel cache entry"
}

if ($racechrono -notmatch 'static void rebuild_active_channels\(void\)') {
    throw "racechrono_ble_diy.c must rebuild a cached active-channel list when stream rules change"
}

if ($racechrono -notmatch 'for \(uint8_t i = 0; i < s_active_channel_count; \+\+i\)') {
    throw "stream_task() must iterate the cached active-channel count instead of scanning all RC_CH_MAX channels"
}

if ($racechrono -match 'for \(int i = 0; i < RC_CH_MAX; i\+\+\)\s*\{\s*bool valid = false;\s*int32_t val = s_channels\[i\]\.read_scaled') {
    throw "stream_task() must not read every channel before checking whether it is active"
}

Write-Output "RaceChrono active-channel contract checks passed"
