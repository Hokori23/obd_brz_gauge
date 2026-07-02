$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$imuPath = Join-Path $repoRoot "main\bsp_obd_dsp\qmi8658_gforce.c"
$imu = Get-Content $imuPath -Raw

if ($imu -notmatch 'obd_data_set_lat_accel_imu_x100\(-32768\)') {
    throw "qmi8658_gforce.c must publish an invalid sentinel when IMU G-force is disabled or unavailable"
}

if ($imu -notmatch 'obd_data_set_lon_accel_imu_x100\(-32768\)') {
    throw "qmi8658_gforce.c must publish an invalid sentinel for longitudinal IMU G-force when disabled or unavailable"
}

if ($imu -notmatch 'QMI8658_DEADZONE_G') {
    throw "qmi8658_gforce.c must keep a deadzone for low-G jitter suppression"
}

if ($imu -notmatch 'QMI8658_AXIS_ALPHA') {
    throw "qmi8658_gforce.c must keep smoothing for IMU G-force updates"
}

if ($imu -notmatch 'qmi8658_gforce_set_enabled') {
    throw "qmi8658_gforce.c must keep the page-demand enable gate"
}

if ($imu -notmatch 'state->bias_ready = false') {
    throw "qmi8658_gforce.c must reset bias capture when IMU G-force is re-enabled"
}

Write-Output "QMI8658 G-force contract checks passed"
