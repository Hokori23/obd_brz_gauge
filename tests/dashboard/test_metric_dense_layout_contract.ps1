$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

$widgetsPath = Join-Path $repoRoot "main\export_path\ui_home_runtime_widgets.c"
$widgetsHeaderPath = Join-Path $repoRoot "main\export_path\ui_home_runtime_widgets.h"
$homeRuntimePath = Join-Path $repoRoot "main\export_path\ui_home_runtime.c"

$widgets = Get-Content $widgetsPath -Raw
$widgetsHeader = Get-Content $widgetsHeaderPath -Raw
$homeRuntime = Get-Content $homeRuntimePath -Raw

if ($widgetsHeader -notmatch 'typedef struct\s*\{(?s).*?ui_home_metric_slot_style_t;') {
    throw "metric dense layout must expose a concrete slot style object instead of passing ad-hoc center/font values"
}

if ($widgets -notmatch 'bool ui_home_runtime_widgets_is_dense_slot_count\(uint8_t slot_count\)\s*\{\s*return ui_home_widgets_use_metric_block_layout\(slot_count\);') {
    throw "metric widgets must expose the dense slot-count predicate through a public helper"
}

if ($widgets -match 'ui_home_widgets_use_centered_metric_layout') {
    throw "3/4/5/6 metric pages must use metric-block semantics, not the old centered dense layout"
}

if ($widgets -notmatch 'return slot_count >= 3u && slot_count <= 6u;') {
    throw "dense metric layout must be limited to 3/4/5/6 slots so 1/2 slot box layout stays isolated"
}

if ($widgets -notmatch 'ui_font_pick_typoder_fit_for_box\(ui_home_widgets_value_sample_text\(item\),') {
    throw "metric widgets must use the shared printLabel-style font picker instead of private slot-count sizing"
}

if ($widgets -notmatch 'ui_font_pick_typoder_fit_for_box\(ui_home_widgets_value_sample_text\(item\),' -or
    $widgets -notmatch 'ui_font_typoder_exact\(style->value_font\)' -or
    $widgets -notmatch 'ui_font_typoder_exact\(value_font\)') {
    throw "metric value fonts must use exact resolved Typoder sizes for measurement and LVGL binding"
}

if ($widgets -notmatch 'ui_font_typoder_candidate_sizes' -and $widgets -match 'static const int16_t k_candidates') {
    throw "dashboard widgets must not own Typoder candidate font arrays; font profile is the single source"
}

if ($widgets -match 'max_font\s*=|slot_count >= 5u\)\s*\{\s*max_font|ui_home_widgets_slot_value_font|ui_home_widgets_value_height_limit|ui_home_widgets_value_vertical_safety_pad') {
    throw "metric value font sizing must not keep slot-count max-font caps"
}

if ($widgets -notmatch 'ui_home_widgets_metric_content_rect' -or
    $widgets -notmatch 'content_rect = ui_home_widgets_metric_content_rect\(x, y, w, h, slot_count, allow_value_span_expand\)' -or
    $widgets -notmatch 'style->unit_y = \(lv_coord_t\)\(style->value_y \+ value_fit.text_h \+ unit_gap\)') {
    throw "3/4/5/6 metric widgets must build label/value/unit as one metric block with unit close below value"
}

$metricContentRect = [regex]::Match($widgets, '(?s)static ui_home_metric_rect_t ui_home_widgets_metric_content_rect\(.*?\n\}')
if (-not $metricContentRect.Success -or
    $metricContentRect.Value -match 'ui_round_shell_safe_span_for_band') {
    throw "metric block content width must not be constrained by the whole slot height's circular safe span"
}

if ($widgets -notmatch 'ui_home_widgets_metric_value_fit_padding' -or
    $widgets -notmatch 'if \(slot_count >= 6u\)\s*\{\s*return ui_layout_px\(6\);' -or
    $widgets -notmatch 'if \(slot_count == 5u\)\s*\{\s*return ui_layout_px\(5\);') {
    throw "5/6-slot metric value fitting must keep a small glyph-edge padding guard without shrinking single rows"
}

if ($widgets -notmatch 'static lv_coord_t ui_home_widgets_metric_block_pad_y' -or
    $widgets -notmatch 'return LV_MAX\(ui_layout_px\(2\), slot_h \* 3 / 100\);' -or
    $widgets -notmatch 'static lv_coord_t ui_home_widgets_metric_block_pad_x' -or
    $widgets -notmatch 'return ui_layout_px\(4\);') {
    throw "dense metric blocks must use a small shared content inset so corner labels are not clipped"
}

