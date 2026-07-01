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

$idfPython = "C:\Users\Hokori\.espressif\python_env\idf5.5_py3.11_env\Scripts"
$env:PATH = "$idfPython;$env:PATH"
$env:PYTHONUTF8 = "1"

$idfArgs = @(
    "-D", "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175"
)

function Initialize-Ws175EspIdf {
    $idfExports = python "D:\esp\v5.5.4-clean\tools\activate.py" --export
    . $idfExports
}

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

    $portPnpDevices = Get-PnpDevice -Class Ports -ErrorAction SilentlyContinue |
        Where-Object {
            $_.FriendlyName -match [Regex]::Escape($SerialPortName) -or
            $_.InstanceId -match 'VID_303A&PID_1001'
        }

    if ($portPnpDevices) {
        Write-Host "Matching PnP devices:"
        foreach ($device in $portPnpDevices) {
            Write-Host ("  {0} | {1} | problem={2}" -f $device.Status, $device.FriendlyName, $device.Problem)
        }
    } else {
        Write-Host "Matching PnP devices: none"
    }

    $pnpOnlyMatch = $portPnpDevices |
        Where-Object {
            $_.FriendlyName -match [Regex]::Escape($SerialPortName) -and
            -not $matchingPorts
        } |
        Select-Object -First 1

    if ($pnpOnlyMatch) {
        Write-Host ""
        Write-Host ("PnP-only state : {0}" -f $pnpOnlyMatch.Status)
        Write-Host "Interpretation : USB device is visible to Plug and Play, but the serial port is not ready for Win32 access yet."
        Write-Host "Likely causes   : missing/unstable driver, board stuck re-enumerating, or download-mode timing issue."
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

function Resolve-Ws175SerialPort {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PreferredPort
    )

    $availablePorts = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object
    if ($availablePorts -contains $PreferredPort) {
        return $PreferredPort
    }

    $espPorts = Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue |
        Where-Object {
            $_.PNPDeviceID -match 'VID_303A&PID_1001|VID_303A&PID_1002' -or
            $_.Name -match 'USB JTAG/serial debug unit|USB Serial Device'
        } |
        Sort-Object DeviceID

    if ($espPorts) {
        $resolvedPort = $espPorts[0].DeviceID
        Write-Host "Preferred port $PreferredPort not present; auto-selected $resolvedPort"
        return $resolvedPort
    }

    $pnpEspPorts = Get-PnpDevice -Class Ports -ErrorAction SilentlyContinue |
        Where-Object {
            $_.FriendlyName -match 'COM\d+' -and
            $_.InstanceId -match 'VID_303A&PID_1001|VID_303A&PID_1002'
        } |
        Sort-Object FriendlyName

    if ($pnpEspPorts) {
        $friendlyName = $pnpEspPorts[0].FriendlyName
        if ($friendlyName -match '(COM\d+)') {
            $resolvedPort = $Matches[1]
            Write-Host "Preferred port $PreferredPort not present in Win32_SerialPort; PnP reports candidate $resolvedPort"
            return $resolvedPort
        }
    }

    return $PreferredPort
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
    $resolvedPort = Resolve-Ws175SerialPort -PreferredPort $Port
    Test-SerialPortReady -SerialPortName $resolvedPort -RetryCount $ProbeRetries -RetryDelayMs $ProbeRetryDelayMs -TimeoutSec $ProbeTimeoutSec
    & python -m esptool --chip esp32s3 --port $resolvedPort chip_id
    exit $LASTEXITCODE
}

function Invoke-Ws175Flash {
    $resolvedPort = Resolve-Ws175SerialPort -PreferredPort $Port
    Test-SerialPortReady -SerialPortName $resolvedPort -RetryCount $ProbeRetries -RetryDelayMs $ProbeRetryDelayMs -TimeoutSec $ProbeTimeoutSec

    $flashArgsFile = Join-Path $PSScriptRoot "..\build\flash_args"
    if (-not (Test-Path $flashArgsFile)) {
        throw "flash_args not found: $flashArgsFile. Run with -BuildOnly first."
    }

    Push-Location (Join-Path $PSScriptRoot "..\build")
    try {
        & python -m esptool --chip esp32s3 --port $resolvedPort --baud $FlashBaud --before default_reset --after hard_reset write_flash "@flash_args"
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

Initialize-Ws175EspIdf

if ($ProbeOnly) {
    Invoke-Ws175Probe
}

if (-not $FlashOnly) {
    Invoke-Ws175Build
}

Invoke-Ws175Flash

if (-not $NoMonitor) {
    $resolvedPort = Resolve-Ws175SerialPort -PreferredPort $Port
    & idf.py @idfArgs -p $resolvedPort monitor
    exit $LASTEXITCODE
}

exit 0
