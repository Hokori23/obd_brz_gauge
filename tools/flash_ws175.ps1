param(
    [string]$Port = "COM12",
    [int]$FlashBaud = 460800,
    [int]$ProbeRetries = 6,
    [int]$ProbeRetryDelayMs = 1000,
    [int]$ProbeTimeoutSec = 45,
    [switch]$BuildOnly,
    [switch]$DiagnoseOnly,
    [switch]$FlashOnly,
    [switch]$ProbeOnly,
    [switch]$NoMonitor
)

$ErrorActionPreference = "Stop"

$idfPython = "C:\Users\Hokori\.espressif\python_env\idf5.4_py3.11_env\Scripts"
$env:PATH = "$idfPython;$env:PATH"
$env:PYTHONUTF8 = "1"

$idfExports = python "D:\esp\v5.4\esp-idf\tools\activate.py" --export
. $idfExports

$idfArgs = @(
    "-D", "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175"
)

function Write-SerialDiagnostics {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SerialPortName
    )

    Write-Host ""
    Write-Host "Serial diagnostics for $SerialPortName"

    $availablePorts = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object
    if ($availablePorts.Count -gt 0) {
        Write-Host ("Available COM ports: {0}" -f ($availablePorts -join ", "))
    } else {
        Write-Host "Available COM ports: none"
    }

    $matchingPorts = Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue |
        Where-Object { $_.DeviceID -eq $SerialPortName }

    if ($matchingPorts) {
        foreach ($portInfo in $matchingPorts) {
            Write-Host ("Port status : {0}" -f $portInfo.Status)
            Write-Host ("Port name   : {0}" -f $portInfo.Name)
            Write-Host ("PNP ID      : {0}" -f $portInfo.PNPDeviceID)
        }
    } else {
        Write-Host "Port status : not enumerated in Win32_SerialPort"
    }

    $usbDevices = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
        Where-Object {
            $_.FriendlyName -match [Regex]::Escape($SerialPortName) -or
            $_.InstanceId -match 'VID_303A&PID_1001'
        }

    if ($usbDevices) {
        Write-Host "Matching PnP devices:"
        foreach ($device in $usbDevices) {
            Write-Host ("  {0} | {1} | problem={2}" -f $device.Status, $device.FriendlyName, $device.Problem)
        }
    } else {
        Write-Host "Matching PnP devices: none"
    }

    Write-Host ""
    Write-Host "If the port exists but still cannot be opened:"
    Write-Host "1. Hold BOOT."
    Write-Host "2. Tap RST once."
    Write-Host "3. Release RST."
    Write-Host "4. Release BOOT."
    Write-Host "5. Re-run this script with -ProbeOnly or -DiagnoseOnly."
    Write-Host "6. If it still fails, reconnect the board to a direct motherboard USB port."
}

function Test-SerialPortReady {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SerialPortName,
        [Parameter(Mandatory = $true)]
        [int]$RetryCount,
        [Parameter(Mandatory = $true)]
        [int]$RetryDelayMs,
        [Parameter(Mandatory = $true)]
        [int]$TimeoutSec
    )

    $lastError = $null
    $attempt = 0
    $deadline = (Get-Date).AddSeconds($TimeoutSec)

    while ($attempt -lt $RetryCount -or (Get-Date) -lt $deadline) {
        $attempt++
        $serialPort = New-Object System.IO.Ports.SerialPort $SerialPortName, 115200, 'None', 8, 'One'
        try {
            $serialPort.Open()
            $serialPort.Close()
            if ($attempt -gt 1) {
                Write-Output "Serial port $SerialPortName became ready on attempt $attempt after waiting up to $TimeoutSec seconds"
            }
            return $true
        } catch {
            $lastError = $_.Exception.Message
            if ((Get-Date) -lt $deadline) {
                Start-Sleep -Milliseconds $RetryDelayMs
            }
        } finally {
            if ($serialPort.IsOpen) {
                $serialPort.Close()
            }
            $serialPort.Dispose()
        }
    }

    Write-SerialDiagnostics -SerialPortName $SerialPortName
    throw "Serial port $SerialPortName is not ready after $attempt attempts across $TimeoutSec seconds. $lastError"
}

function Invoke-Ws175Build {
    & idf.py @idfArgs build
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

function Invoke-Ws175Probe {
    Test-SerialPortReady -SerialPortName $Port -RetryCount $ProbeRetries -RetryDelayMs $ProbeRetryDelayMs -TimeoutSec $ProbeTimeoutSec
    & python -m esptool --chip esp32s3 --port $Port chip_id
    exit $LASTEXITCODE
}

function Invoke-Ws175Flash {
    Test-SerialPortReady -SerialPortName $Port -RetryCount $ProbeRetries -RetryDelayMs $ProbeRetryDelayMs -TimeoutSec $ProbeTimeoutSec

    $flashArgsFile = Join-Path $PSScriptRoot "..\build\flash_args"
    if (-not (Test-Path $flashArgsFile)) {
        throw "flash_args not found: $flashArgsFile. Run with -BuildOnly first."
    }

    Push-Location (Join-Path $PSScriptRoot "..\build")
    try {
        & python -m esptool --chip esp32s3 --port $Port --baud $FlashBaud --before default_reset --after hard_reset write_flash "@flash_args"
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    } finally {
        Pop-Location
    }
}

if ($BuildOnly) {
    Invoke-Ws175Build
    exit 0
}

if ($DiagnoseOnly) {
    Write-SerialDiagnostics -SerialPortName $Port
    exit 0
}

if ($ProbeOnly) {
    Invoke-Ws175Probe
}

if (-not $FlashOnly) {
    Invoke-Ws175Build
}

Invoke-Ws175Flash

if (-not $NoMonitor) {
    & idf.py @idfArgs -p $Port monitor
    exit $LASTEXITCODE
}

exit 0
