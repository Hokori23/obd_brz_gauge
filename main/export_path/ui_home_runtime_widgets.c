#include "ui_home_runtime_widgets.h"

#include "ui_font_profile.h"
#include "ui_layout.h"
#include "ui_round_shell.h"

#include <string.h>

typedef struct {
    lv_coord_t x;
    lv_coord_t y;
    lv_coord_t w;
    lv_coord_t h;
} ui_home_metric_rect_t;

static int16_t ui_home_widgets_slot_name_font(uint8_t slot_count);
static int16_t ui_home_widgets_slot_unit_font(uint8_t slot_count);
static lv_coord_t ui_home_widgets_metric_label_value_gap_floor(lv_coord_t slot_h, uint8_t slot_count);
static lv_coord_t ui_home_widgets_metric_value_unit_gap_floor(lv_coord_t slot_h, uint8_t slot_count);

/** 返回某个仪表项用于估算字号的样本文本。 */
static const char *ui_home_widgets_value_sample_text(disp_item_t item)
{
    switch (item) {
    case DISP_ITEM_RPM:
        return "9999";
    case DISP_ITEM_SPEED:
        return "999";
    case DISP_ITEM_BAT:
        return "99.9";
    case DISP_ITEM_OILP:
        return "99.9";
    case DISP_ITEM_BOOST:
        return "-99.9";
    case DISP_ITEM_LOAD:
    case DISP_ITEM_TPS:
        return "100";
    case DISP_ITEM_BKT:
        return "999";
    case DISP_ITEM_CLT:
    case DISP_ITEM_IAT:
    case DISP_ITEM_OIL:
    default:
        return "999";
    }
}

/** 根据槽位高度和数量推算数值文本可占用的高度上限。 */
/** 判断某个字体在给定宽高约束内能否放下样本文本。 */
static lv_coord_t ui_home_widgets_font_line_height(int16_t font_size, lv_coord_t fallback)
{
    const lv_font_t *font = ui_font_typoder(font_size);

    return (font != NULL) ? (lv_coord_t)font->line_height : fallback;
}

static lv_coord_t ui_home_widgets_exact_font_line_height(int16_t font_size, lv_coord_t fallback)
{
    const lv_font_t *font = ui_font_typoder_exact(font_size);

    return (font != NULL) ? (lv_coord_t)font->line_height : fallback;
}

/** 计算圆屏裁切下某个面板左侧需要额外避开的安全边距。 */
static lv_coord_t ui_home_widgets_circular_safe_left(lv_coord_t panel_x, lv_coord_t panel_y)
{
    lv_coord_t circle_left = ui_round_shell_circle_left_at_y(panel_y);
    lv_coord_t margin = ui_layout_px(1);

    if (circle_left + margin > panel_x) {
        return (circle_left + margin) - panel_x;
    }
    return 0;
}

/** 计算圆屏裁切下某个面板右侧需要额外避开的安全边距。 */
static lv_coord_t ui_home_widgets_circular_safe_right(lv_coord_t panel_right, lv_coord_t panel_y)
{
    lv_coord_t circle_right = ui_round_shell_circle_right_at_y(panel_y);
    lv_coord_t margin = ui_layout_px(1);

    if (circle_right - margin < panel_right) {
        return panel_right - (circle_right - margin);
    }
    return 0;
}

static void ui_home_widgets_trim_center_divider(ui_home_metric_rect_t *rect, lv_coord_t center_x, lv_coord_t gap)
{
    lv_coord_t rect_right;

    if (rect == NULL || rect->w <= 0) {
        return;
    }

    rect_right = rect->x + rect->w;
    if (rect_right <= center_x) {
        lv_coord_t right = LV_MIN(rect_right, center_x - gap);
        rect->w = LV_MAX(right - rect->x, ui_layout_px(24));
    } else if (rect->x >= center_x) {
        lv_coord_t left = LV_MAX(rect->x, center_x + gap);
        rect->w = LV_MAX(rect_right - left, ui_layout_px(24));
        rect->x = left;
    }
}