if ($widgets -notmatch 'ui_home_widgets_metric_resolve_gaps' -or
    $widgets -notmatch 'value_text_h / 12' -or
    $widgets -notmatch 'value_text_h / 18' -or
    $widgets -notmatch 'ui_home_widgets_metric_resolve_gaps\(content_rect\.h,\s*slot_count,\s*value_fit\.text_h,' -or
    $widgets -match 'single_slot_row') {
    throw "dense metric vertical gaps must scale from resolved value font height, not slot-specific single-row branches"
}

if ($widgets -notmatch 'ui_home_widgets_metric_value_inset_x\(uint8_t slot_count, bool allow_value_span_expand\)' -or
    $widgets -notmatch 'if \(slot_count == 3u && allow_value_span_expand\)\s*\{\s*return ui_layout_px\(10\);' -or
    $widgets -notmatch 'if \(slot_count >= 5u\)\s*\{\s*return ui_layout_px\([0-9]+\);' -or
    $widgets -notmatch 'value_rect\.x = \(lv_coord_t\)\(value_rect\.x \+ inset_x\)' -or
    $widgets -notmatch 'value_rect\.w = \(lv_coord_t\)\(value_rect\.w - \(inset_x \* 2\)\)') {
    throw "dense metric value rect must support dedicated horizontal insets before font fitting"
}

if ($widgets -match 'ui_home_widgets_metric_label_value_gap\(' -or
    $widgets -match 'ui_home_widgets_metric_value_unit_gap\(') {
    throw "dense metric gap callers must use the floor helpers or resolved-gap helper; old gap names must not remain"
}

if ($widgets -notmatch 'ui_home_runtime_widgets_create_box_slot_card\(parent') {
    throw "metric slot creation must keep an explicit box slot-card path for 1/2 slots"
}

if ($widgets -notmatch 'axis_aligned_metadata = \(slot_count <= 2u\)' -or
    $widgets -notmatch 'axis_aligned_metadata \? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT' -or
    $widgets -notmatch 'axis_aligned_metadata \? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_RIGHT') {
    throw "1/2-slot box cards must align label and unit on the same axis as value"
}

if ($widgets -notmatch '(?s)if \(slot_count <= 2u\)\s*\{.*?lv_obj_set_x\(name_label,\s*\(lv_coord_t\)\(value_center_x - \(value_width / 2\)\)\).*?lv_obj_set_x\(unit_label,\s*\(lv_coord_t\)\(value_center_x - \(value_width / 2\)\)\).*?LV_TEXT_ALIGN_CENTER') {
    throw "1/2-slot typography refresh must preserve label/unit/value axis alignment"
}

if ($widgets -notmatch 'axis_unit_gap_y = \(slot_count == 2u\) \? ui_layout_px\(5\) : ui_layout_px\(10\)' -or
    $widgets -notmatch 'value_label_y \+ value_line_h \+ axis_unit_gap_y' -or
    $widgets -notmatch 'lv_obj_set_y\(unit_label,\s*\(lv_coord_t\)\(lv_obj_get_y\(value_label\) \+ value_line_h \+ axis_unit_gap_y\)\)') {
    throw "1/2-slot unit labels must keep label/value fixed while letting the 2-slot unit sit 5 px below the value"
}

if ($widgets -match 'value_host|create_legacy_slot_card') {
    throw "metric slot layout must not reintroduce the old value_host or legacy slot-card escape hatch"
}

$denseCreate = [regex]::Match($widgets, '(?s)static void ui_home_runtime_widgets_create_dense_slot_card\(.*?\n\}')
if (-not $denseCreate.Success) {
    throw "metric widgets must keep a dedicated dense slot-card creation function"
}

if ($denseCreate.Value -notmatch 'const ui_home_metric_slot_style_t \*dense_style') {
    throw "dense slot-card creation must consume a precomputed dense style"
}

if ($denseCreate.Value -match 'ui_home_widgets_position_metric_block') {
    throw "dense slot-card creation must not recompute block positioning after receiving a style"
}

if ($widgets -match 'ui_home_widgets_position_metric_block') {
    throw "dense metric layout must not keep the old ad-hoc positioning helper as a second layout path"
}

if ($widgets -notmatch 'if \(ui_home_widgets_use_metric_block_layout\(slot_count\)\)\s*\{\s*return;\s*\}') {
    throw "box typography apply must explicitly refuse dense slot counts"
}

if ($widgets -notmatch 'bool allow_value_span_expand' -or
    $widgets -notmatch 'ui_home_widgets_metric_content_rect\(x, y, w, h, slot_count, allow_value_span_expand\)') {
    throw "dense metric layout must let the layout layer decide value-band expansion and must not blindly trim the center divider"
}

