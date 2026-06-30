param(
    [switch]$KeepIfBusy
)

$ErrorActionPreference = "Stop"

$buildDir = Join-Path $PSScriptRoot "..\build"

function Clear-ReadOnlyTree {
    param(
        [Parameter(Mandatory = $true)]
        [string]$TargetDir
    )

    if (-not (Test-Path $TargetDir)) {
        return
    }

    Get-ChildItem -LiteralPath $TargetDir -Force -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
        try {
            $_.Attributes = $_.Attributes -band (-bnot [System.IO.FileAttributes]::ReadOnly)
        } catch {
            # Best effort only; actual delete will surface any remaining blockers.
        }
    }
}

if (-not (Test-Path $buildDir)) {
    Write-Host "Default build directory does not exist: $buildDir"
    exit 0
}

try {
    Clear-ReadOnlyTree -TargetDir $buildDir
    Remove-Item -LiteralPath $buildDir -Recurse -Force
    Write-Host "Removed default build directory: $buildDir"
    exit 0
} catch {
    Write-Host "Failed to remove default build directory: $buildDir"
    Write-Host $_.Exception.Message
    Write-Host ""
    Write-Host "Common cause: a running Ninja, VS Code ESP-IDF task, monitor, or indexer still holds files like .ninja_deps."
    Write-Host "Another common cause: read-only files under managed component Git mirrors inside build, such as _upstream_waveshare_components\\.git\\objects\\pack."
    Write-Host "Recommended order:"
    Write-Host "1. Stop Build/Monitor tasks in VS Code."
    Write-Host "2. Close any terminal running idf.py, ninja, or monitor."
    Write-Host "3. Re-run this script so it can clear read-only attributes and retry deletion."
    if ($KeepIfBusy) {
        exit 0
    }
    exit 1
}