static ui_home_metric_rect_t ui_home_widgets_value_rect_for_panel(lv_coord_t panel_x,
                                                                  lv_coord_t panel_y,
                                                                  lv_coord_t panel_w,
                                                                  lv_coord_t panel_h,
                                                                  uint8_t slot_count,
                                                                  bool allow_safe_span_expand,
                                                                  bool avoid_center_line)
{
    lv_coord_t inset_x = ui_layout_px(4);
    lv_coord_t stack_pad_y = ui_layout_px((slot_count <= 2u) ? 8 : 5);
    lv_coord_t label_gap;
    lv_coord_t unit_gap;
    lv_coord_t name_h;
    lv_coord_t unit_h;
    lv_coord_t value_y;
    lv_coord_t value_h;
    lv_coord_t span_left;
    lv_coord_t span_right;
    lv_coord_t rect_left;
    lv_coord_t rect_right;
    ui_home_metric_rect_t rect;

    if (panel_w <= 0 || panel_h <= 0) {
        return (ui_home_metric_rect_t){panel_x, panel_y, 0, 0};
    }

    if ((inset_x * 2) >= panel_w) {
        inset_x = 0;
    }

    label_gap = ui_home_widgets_metric_label_value_gap_floor(panel_h, slot_count);
    unit_gap = ui_home_widgets_metric_value_unit_gap_floor(panel_h, slot_count);
    name_h = ui_home_widgets_font_line_height(ui_home_widgets_slot_name_font(slot_count), ui_layout_px(20));
    unit_h = ui_home_widgets_font_line_height(ui_home_widgets_slot_unit_font(slot_count), ui_layout_px(16));
    value_h = panel_h - name_h - unit_h - label_gap - unit_gap - (stack_pad_y * 2);
    if (value_h < ui_layout_px(20)) {
        value_h = ui_layout_px(20);
    }
    value_y = panel_y + stack_pad_y + name_h + label_gap;
    if ((value_y + value_h + unit_gap + unit_h + stack_pad_y) > (panel_y + panel_h)) {
        value_y = panel_y + LV_MAX((panel_h - value_h) / 2, 0);
    }

    ui_round_shell_safe_span_for_band(value_y,
                                      value_h,
                                      ui_safe_margin() + ui_layout_px(1),
                                      &span_left,
                                      &span_right);
    if (allow_safe_span_expand) {
        rect_left = span_left + inset_x;
        rect_right = span_right - inset_x;
    } else {
        rect_left = LV_MAX(panel_x + inset_x, span_left);
        rect_right = LV_MIN(panel_x + panel_w - inset_x, span_right);
    }
    if (rect_right <= rect_left) {
        rect_left = panel_x + inset_x;
        rect_right = panel_x + panel_w - inset_x;
    }

    rect = (ui_home_metric_rect_t){
        rect_left,
        value_y,
        LV_MAX(rect_right - rect_left, ui_layout_px(24)),
        value_h
    };
    if (avoid_center_line) {
        ui_home_widgets_trim_center_divider(&rect, ui_center_x(), ui_layout_px(5));
    }
    return rect;
}

/**
 * 计算数值文本可安全使用的宽度
 *
 * 核心职责：结合圆屏边缘和槽位数量，为中间大数字预留不被裁切的宽度
 */
static lv_coord_t ui_home_widgets_value_safe_width(lv_coord_t panel_x,
                                                   lv_coord_t panel_y,
                                                   lv_coord_t panel_w,
                                                   lv_coord_t panel_h,
                                                   lv_coord_t value_offset_y,
                                                   uint8_t slot_count)
{
    lv_coord_t edge_pad;

    if (panel_w <= 0 || panel_h <= 0) {
        return 0;
    }

    LV_UNUSED(panel_x);
    LV_UNUSED(panel_y);
    LV_UNUSED(value_offset_y);

    edge_pad = (slot_count <= 2u) ? ui_layout_px(2) : ui_layout_px(1);
    if (panel_w <= (edge_pad * 2)) {
        return ui_layout_px(56);
    }

    return LV_MAX(panel_w - (edge_pad * 2), ui_layout_px(56));
}

/**
 * 计算数值标签宿主容器的横向边界
 *
 * 核心职责：让数值标签尽量待在圆屏安全带里，并避开中轴附近的拥挤区域
 */
static void ui_home_widgets_value_band_bounds(lv_coord_t panel_x,
                                              lv_coord_t panel_y,
                                              lv_coord_t panel_w,
                                              lv_coord_t panel_h,
                                              lv_coord_t value_offset_y,
                                              lv_coord_t *band_x_out,
                                              lv_coord_t *band_w_out)
{
    lv_coord_t band_h = panel_h * 30 / 100;
    lv_coord_t band_y;
    lv_coord_t span_left;
    lv_coord_t span_right;
    lv_coord_t band_left;
    lv_coord_t band_right;
    lv_coord_t center_x = ui_center_x();
    lv_coord_t mid_gap = ui_layout_px(4);
    lv_coord_t panel_right = panel_x + panel_w;

    if (band_x_out != NULL) {
        *band_x_out = panel_x;
    }
    if (band_w_out != NULL) {
        *band_w_out = panel_w;
    }
    if (panel_w <= 0 || panel_h <= 0) {
        return;
    }

    if (band_h < ui_layout_px(28)) {
        band_h = ui_layout_px(28);
    }
    band_y = panel_y + ((panel_h - band_h) / 2) + value_offset_y;
    ui_round_shell_safe_span_for_band(band_y,
                                      band_h,
                                      ui_safe_margin() + ui_layout_px(1),
                                      &span_left,
                                      &span_right);

    band_left = span_left;
    band_right = span_right;
    if (panel_right <= center_x + ui_layout_px(4)) {
        band_right = center_x - mid_gap;
    } else if (panel_x >= center_x - ui_layout_px(4)) {
        band_left = center_x + mid_gap;
    }

    if (band_right <= band_left) {
        band_left = panel_x;
        band_right = panel_right;
    }

    if (band_x_out != NULL) {
        *band_x_out = band_left;
    }
    if (band_w_out != NULL) {
        *band_w_out = LV_MAX(band_right - band_left, ui_layout_px(56));
    }
}

/** 根据槽位数量返回标题标签建议字号。 */
static int16_t ui_home_widgets_slot_name_font(uint8_t slot_count)
{
    if (slot_count == 3u) {
        return 18;
    }
    if (slot_count == 4u) {
        return 18;
    }
    if (slot_count == 5u) {
        return 18;
    }
    if (slot_count == 6u) {
        return 16;
    }
    if (slot_count == 1u) {
        return 28;
    }
    if (slot_count <= 2u) {
        return 22;
    }
    if (slot_count <= 4u) {
        return 18;
    }
    return 16;
}

