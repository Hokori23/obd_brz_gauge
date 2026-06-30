param(
    [string]$Port = "COM12",
    [switch]$SmokeTestJtag
)

$ErrorActionPreference = "Stop"

function Write-Section {
    param([string]$Title)

    Write-Host ""
    Write-Host "=== $Title ==="
}

function Get-OpenOcdBinary {
    $toolRoots = @(
        "C:\Users\Hokori\.espressif\tools\openocd-esp32"
    )

    foreach ($toolRoot in $toolRoots) {
        if (-not (Test-Path $toolRoot)) {
            continue
        }

        $openOcd = Get-ChildItem -Path $toolRoot -Recurse -Filter openocd.exe -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($openOcd) {
            return $openOcd.FullName
        }
    }

    return $null
}

function Get-DfuUtilBinary {
    $toolRoots = @(
        "C:\Users\Hokori\.espressif\tools\dfu-util"
    )

    foreach ($toolRoot in $toolRoots) {
        if (-not (Test-Path $toolRoot)) {
            continue
        }

        $dfuUtil = Get-ChildItem -Path $toolRoot -Recurse -Filter dfu-util.exe -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($dfuUtil) {
            return $dfuUtil.FullName
        }
    }

    return $null
}

function Test-SerialPortOpen {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SerialPortName
    )

    $serialPort = New-Object System.IO.Ports.SerialPort $SerialPortName, 115200, 'None', 8, 'One'
    try {
        $serialPort.Open()
        return @{
            Ok = $true
            Message = "Serial port opened successfully"
        }
    } catch {
        return @{
            Ok = $false
            Message = $_.Exception.Message
        }
    } finally {
        if ($serialPort.IsOpen) {
            $serialPort.Close()
        }
        $serialPort.Dispose()
    }
}

