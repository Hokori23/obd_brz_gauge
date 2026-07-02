$sourcePath = Join-Path $PSScriptRoot "..\..\main\bsp_obd_dsp\nvs_storage.c"
$source = Get-Content $sourcePath -Raw

if ($source -notmatch '(?s)void nvs_stat_add_trip\(uint32_t delta_m\)\s*\{.*s_stat\.trip_m \+= delta_m;.*s_stat_dirty = true;') {
    throw "nvs_storage.c must implement nvs_stat_add_trip() and mark the stat blob dirty"
}

if ($source -notmatch '(?s)void nvs_stat_reset_trip\(void\)\s*\{.*s_stat\.trip_m = 0;.*s_stat\.max_speed_kmh = 0;.*s_stat\.avg_speed_kmh = 0;.*s_stat\.trip_run_time_s = 0;') {
    throw "nvs_storage.c must clear trip mileage, max speed, avg speed, and trip runtime in nvs_stat_reset_trip"
}

if ($source -match '(?s)void nvs_stat_reset_trip\(void\)\s*\{.*s_stat\.run_time_s = 0;') {
    throw "nvs_storage.c must not clear total run_time_s when resetting only trip statistics"
}

Write-Output "nvs storage stat contract checks passed"
