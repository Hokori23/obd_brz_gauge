$scriptPath = Join-Path $PSScriptRoot "..\..\tools\clean_default_build.ps1"
$content = Get-Content $scriptPath -Raw

if ($content -notmatch '\.\.\\build') {
    throw "clean_default_build.ps1 must target the repository default build directory"
}

if ($content -notmatch 'Remove-Item -LiteralPath \$buildDir -Recurse -Force') {
    throw "clean_default_build.ps1 must recursively remove the default build directory"
}

if ($content -notmatch 'function Clear-ReadOnlyTree') {
    throw "clean_default_build.ps1 must proactively clear read-only attributes in the build tree"
}

if ($content -notmatch '\.ninja_deps') {
    throw "clean_default_build.ps1 must explain the common Windows file-lock failure mode"
}

if ($content -notmatch '_upstream_waveshare_components\\\\\.git\\\\objects\\\\pack') {
    throw "clean_default_build.ps1 must document the managed-component git pack attribute failure mode"
}

if ($content -notmatch 'Stop Build/Monitor tasks in VS Code') {
    throw "clean_default_build.ps1 must document the recommended VS Code recovery flow"
}

Write-Output "clean_default_build script test passed"
