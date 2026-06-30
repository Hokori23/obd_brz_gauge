param(
    [string]$Port = "COM12",
    [string]$Chip = "esp32s3",
    [string]$OutputDir = "",
    [UInt32]$FlashSizeBytes = 0x1000000,
    [int]$Baud = 115200,
    [ValidateSet("single", "chunked")]
    [string]$Mode = "chunked",
    [UInt32]$ChunkSizeBytes = 0x10000,
    [string]$ResumeBaseName = "",
    [switch]$ForceReRead
)

$ErrorActionPreference = "Stop"

$pythonCommand = (Get-Command python -ErrorAction Stop).Source

function Format-Hex32 {
    param([UInt32]$Value)
    return ("0x{0:x}" -f $Value)
}

function Invoke-EsptoolReadFlash {
    param(
        [UInt32]$Offset,
        [UInt32]$Size,
        [string]$TargetFile,
        [string]$LogFile
    )

    $esptoolArgs = @(
        "-m", "esptool",
        "--chip", $Chip,
        "--port", $Port,
        "--baud", "$Baud",
        "read_flash", (Format-Hex32 $Offset), (Format-Hex32 $Size), $TargetFile
    )

    & $pythonCommand @esptoolArgs 2>&1 | Tee-Object -FilePath $LogFile
    if ($LASTEXITCODE -ne 0) {
        throw "esptool read_flash failed at offset $(Format-Hex32 $Offset) with exit code $LASTEXITCODE"
    }
}

function Get-PartPaths {
    param(
        [string]$BaseDir,
        [int]$ChunkIndex,
        [UInt32]$Offset
    )

    $offsetSuffix = (Format-Hex32 $Offset).Substring(2)
    return @{
        PartFile = Join-Path $BaseDir ("part-{0:d4}-{1}.bin" -f $ChunkIndex, $offsetSuffix)
        PartLog  = Join-Path $BaseDir ("part-{0:d4}-{1}.log" -f $ChunkIndex, $offsetSuffix)
    }
}

if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = Join-Path $PSScriptRoot "..\\backups"
}

if ($ChunkSizeBytes -eq 0) {
    throw "ChunkSizeBytes must be greater than zero."
}

$resolvedOutputDir = [System.IO.Path]::GetFullPath($OutputDir)
New-Item -ItemType Directory -Force -Path $resolvedOutputDir | Out-Null

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$baseName = if ([string]::IsNullOrWhiteSpace($ResumeBaseName)) {
    "factory-backup-$Chip-$Port-$timestamp"
} else {
    $ResumeBaseName
}
$outputFile = Join-Path $resolvedOutputDir "$baseName.bin"
$logFile = Join-Path $resolvedOutputDir "$baseName.log"
$chunkDir = Join-Path $resolvedOutputDir "$baseName.parts"

Write-Host "Backing up flash from $Port"
Write-Host "Mode : $Mode"
Write-Host "Chip : $Chip"
Write-Host "Baud : $Baud"
Write-Host "Size : $(Format-Hex32 $FlashSizeBytes) ($FlashSizeBytes bytes)"
if (-not [string]::IsNullOrWhiteSpace($ResumeBaseName)) {
    Write-Host "Resume base : $ResumeBaseName"
}

try {
    if ($Mode -eq "single") {
        Write-Host "Image: $outputFile"
        Invoke-EsptoolReadFlash -Offset 0 -Size $FlashSizeBytes -TargetFile $outputFile -LogFile $logFile
    }
    else {
        New-Item -ItemType Directory -Force -Path $chunkDir | Out-Null
        if ((Test-Path $outputFile) -and [string]::IsNullOrWhiteSpace($ResumeBaseName)) {
            Remove-Item -LiteralPath $outputFile -Force
        }

        $offset = [UInt32]0
        $chunkIndex = 0

        while ($offset -lt $FlashSizeBytes) {
            $remaining = $FlashSizeBytes - $offset
            $readSize = [Math]::Min([UInt32]$ChunkSizeBytes, [UInt32]$remaining)
            $paths = Get-PartPaths -BaseDir $chunkDir -ChunkIndex $chunkIndex -Offset $offset
            $partFile = $paths.PartFile
            $partLog = $paths.PartLog

            $reuseChunk = $false
            if ((-not $ForceReRead) -and (Test-Path $partFile)) {
                $partInfo = Get-Item $partFile
                if ($partInfo.Length -eq $readSize) {
                    $reuseChunk = $true
                    Write-Host ("[{0}] Reusing existing chunk {1} at {2}" -f $chunkIndex, $partInfo.Name, (Format-Hex32 $offset))
                } else {
                    Write-Host ("[{0}] Existing chunk size mismatch at {1}; rereading" -f $chunkIndex, (Format-Hex32 $offset))
                    Remove-Item -LiteralPath $partFile -Force
                    if (Test-Path $partLog) {
                        Remove-Item -LiteralPath $partLog -Force
                    }
                }
            }

            if (-not $reuseChunk) {
                Write-Host ("[{0}] Reading {1} bytes at {2}" -f $chunkIndex, $readSize, (Format-Hex32 $offset))
                Invoke-EsptoolReadFlash -Offset $offset -Size $readSize -TargetFile $partFile -LogFile $partLog

                $partInfo = Get-Item $partFile
                if ($partInfo.Length -ne $readSize) {
                    throw "Chunk size mismatch at offset $(Format-Hex32 $offset): expected $readSize got $($partInfo.Length)"
                }
            }

            $offset += $readSize
            $chunkIndex++
        }

        Write-Host "Merging chunks into $outputFile"
        $outStream = [System.IO.File]::Open($outputFile, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
        try {
            Get-ChildItem $chunkDir -Filter "*.bin" | Sort-Object Name | ForEach-Object {
                $bytes = [System.IO.File]::ReadAllBytes($_.FullName)
                $outStream.Write($bytes, 0, $bytes.Length)
            }
        }
        finally {
            $outStream.Dispose()
        }

        $summary = @(
            "mode=$Mode"
            "baud=$Baud"
            "flash_size=$(Format-Hex32 $FlashSizeBytes)"
            "chunk_size=$(Format-Hex32 $ChunkSizeBytes)"
            "chunk_count=$chunkIndex"
            "image=$outputFile"
            "parts=$chunkDir"
        )
        $summary | Set-Content -Path $logFile
    }
}
catch {
    Write-Error $_
    Write-Host ""
    Write-Host "Backup failed. Common reasons:"
    Write-Host "1. Wrong serial port."
    Write-Host "2. Board not in a readable boot mode."
    Write-Host "3. Flash encryption or read protection is enabled."
    Write-Host "4. Flash size or chunk size is not suitable for this board."
    Write-Host "5. USB connection is unstable during long reads."
    exit 1
}

$fileInfo = Get-Item $outputFile
if ($fileInfo.Length -ne $FlashSizeBytes) {
    throw "Final image size mismatch: expected $FlashSizeBytes got $($fileInfo.Length)"
}

Write-Host ""
Write-Host "Backup complete."
Write-Host "Image: $($fileInfo.FullName)"
Write-Host "Size : $($fileInfo.Length) bytes"
Write-Host "Log  : $logFile"
if ($Mode -eq "chunked") {
    Write-Host "Parts: $chunkDir"
}
Write-Host ""
Write-Host "Restore command example:"
Write-Host "python -m esptool --chip $Chip --port $Port --baud $Baud write_flash 0x0 `"$outputFile`""
