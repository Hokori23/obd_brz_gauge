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

if ($headerText -notmatch 'int16_t ui_font_pick_typoder_for_box\(const char \*text,\s*lv_coord_t width,\s*lv_coord_t height,\s*lv_coord_t padding\);') {
    throw "ui_font_profile.h must expose a reusable printLabel-style font picker based only on text/box/padding"
}

if ($headerText -notmatch 'const lv_font_t \*ui_font_typoder_exact\(int16_t font_px\);') {
    throw "ui_font_profile.h must expose an exact Typoder accessor for resolved metric layout fonts"
}

if ($headerText -notmatch 'bool ui_font_pick_typoder_fit_for_box\(const char \*text,\s*lv_coord_t width,\s*lv_coord_t height,\s*lv_coord_t padding,\s*ui_font_fit_t \*fit_out\);') {
    throw "ui_font_profile.h must expose box-fit results so layout code does not reinterpret resolved font sizes"
}

if ($headerText -notmatch 'lv_coord_t ui_font_measure_text_width\(const lv_font_t \*font, const char \*text\);') {
    throw "ui_font_profile.h must expose reusable text measurement for layout anchors"
}

if ($headerText -notmatch 'const int16_t \*ui_font_typoder_candidate_sizes\(uint8_t \*count_out\);') {
    throw "ui_font_profile.h must expose one reusable Typoder candidate list instead of scattering font-size arrays"
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

if ($sourceText -notmatch 'ui_font_pick_typoder_for_box' -or
    $sourceText -notmatch 'width > \(padding \* 2\)' -or
    $sourceText -notmatch 'height > \(padding \* 2\)') {
    throw "ui_font_profile.c must implement box-based font picking without dashboard slot-count knowledge"
}

if ($sourceText -match 'ui_font_text_fits_box\(ui_font_typoder\(candidate\)' -or
    $sourceText -match 'ui_font_measure_text_box\(ui_font_typoder\(candidate\)') {
    throw "Typoder box picking must measure exact candidate fonts, not re-scale candidate sizes through ui_font_typoder()"
}

if ($sourceText -notmatch 'ui_font_typoder_candidate_sizes' -or
    $sourceText -notmatch 'const int16_t k_candidates\[\]' -or
    $sourceText -match '128, 120, 112|72, 68, 64') {
    throw "Typoder box picking must reuse a single candidate list that only contains real wired Typoder font sizes"
}

if ($sourceText -notmatch 's_typoder_40' -or
    $sourceText -notmatch 'ui_font_FontTypoderSize40') {
    throw "ui_font_profile.c must wire the available Typoder 40 font before exposing it as a candidate"
}

if ($uiText -notmatch 'ui_font_profile_init\(\);') {
    throw "ui.c must initialize font fallbacks before creating screens"
}

Write-Output "ui font profile contract checks passed"