function Invoke-OpenOcdSmokeTest {
    param(
        [Parameter(Mandatory = $true)]
        [string]$OpenOcdBinary
    )

    $scriptRoot = Split-Path $OpenOcdBinary -Parent
    $openOcdScripts = Join-Path (Split-Path $scriptRoot -Parent) "share\openocd\scripts"
    $boardCfg = Join-Path $openOcdScripts "board\esp32s3-builtin.cfg"
    $stdoutLog = Join-Path $env:TEMP "ws175-openocd-stdout.log"
    $stderrLog = Join-Path $env:TEMP "ws175-openocd-stderr.log"

    if (-not (Test-Path $boardCfg)) {
        return @{
            Ok = $false
            Message = "OpenOCD board config not found: $boardCfg"
        }
    }

    if (Test-Path $stdoutLog) {
        Remove-Item -LiteralPath $stdoutLog -Force
    }
    if (Test-Path $stderrLog) {
        Remove-Item -LiteralPath $stderrLog -Force
    }

    $process = Start-Process -FilePath $OpenOcdBinary `
        -ArgumentList @("-f", $boardCfg) `
        -RedirectStandardOutput $stdoutLog `
        -RedirectStandardError $stderrLog `
        -PassThru `
        -WindowStyle Hidden

    try {
        if ($process.WaitForExit(5000)) {
            $output = @()
            if (Test-Path $stdoutLog) {
                $output += Get-Content $stdoutLog
            }
            if (Test-Path $stderrLog) {
                $output += Get-Content $stderrLog
            }
            $firstInterestingLine = ($output | Where-Object { $_ -match 'Error:|LIBUSB_' } | Select-Object -First 1)
            if (-not $firstInterestingLine) {
                $firstInterestingLine = $output |
                    Where-Object { $_ -match 'Listening on port|Info :' } |
                    Select-Object -First 1
            }
            return @{
                Ok = ($process.ExitCode -eq 0)
                Message = $(if ($firstInterestingLine) { $firstInterestingLine } else { "OpenOCD exited with code $($process.ExitCode)" })
            }
        }

        $process.Kill()
        $process.WaitForExit()
        return @{
            Ok = $true
            Message = "OpenOCD stayed alive for 5s; JTAG transport likely attached"
        }
    } finally {
        if (-not $process.HasExited) {
            $process.Kill()
            $process.WaitForExit()
        }
    }
}

Write-Section "Backup"
$backupDir = Join-Path $PSScriptRoot "..\backups"
$factoryBackups = @()
if (Test-Path $backupDir) {
    $factoryBackups = Get-ChildItem -Path $backupDir -Filter "factory-backup-esp32s3-*.bin" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending
}
if ($factoryBackups.Count -gt 0) {
    $latestBackup = $factoryBackups[0]
    Write-Host "Latest factory backup: $($latestBackup.FullName)"
    Write-Host "Latest backup size   : $($latestBackup.Length) bytes"
} else {
    Write-Host "Latest factory backup: not found"
}

Write-Section "COM Port"
$availablePorts = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object
Write-Host ("Available COM ports  : {0}" -f ($(if ($availablePorts.Count -gt 0) { $availablePorts -join ", " } else { "none" })))

$serialDevice = Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue |
    Where-Object { $_.DeviceID -eq $Port } |
    Select-Object -First 1

if ($serialDevice) {
    Write-Host "Serial device name   : $($serialDevice.Name)"
    Write-Host "Serial device status : $($serialDevice.Status)"
    Write-Host "Serial device PNP ID : $($serialDevice.PNPDeviceID)"
} else {
    Write-Host "Serial device name   : not enumerated"
}

$serialOpenResult = Test-SerialPortOpen -SerialPortName $Port
Write-Host "Serial open result   : $(if ($serialOpenResult.Ok) { "OK" } else { "FAIL" })"
Write-Host "Serial open detail   : $($serialOpenResult.Message)"

Write-Section "USB JTAG"
$jtagDevice = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
    Where-Object { $_.FriendlyName -eq "USB JTAG/serial debug unit" } |
    Select-Object -First 1

if ($jtagDevice) {
    Write-Host "JTAG device status   : $($jtagDevice.Status)"
    Write-Host "JTAG device problem  : $($jtagDevice.Problem)"
    Write-Host "JTAG instance ID     : $($jtagDevice.InstanceId)"

    $jtagDriver = Get-PnpDeviceProperty -InstanceId $jtagDevice.InstanceId -ErrorAction SilentlyContinue |
        Where-Object { $_.KeyName -eq "DEVPKEY_Device_DriverInfPath" } |
        Select-Object -First 1
    $jtagService = Get-PnpDeviceProperty -InstanceId $jtagDevice.InstanceId -ErrorAction SilentlyContinue |
        Where-Object { $_.KeyName -eq "DEVPKEY_Device_Service" } |
        Select-Object -First 1

    if ($jtagDriver) {
        Write-Host "JTAG driver inf path : $($jtagDriver.Data)"
    }
    if ($jtagService) {
        Write-Host "JTAG driver service  : $($jtagService.Data)"
    }
} else {
    Write-Host "JTAG device status   : not enumerated"
}

Write-Section "OpenOCD"
$openOcdBinary = Get-OpenOcdBinary
if ($openOcdBinary) {
    Write-Host "OpenOCD binary       : $openOcdBinary"
} else {
    Write-Host "OpenOCD binary       : not found"
}

if ($SmokeTestJtag -and $openOcdBinary) {
    $jtagSmokeResult = Invoke-OpenOcdSmokeTest -OpenOcdBinary $openOcdBinary
    Write-Host "OpenOCD smoke result : $(if ($jtagSmokeResult.Ok) { "OK" } else { "FAIL" })"
    Write-Host "OpenOCD smoke detail : $($jtagSmokeResult.Message)"
}

Write-Section "DFU"
$dfuUtilBinary = Get-DfuUtilBinary
if ($dfuUtilBinary) {
    Write-Host "dfu-util binary      : $dfuUtilBinary"
} else {
    Write-Host "dfu-util binary      : not found"
}
Write-Host "DFU note             : ESP32-S3 DFU needs USB OTG access. On the default internal USB Serial/JTAG path, switching to DFU can require the permanent USB_PHY_SEL eFuse or an external USB PHY."

Write-Section "Hints"
if (-not $serialOpenResult.Ok) {
    Write-Host "COM recovery steps:"
    Write-Host "1. Hold BOOT."
    Write-Host "2. Tap RST once."
    Write-Host "3. Release RST."
    Write-Host "4. Release BOOT."
    Write-Host "5. Re-run flash_ws175.ps1 -ProbeOnly -Port $Port"
}

if ($jtagDevice -and $openOcdBinary) {
    Write-Host "JTAG path is present in Windows. If OpenOCD still cannot attach, capture its first error line next."
}

Write-Host "VS Code note         : This repository now defaults to WS175 on the root build directory. In the ESP-IDF VS Code extension, use Build/Flash directly and avoid Set Target unless the build directory is fully idle."