if ($widgets -notmatch 'style->value_text_w' -or
    $widgets -notmatch 'min_center = style->value_rect_x \+ \(\(style->value_text_w \+ ui_layout_px\(10\)\) / 2\)') {
    throw "dense column anchors must clamp by measured value text width, not by the full label container width"
}

if ($homeRuntime -notmatch 'static void ui_home_build_dense_metric_styles\(') {
    throw "home runtime must compute dense metric styles once per metric page"
}

if ($homeRuntime -notmatch 'ui_home_metric_layout_style_t metric_styles;' -or
    $homeRuntime -notmatch '&rt->metric_styles' -or
    $homeRuntime -match '(?s)static void ui_home_refresh_metric_tile\(.*?ui_home_metric_layout_style_t dense_styles') {
    throw "home runtime must cache resolved metric styles on the tile runtime instead of rebuilding a local style set every refresh"
}

if ($homeRuntime -notmatch 'ui_home_apply_metric_column_anchor' -or
    $homeRuntime -notmatch 'if \(slot_count != 3u\)') {
    throw "dense metric layout must apply column anchors for multi-row double columns while keeping 3-slot independent"
}

if ($homeRuntime -notmatch 'ui_home_metric_column_preferred_center' -or
    $homeRuntime -match 'ui_center_x\(\) [-+] ui_layout_px\(8\)') {
    throw "dense metric column anchors must prefer the actual column centers, not a near-center fallback"
}

if ($homeRuntime -notmatch 'if \(apply_row_nudges && slot_count == 4u && row_count == 2u\)' -or
    $homeRuntime -notmatch 'row_y = \(lv_coord_t\)\(row_y \+ \(\(row == 0u\) \? ui_layout_px\([0-9]+\) : -ui_layout_px\([0-9]+\)\)\);') {
    throw "4-slot metric pages must support explicit row-level Y nudges toward the center"
}

if ($homeRuntime -notmatch 'if \(apply_row_nudges && slot_count == 3u && row_count == 2u && row == 1u\)' -or
    $homeRuntime -notmatch 'row_y = \(lv_coord_t\)\(row_y - ui_layout_px\(5\)\);') {
    throw "3-slot metric pages must support a bottom-row Y nudge toward the center"
}

if ($homeRuntime -notmatch 'dense_metric_layout = ui_home_runtime_widgets_is_dense_slot_count\(slot_count\)' -or
    $homeRuntime -notmatch '(?s)if \(dense_metric_layout\)\s*\{\s*span_left = grid_left;\s*span_right = grid_right;\s*\}\s*else\s*\{\s*ui_round_shell_safe_span_for_band') {
    throw "dense metric raw slots must use a stable inner grid instead of whole-row circular safe-span clipping"
}

if ($homeRuntime -notmatch 'int16_t row_value_font = LV_MIN\(styles->slots\[slot_index\]\.value_font,' -or
    $homeRuntime -notmatch '(?s)visible_items\[slot_index\],\s*row_value_font,\s*false,\s*&styles->slots\[slot_index\]' -or
    $homeRuntime -notmatch '(?s)visible_items\[slot_index \+ 1u\],\s*row_value_font,\s*false,\s*&styles->slots\[slot_index \+ 1u\]') {
    throw "each paired metric row must use one shared value font while single-slot rows keep independent sizing"
}

if ($homeRuntime -notmatch 'slot_count == 5u && row_count == 3u' -or
    $homeRuntime -notmatch 'row_weights\[2\] = 116u' -or
    $homeRuntime -notmatch 'slot_count == 3u && row_count == 2u' -or
    $homeRuntime -notmatch 'row_weights\[1\] = 106u') {
    throw "3/5-slot metric pages must give their bottom single block independent vertical weight"
}

if ($homeRuntime -notmatch '(?s)row_slot_counts\[row\] == 1u.*?visible_items\[slot_index\],\s*0,\s*true,\s*&styles->slots\[slot_index\]' -or
    $homeRuntime -notmatch '(?s)visible_items\[slot_index\],\s*0,\s*false,\s*&styles->slots\[slot_index\]' -or
    $homeRuntime -notmatch '(?s)visible_items\[slot_index \+ 1u\],\s*0,\s*false,\s*&styles->slots\[slot_index \+ 1u\]') {
    throw "dense metric layout must expand single-slot rows while keeping paired slots panel-local before column anchoring"
}

if ($homeRuntime -notmatch 'ui_home_runtime_widgets_apply_dense_slot_style') {
    throw "home runtime must apply dense metric styles through the dense-specific path"
}

Write-Output "metric dense layout contract checks passed"