/** 根据槽位数量返回单位标签建议字号。 */
static int16_t ui_home_widgets_slot_unit_font(uint8_t slot_count)
{
    if (slot_count == 3u) {
        return 17;
    }
    if (slot_count == 4u) {
        return 14;
    }
    if (slot_count == 5u) {
        return 14;
    }
    if (slot_count == 6u) {
        return 12;
    }
    if (slot_count == 1u) {
        return 24;
    }
    if (slot_count <= 2u) {
        return 22;
    }
    return 16;
}

/**
 * 为数值标签选择最合适的字号
 *
 * 核心职责：从候选字号里挑出既够大又不会超出槽位可视区域的结果
 */
static int16_t ui_home_widgets_value_font_for_rect(disp_item_t item,
                                                   ui_home_metric_rect_t value_rect)
{
    ui_font_fit_t fit = {0};

    ui_font_pick_typoder_fit_for_box(ui_home_widgets_value_sample_text(item),
                                     value_rect.w,
                                     value_rect.h,
                                     ui_layout_px(6),
                                     &fit);
    return (fit.font_size > 0) ? fit.font_size : 16;
}

static bool ui_home_widgets_use_metric_block_layout(uint8_t slot_count)
{
    // 3/4/5/6 slots use the dense metric template; 1/2 keep the separate box-card path.
    return slot_count >= 3u && slot_count <= 6u;
}

bool ui_home_runtime_widgets_is_dense_slot_count(uint8_t slot_count)
{
    return ui_home_widgets_use_metric_block_layout(slot_count);
}

static lv_coord_t ui_home_widgets_metric_block_pad_y(lv_coord_t slot_h, uint8_t slot_count)
{
    LV_UNUSED(slot_count);

    // Shared top/bottom inset for dense metric blocks. This trims edge clipping only.
    return LV_MAX(ui_layout_px(2), slot_h * 3 / 100);
}

static lv_coord_t ui_home_widgets_metric_block_pad_x(uint8_t slot_count)
{
    LV_UNUSED(slot_count);

    // Shared left/right inset for dense metric blocks.
    return ui_layout_px(4);
}

static lv_coord_t ui_home_widgets_metric_label_value_gap_floor(lv_coord_t slot_h, uint8_t slot_count)
{
    if (slot_count >= 6u) {
        return LV_MAX(ui_layout_px(2), slot_h * 2 / 100);
    }
    return LV_MAX(ui_layout_px(3), slot_h * 3 / 100);
}

static lv_coord_t ui_home_widgets_metric_value_unit_gap_floor(lv_coord_t slot_h, uint8_t slot_count)
{
    return LV_MAX(ui_layout_px(1), slot_h / ((slot_count >= 6u) ? 100 : 80));
}

static void ui_home_widgets_metric_resolve_gaps(lv_coord_t slot_h,
                                                uint8_t slot_count,
                                                lv_coord_t value_text_h,
                                                lv_coord_t *label_gap_out,
                                                lv_coord_t *unit_gap_out)
{
    lv_coord_t label_gap = ui_home_widgets_metric_label_value_gap_floor(slot_h, slot_count);
    lv_coord_t unit_gap = ui_home_widgets_metric_value_unit_gap_floor(slot_h, slot_count);

    // Gap scales with the resolved value font. Wide single rows naturally pick bigger values,
    // so they need more visual air without any slot-specific branch.
    if (value_text_h > 0) {
        label_gap = LV_MAX(label_gap, value_text_h / 12);
        unit_gap = LV_MAX(unit_gap, value_text_h / 18);
    }
    if (label_gap_out != NULL) {
        *label_gap_out = label_gap;
    }
    if (unit_gap_out != NULL) {
        *unit_gap_out = unit_gap;
    }
}

static lv_coord_t ui_home_widgets_metric_value_fit_padding(uint8_t slot_count)
{
    // Dense metric value sizing funnels through ui_font_pick_typoder_fit_for_box().
    // This guards glyph edges only; overall label inset is handled by metric_block_pad_x/y.
    if (slot_count >= 6u) {
        return ui_layout_px(6);
    }
    if (slot_count == 5u) {
        return ui_layout_px(5);
    }
    return ui_layout_px(4);
}

static lv_coord_t ui_home_widgets_metric_value_inset_x(uint8_t slot_count, bool allow_value_span_expand)
{
    // Layout-level value rect inset. Adjust this when a template needs extra side breathing room
    // before font fitting; keep the font helper itself template-agnostic.
    if (slot_count == 3u && allow_value_span_expand) {
        return ui_layout_px(10);
    }
    if (slot_count >= 5u) {
        return ui_layout_px(5);
    }
    return 0;
}

