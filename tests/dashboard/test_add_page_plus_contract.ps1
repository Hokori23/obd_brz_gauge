$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"
$homeRuntime = Get-Content $homeRuntimePath -Raw

if ($homeRuntime -notmatch 'plus_wrap') {
    throw "ui_home_runtime.c must keep a dedicated geometric plus wrapper for the ADD page"
}

if ($homeRuntime -notmatch 'plus_h' -or $homeRuntime -notmatch 'plus_v') {
    throw "ui_home_runtime.c must build the ADD page plus sign from centered horizontal and vertical bars"
}

if ($homeRuntime -match 'lv_label_set_text\(plus,\s*"\+"\)') {
    throw "ui_home_runtime.c must not rely on a font glyph for the ADD page plus sign alignment"
}

Write-Output "ADD page plus contract checks passed"
