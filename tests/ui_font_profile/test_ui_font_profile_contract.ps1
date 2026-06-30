$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$fontProfileHeader = Join-Path $repoRoot "main\export_path\ui_font_profile.h"
$fontProfileSource = Join-Path $repoRoot "main\export_path\ui_font_profile.c"
$uiSource = Join-Path $repoRoot "main\export_path\ui.c"

$headerText = Get-Content $fontProfileHeader -Raw
$sourceText = Get-Content $fontProfileSource -Raw
$uiText = Get-Content $uiSource -Raw

if ($headerText -notmatch 'void ui_font_profile_init\(void\);') {
    throw "ui_font_profile.h must expose ui_font_profile_init so UI startup can prepare font fallbacks"
}

if ($sourceText -notmatch 'ui_font_set_fallback') {
    throw "ui_font_profile.c must centralize fallback wiring for custom fonts"
}

if ($sourceText -notmatch 'ui_font_FontTypoderSize16') {
    throw "ui_font_profile.c must wire Typoder fonts to a fallback glyph source"
}

if ($sourceText -notmatch 'ui_font_FontBabySize200') {
    throw "ui_font_profile.c must wire Baby fonts to a fallback glyph source"
}

if ($sourceText -notmatch 'ui_font_FontTaikongSize128') {
    throw "ui_font_profile.c must wire Taikong fonts to a fallback glyph source"
}

if ($sourceText -notmatch 'lv_font_montserrat_48') {
    throw "ui_font_profile.c must use an LVGL built-in fallback font that includes degree glyphs"
}

if ($uiText -notmatch 'ui_font_profile_init\(\);') {
    throw "ui.c must initialize font fallbacks before creating screens"
}

Write-Output "ui font profile contract checks passed"