static bool ui_home_widgets_pick_value_fit(disp_item_t item,
                                           ui_home_metric_rect_t value_rect,
                                           uint8_t slot_count,
                                           int16_t forced_value_font,
                                           ui_font_fit_t *fit_out)
{
    const char *sample = ui_home_widgets_value_sample_text(item);

    if (fit_out == NULL) {
        return false;
    }
    memset(fit_out, 0, sizeof(*fit_out));

    if (forced_value_font > 0) {
        const lv_font_t *font = ui_font_typoder_exact(forced_value_font);

        fit_out->font_size = forced_value_font;
        fit_out->font = font;
        fit_out->text_w = ui_font_measure_text_width(font, sample);
        fit_out->text_h = ui_home_widgets_exact_font_line_height(forced_value_font, ui_layout_px(40));
        return true;
    }

    return ui_font_pick_typoder_fit_for_box(sample,
                                           value_rect.w,
                                           value_rect.h,
                                           ui_home_widgets_metric_value_fit_padding(slot_count),
                                           fit_out);
}

static ui_home_metric_rect_t ui_home_widgets_metric_content_rect(lv_coord_t x,
                                                                 lv_coord_t y,
                                                                 lv_coord_t w,
                                                                 lv_coord_t h,
                                                                 uint8_t slot_count,
                                                                 bool allow_safe_span_expand)
{
    lv_coord_t pad_x = ui_home_widgets_metric_block_pad_x(slot_count);
    lv_coord_t pad_y = ui_home_widgets_metric_block_pad_y(h, slot_count);
    lv_coord_t content_y;
    lv_coord_t content_h;
    lv_coord_t left;
    lv_coord_t right;

    if (w <= 0 || h <= 0) {
        return (ui_home_metric_rect_t){x, y, 0, 0};
    }
    LV_UNUSED(allow_safe_span_expand);

    if ((pad_y * 2) >= h) {
        pad_y = 0;
    }
    content_y = y + pad_y;
    content_h = h - (pad_y * 2);
    left = x + pad_x;
    right = x + w - pad_x;
    if (right <= left) {
        left = x + pad_x;
        right = x + w - pad_x;
    }
    if (right <= left) {
        left = x;
        right = x + w;
    }

    return (ui_home_metric_rect_t){left, content_y, LV_MAX(right - left, ui_layout_px(24)), content_h};
}

void ui_home_runtime_widgets_build_dense_slot_style(lv_coord_t x,
                                                   lv_coord_t y,
                                                   lv_coord_t w,
                                                   lv_coord_t h,
                                                   uint8_t slot_count,
                                                   lv_coord_t content_center_x,
                                                   disp_item_t item,
                                                   int16_t forced_value_font,
                                                   bool allow_value_span_expand,
                                                   ui_home_metric_slot_style_t *style)
{
    lv_coord_t panel_right;
    lv_coord_t content_right;
    lv_coord_t name_h;
    lv_coord_t unit_h;
    lv_coord_t label_gap;
    lv_coord_t unit_gap;
    lv_coord_t total_h;
    lv_coord_t start_y;
    const lv_font_t *name_font_ptr;
    const lv_font_t *unit_font_ptr;
    ui_home_metric_rect_t content_rect;
    ui_home_metric_rect_t value_rect;
    ui_font_fit_t value_fit;

    if (style == NULL) {
        return;
    }
    memset(style, 0, sizeof(*style));
    if (!ui_home_widgets_use_metric_block_layout(slot_count) || w <= 0 || h <= 0) {
        return;
    }

    content_rect = ui_home_widgets_metric_content_rect(x, y, w, h, slot_count, allow_value_span_expand);
    name_font_ptr = ui_font_typoder(ui_home_widgets_slot_name_font(slot_count));
    unit_font_ptr = ui_font_typoder(ui_home_widgets_slot_unit_font(slot_count));
    name_h = (name_font_ptr != NULL) ? (lv_coord_t)name_font_ptr->line_height : ui_layout_px(20);
    unit_h = (unit_font_ptr != NULL) ? (lv_coord_t)unit_font_ptr->line_height : ui_layout_px(16);
    ui_home_widgets_metric_resolve_gaps(content_rect.h, slot_count, 0, &label_gap, &unit_gap);

    value_rect = content_rect;
    value_rect.y = content_rect.y + name_h + label_gap;
    value_rect.h = content_rect.h - name_h - unit_h - label_gap - unit_gap;
    {
        lv_coord_t inset_x = ui_home_widgets_metric_value_inset_x(slot_count, allow_value_span_expand);

        if ((inset_x * 2) < value_rect.w) {
            value_rect.x = (lv_coord_t)(value_rect.x + inset_x);
            value_rect.w = (lv_coord_t)(value_rect.w - (inset_x * 2));
        }
    }
    if (value_rect.h < ui_layout_px(20)) {
        value_rect.h = ui_layout_px(20);
    }
    ui_home_widgets_pick_value_fit(item, value_rect, slot_count, forced_value_font, &value_fit);
    ui_home_widgets_metric_resolve_gaps(content_rect.h,
                                        slot_count,
                                        value_fit.text_h,
                                        &label_gap,
                                        &unit_gap);
    value_rect.y = content_rect.y + name_h + label_gap;
    value_rect.h = content_rect.h - name_h - unit_h - label_gap - unit_gap;
    {
        lv_coord_t inset_x = ui_home_widgets_metric_value_inset_x(slot_count, allow_value_span_expand);

        if ((inset_x * 2) < value_rect.w) {
            value_rect.x = (lv_coord_t)(value_rect.x + inset_x);
            value_rect.w = (lv_coord_t)(value_rect.w - (inset_x * 2));
        }
    }
    if (value_rect.h < ui_layout_px(20)) {
        value_rect.h = ui_layout_px(20);
    }
    ui_home_widgets_pick_value_fit(item, value_rect, slot_count, forced_value_font, &value_fit);

    style->valid = true;
    panel_right = x + w;
    content_right = content_rect.x + content_rect.w;
    if (allow_value_span_expand) {
        x = LV_MIN(x, content_rect.x);
        panel_right = LV_MAX(panel_right, content_right);
        w = panel_right - x;
    }

    style->panel_x = x;
    style->panel_y = y;
    style->panel_w = w;
    style->panel_h = h;
    style->value_rect_x = content_rect.x;
    style->value_rect_y = value_rect.y;
    style->value_rect_w = content_rect.w;
    style->value_rect_h = value_rect.h;
    style->name_font = ui_home_widgets_slot_name_font(slot_count);
    style->value_font = value_fit.font_size;
    style->unit_font = ui_home_widgets_slot_unit_font(slot_count);
    style->value_w = content_rect.w;
    style->value_text_w = value_fit.text_w;
    style->name_w = content_rect.w;
    style->unit_w = content_rect.w;
    style->content_center_x = content_center_x;

    total_h = name_h + label_gap + value_fit.text_h + unit_gap + unit_h;
    start_y = content_rect.y - y + ((content_rect.h - total_h) / 2);
    if ((start_y + total_h) > (content_rect.y - y + content_rect.h)) {
        start_y = content_rect.y - y + content_rect.h - total_h;
    }
    if (start_y < (content_rect.y - y)) {
        start_y = content_rect.y - y;
    }

    style->name_y = start_y;
    style->value_y = (lv_coord_t)(start_y + name_h + label_gap);
    style->unit_y = (lv_coord_t)(style->value_y + value_fit.text_h + unit_gap);
}

static void ui_home_runtime_widgets_create_box_slot_card(lv_obj_t *parent,
                                                         lv_coord_t x,
                                                         lv_coord_t y,
                                                         lv_coord_t w,
                                                         lv_coord_t h,
                                                         uint8_t slot_count,
                                                         disp_item_t item,
                                                         lv_obj_t **name_out,
                                                         lv_obj_t **value_out,
                                                         lv_obj_t **unit_out)
{
    bool is_single_slot = (slot_count == 1u);
    lv_coord_t label_inset_x = w * (is_single_slot ? 3 : 4) / 100;
    lv_coord_t label_inset_y = is_single_slot ? h * 2 / 100 : h * 3 / 100;
    lv_coord_t unit_inset_x = label_inset_x;
    lv_coord_t unit_inset_y = h * 2 / 100;
    lv_coord_t label_sample_y;
    lv_coord_t unit_sample_y;
    lv_coord_t left_extra;
    lv_coord_t right_extra;
    lv_coord_t text_width;
    lv_coord_t name_font;
    lv_coord_t unit_font;
    lv_coord_t value_font;
    lv_coord_t value_offset_y;
    lv_coord_t value_width;
    lv_coord_t value_band_x;
    lv_coord_t value_band_w;
    bool axis_aligned_metadata = (slot_count <= 2u);
    lv_coord_t metadata_center_x;
    lv_coord_t metadata_w;
    lv_coord_t metadata_x;
    lv_coord_t panel_frame_x;
    lv_coord_t panel_frame_w;
    lv_coord_t panel_frame_right;
    lv_coord_t value_label_y;
    lv_coord_t value_line_h;
    lv_coord_t unit_line_h;
    lv_coord_t axis_unit_gap_y;
    ui_home_metric_rect_t value_rect;
    lv_obj_t *panel;
    lv_obj_t *name;
    lv_obj_t *value;
    lv_obj_t *unit;

    if (label_inset_x < ui_layout_px(3)) label_inset_x = ui_layout_px(3);
    if (label_inset_y < ui_layout_px(2)) label_inset_y = ui_layout_px(2);
    if (unit_inset_x < ui_layout_px(3)) unit_inset_x = ui_layout_px(3);
    if (unit_inset_y < ui_layout_px(1)) unit_inset_y = ui_layout_px(1);

    label_sample_y = y + label_inset_y;
    unit_sample_y = y + h - unit_inset_y;
    left_extra = ui_home_widgets_circular_safe_left(x, label_sample_y);
    right_extra = ui_home_widgets_circular_safe_right(x + w, unit_sample_y);
    if (left_extra > label_inset_x) {
        label_inset_x = left_extra;
    }
    if (right_extra > unit_inset_x) {
        unit_inset_x = right_extra;
    }

    text_width = w - (label_inset_x + unit_inset_x);
    if (text_width < ui_layout_px(56)) {
        text_width = ui_layout_px(56);
    }
    name_font = ui_home_widgets_slot_name_font(slot_count);
    unit_font = ui_home_widgets_slot_unit_font(slot_count);
    value_offset_y = is_single_slot ? h * 3 / 100 : 0;
    ui_home_widgets_value_band_bounds(x, y, w, h, value_offset_y, &value_band_x, &value_band_w);
    value_width = ui_home_widgets_value_safe_width(value_band_x,
                                                   y,
                                                   value_band_w,
                                                   h,
                                                   value_offset_y,
                                                   slot_count);
    value_rect = ui_home_widgets_value_rect_for_panel(value_band_x, y, value_band_w, h, slot_count, false, false);
    value_rect.w = value_width;
    value_font = ui_home_widgets_value_font_for_rect(item, value_rect);
    value_line_h = ui_home_widgets_exact_font_line_height(value_font, ui_layout_px(40));
    unit_line_h = ui_home_widgets_font_line_height(unit_font, ui_layout_px(16));
    axis_unit_gap_y = (slot_count == 2u) ? ui_layout_px(5) : ui_layout_px(10);
    metadata_center_x = value_band_x + (value_band_w / 2);
    metadata_w = axis_aligned_metadata ? value_width : text_width;
    metadata_x = metadata_center_x - (metadata_w / 2);

    panel_frame_x = LV_MIN(x, value_band_x);
    panel_frame_right = LV_MAX((lv_coord_t)(x + w), (lv_coord_t)(value_band_x + value_band_w));
    panel_frame_w = panel_frame_right - panel_frame_x;

    panel = lv_obj_create(parent);
    lv_obj_set_size(panel, panel_frame_w, h);
    lv_obj_align(panel, LV_ALIGN_TOP_LEFT, panel_frame_x, y);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);

    name = lv_label_create(panel);
    lv_label_set_text(name, "RPM");
    lv_obj_set_width(name, metadata_w);
    lv_label_set_long_mode(name, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(name, ui_font_typoder(name_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(name, lv_color_hex(0x9A9A9A), LV_PART_MAIN);
    lv_obj_set_style_text_align(name,
                                axis_aligned_metadata ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT,
                                LV_PART_MAIN);
    lv_obj_set_pos(name,
                   axis_aligned_metadata
                       ? (lv_coord_t)(metadata_x - panel_frame_x)
                       : (lv_coord_t)(x + label_inset_x - panel_frame_x),
                   label_inset_y);

    value = lv_label_create(panel);
    lv_label_set_text(value, "--");
    lv_obj_set_width(value, value_width);
    lv_label_set_long_mode(value, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(value, ui_font_typoder_exact(value_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(value, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    value_label_y = (lv_coord_t)(((h - value_line_h) / 2) + value_offset_y);
    lv_obj_set_pos(value,
                   (lv_coord_t)(value_band_x + ((value_band_w - value_width) / 2) - panel_frame_x),
                   value_label_y);

    unit = lv_label_create(panel);
    lv_label_set_text(unit, "");
    lv_obj_set_width(unit, metadata_w);
    lv_label_set_long_mode(unit, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(unit,
                                axis_aligned_metadata ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_RIGHT,
                                LV_PART_MAIN);
    lv_obj_set_style_text_font(unit, ui_font_typoder(unit_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(unit, lv_color_hex(0x707070), LV_PART_MAIN);
    lv_obj_set_pos(unit,
                   axis_aligned_metadata
                       ? (lv_coord_t)(metadata_x - panel_frame_x)
                       : (lv_coord_t)(x + w - unit_inset_x - text_width - panel_frame_x),
                   axis_aligned_metadata
                       ? (lv_coord_t)(value_label_y + value_line_h + axis_unit_gap_y)
                       : (lv_coord_t)(h - unit_inset_y - unit_line_h));

    *name_out = name;
    *value_out = value;
    *unit_out = unit;
}

static void ui_home_runtime_widgets_create_dense_slot_card(lv_obj_t *parent,
                                                           lv_coord_t x,
                                                           lv_coord_t y,
                                                           lv_coord_t w,
                                                           lv_coord_t h,
                                                           uint8_t slot_count,
                                                           lv_coord_t content_center_x,
                                                           const ui_home_metric_slot_style_t *dense_style,
                                                           disp_item_t item,
                                                           lv_obj_t **name_out,
                                                           lv_obj_t **value_out,
                                                           lv_obj_t **unit_out)
{
    ui_home_metric_slot_style_t fallback_style;
    const ui_home_metric_slot_style_t *style = dense_style;
    lv_obj_t *panel;
    lv_obj_t *name;
    lv_obj_t *value;
    lv_obj_t *unit;

    if (style == NULL || !style->valid) {
        ui_home_runtime_widgets_build_dense_slot_style(x,
                                                       y,
                                                       w,
                                                       h,
                                                       slot_count,
                                                       content_center_x,
                                                       item,
                                                       0,
                                                       false,
                                                       &fallback_style);
        style = &fallback_style;
    }

    panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, style->panel_w, style->panel_h);
    lv_obj_set_pos(panel, style->panel_x, style->panel_y);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);

    name = lv_label_create(panel);
    lv_label_set_text(name, "RPM");
    lv_obj_set_width(name, style->name_w);
    lv_label_set_long_mode(name, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(name, ui_font_typoder(style->name_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(name, lv_color_hex(0x9A9A9A), LV_PART_MAIN);
    lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(name,
                   (lv_coord_t)(style->content_center_x - style->panel_x - (style->name_w / 2)),
                   style->name_y);

    value = lv_label_create(panel);
    lv_label_set_text(value, "--");
    lv_obj_set_width(value, style->value_w);
    lv_label_set_long_mode(value, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(value, ui_font_typoder_exact(style->value_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(value, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_pos(value,
                   (lv_coord_t)(style->content_center_x - style->panel_x - (style->value_w / 2)),
                   style->value_y);

    unit = lv_label_create(panel);
    lv_label_set_text(unit, "");
    lv_obj_set_width(unit, style->unit_w);
    lv_label_set_long_mode(unit, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(unit, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(unit, ui_font_typoder(style->unit_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(unit, lv_color_hex(0x707070), LV_PART_MAIN);
    lv_obj_set_pos(unit,
                   (lv_coord_t)(style->content_center_x - style->panel_x - (style->unit_w / 2)),
                   style->unit_y);

    *name_out = name;
    *value_out = value;
    *unit_out = unit;
}

/** 创建首页菜单区域使用的中心卡片。 */
lv_obj_t *ui_home_runtime_widgets_create_menu_card(lv_obj_t *parent,
                                                   lv_coord_t width,
                                                   lv_coord_t min_height,
                                                   lv_coord_t y_offset)
{
    return ui_round_shell_create_center_card(parent,
                                             width,
                                             min_height,
                                             y_offset,
                                             ui_layout_px(22),
                                             ui_layout_px(16),
                                             lv_color_hex(0x2F2F2F));
}

/** 给“新增页面”按钮应用统一视觉主题。 */
void ui_home_runtime_widgets_apply_add_button_theme(lv_obj_t *btn, lv_coord_t size)
{
    if (btn == NULL) {
        return;
    }

    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x161B24), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, LV_MAX(2, size / 120), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x2F80ED), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
}

/**
 * 按槽位空间为名称、数值、单位应用合适排版
 *
 * 核心职责：重新计算宽度和字号，让不同页型下的指标卡都能稳定显示
 */
void ui_home_runtime_widgets_apply_slot_typography(lv_obj_t *name_label,
                                                   lv_obj_t *value_label,
                                                   lv_obj_t *unit_label,
                                                   disp_item_t item,
                                                   uint8_t slot_count,
                                                   int16_t forced_value_font)
{
    lv_obj_t *panel;
    lv_coord_t panel_x;
    lv_coord_t panel_y;
    lv_coord_t panel_w;
    lv_coord_t panel_h;
    lv_coord_t value_offset_y;
    lv_coord_t value_width;
    lv_coord_t value_center_x;
    ui_home_metric_rect_t value_rect;
    int16_t name_font;
    int16_t unit_font;
    int16_t value_font;
    lv_coord_t value_line_h;
    lv_coord_t axis_unit_gap_y;

    if (name_label == NULL || value_label == NULL || unit_label == NULL) {
        return;
    }
    if (ui_home_widgets_use_metric_block_layout(slot_count)) {
        return;
    }

    panel = lv_obj_get_parent(value_label);
    if (panel == NULL) {
        return;
    }

    panel_h = lv_obj_get_height(panel);
    panel_x = lv_obj_get_x(panel);
    panel_y = lv_obj_get_y(panel);
    panel_w = lv_obj_get_width(panel);

    name_font = ui_home_widgets_slot_name_font(slot_count);
    unit_font = ui_home_widgets_slot_unit_font(slot_count);
    value_offset_y = (slot_count == 1u) ? (panel_h * 3 / 100) : ((slot_count >= 5u) ? -(panel_h * 2 / 100) : 0);
    value_width = ui_home_widgets_value_safe_width(panel_x,
                                                   panel_y,
                                                   panel_w,
                                                   panel_h,
                                                   value_offset_y,
                                                   slot_count);
    if (forced_value_font > 0) {
        value_rect = ui_home_widgets_value_rect_for_panel(panel_x, panel_y, panel_w, panel_h, slot_count, false, false);
        value_rect.w = value_width;
        value_font = LV_MIN(forced_value_font, ui_home_widgets_value_font_for_rect(item, value_rect));
    } else {
        value_rect = ui_home_widgets_value_rect_for_panel(panel_x, panel_y, panel_w, panel_h, slot_count, false, false);
        value_rect.w = value_width;
        value_font = ui_home_widgets_value_font_for_rect(item, value_rect);
    }
    value_line_h = ui_home_widgets_exact_font_line_height(value_font, ui_layout_px(40));
    axis_unit_gap_y = (slot_count == 2u) ? ui_layout_px(5) : ui_layout_px(10);

    value_center_x = (lv_coord_t)(lv_obj_get_x(value_label) + (lv_obj_get_width(value_label) / 2));
    lv_obj_set_width(value_label, value_width);
    lv_obj_set_x(value_label, (lv_coord_t)(value_center_x - (value_width / 2)));
    if (slot_count <= 2u) {
        lv_obj_set_width(name_label, value_width);
        lv_obj_set_width(unit_label, value_width);
        lv_obj_set_x(name_label, (lv_coord_t)(value_center_x - (value_width / 2)));
        lv_obj_set_x(unit_label, (lv_coord_t)(value_center_x - (value_width / 2)));
        lv_obj_set_style_text_align(name_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_align(unit_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }
    lv_obj_set_style_text_font(name_label, ui_font_typoder(name_font), LV_PART_MAIN);
    lv_obj_set_style_text_font(unit_label, ui_font_typoder(unit_font), LV_PART_MAIN);
    lv_obj_set_style_text_font(value_label, ui_font_typoder_exact(value_font), LV_PART_MAIN);
    if (slot_count <= 2u) {
        lv_obj_set_y(unit_label,
                     (lv_coord_t)(lv_obj_get_y(value_label) + value_line_h + axis_unit_gap_y));
    }
}

void ui_home_runtime_widgets_apply_dense_slot_style(lv_obj_t *name_label,
                                                    lv_obj_t *value_label,
                                                    lv_obj_t *unit_label,
                                                    const ui_home_metric_slot_style_t *style)
{
    lv_obj_t *panel;

    if (name_label == NULL || value_label == NULL || unit_label == NULL ||
        style == NULL || !style->valid) {
        return;
    }

    panel = lv_obj_get_parent(value_label);
    if (panel == NULL) {
        return;
    }

    lv_obj_set_pos(panel, style->panel_x, style->panel_y);
    lv_obj_set_size(panel, style->panel_w, style->panel_h);
    lv_obj_set_width(name_label, style->name_w);
    lv_obj_set_width(value_label, style->value_w);
    lv_obj_set_width(unit_label, style->unit_w);
    lv_obj_set_style_text_font(name_label, ui_font_typoder(style->name_font), LV_PART_MAIN);
    lv_obj_set_style_text_font(value_label, ui_font_typoder_exact(style->value_font), LV_PART_MAIN);
    lv_obj_set_style_text_font(unit_label, ui_font_typoder(style->unit_font), LV_PART_MAIN);
    lv_obj_set_pos(name_label,
                   (lv_coord_t)(style->content_center_x - style->panel_x - (style->name_w / 2)),
                   style->name_y);
    lv_obj_set_pos(value_label,
                   (lv_coord_t)(style->content_center_x - style->panel_x - (style->value_w / 2)),
                   style->value_y);
    lv_obj_set_pos(unit_label,
                   (lv_coord_t)(style->content_center_x - style->panel_x - (style->unit_w / 2)),
                   style->unit_y);
}

bool ui_home_runtime_widgets_slot_style_center_interval(const ui_home_metric_slot_style_t *style,
                                                       disp_item_t item,
                                                       ui_home_metric_center_interval_t *interval)
{
    lv_coord_t text_w;
    lv_coord_t half_w;

    if (interval != NULL) {
        memset(interval, 0, sizeof(*interval));
    }
    if (style == NULL || interval == NULL || !style->valid || style->value_rect_w <= 0) {
        return false;
    }

    text_w = (style->value_text_w > 0)
                 ? style->value_text_w
                 : ui_font_measure_text_width(ui_font_typoder_exact(style->value_font),
                                               ui_home_widgets_value_sample_text(item));
    half_w = (text_w + ui_layout_px(10)) / 2;
    interval->min_x = style->value_rect_x + half_w;
    interval->max_x = style->value_rect_x + style->value_rect_w - half_w;
    if (interval->max_x < interval->min_x) {
        lv_coord_t center_x = style->value_rect_x + (style->value_rect_w / 2);
        interval->min_x = center_x;
        interval->max_x = center_x;
    }
    interval->valid = true;
    return true;
}

void ui_home_runtime_widgets_apply_content_center(ui_home_metric_slot_style_t *style, lv_coord_t center_x)
{
    lv_coord_t min_center;
    lv_coord_t max_center;

    if (style == NULL || !style->valid) {
        return;
    }

    min_center = style->value_rect_x + ((style->value_text_w + ui_layout_px(10)) / 2);
    max_center = style->value_rect_x + style->value_rect_w - ((style->value_text_w + ui_layout_px(10)) / 2);
    if (max_center < min_center) {
        min_center = style->value_rect_x + (style->value_rect_w / 2);
        max_center = min_center;
    }
    if (center_x < min_center) {
        center_x = min_center;
    } else if (center_x > max_center) {
        center_x = max_center;
    }
    style->content_center_x = center_x;
}

int16_t ui_home_runtime_widgets_measure_slot_value_font(lv_coord_t slot_w,
                                                        lv_coord_t slot_h,
                                                        disp_item_t item,
                                                        uint8_t slot_count)
{
    ui_home_metric_rect_t value_rect;

    value_rect = ui_home_widgets_value_rect_for_panel(0, 0, slot_w, slot_h, slot_count, false, false);
    return ui_home_widgets_value_font_for_rect(item, value_rect);
}

/**
 * 创建一个仪表槽位卡片
 *
 * 核心职责：搭建名称、数值、单位三段式结构，并按圆屏空间完成初始布局
 */
void ui_home_runtime_widgets_create_slot_card(lv_obj_t *parent,
                                              lv_coord_t x,
                                              lv_coord_t y,
                                              lv_coord_t w,
                                              lv_coord_t h,
                                              uint8_t slot_count,
                                              lv_coord_t content_center_x,
                                              const ui_home_metric_slot_style_t *dense_style,
                                              disp_item_t item,
                                              lv_obj_t **name_out,
                                              lv_obj_t **value_out,
                                              lv_obj_t **unit_out)
{
    if (ui_home_widgets_use_metric_block_layout(slot_count)) {
        ui_home_runtime_widgets_create_dense_slot_card(parent,
                                                       x,
                                                       y,
                                                       w,
                                                       h,
                                                       slot_count,
                                                       content_center_x,
                                                       dense_style,
                                                       item,
                                                       name_out,
                                                       value_out,
                                                       unit_out);
        return;
    }

    ui_home_runtime_widgets_create_box_slot_card(parent,
                                                 x,
                                                 y,
                                                 w,
                                                 h,
                                                 slot_count,
                                                 item,
                                                 name_out,
                                                 value_out,
                                                 unit_out);
}
