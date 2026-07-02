#include "ui_home_runtime_widgets.h"

#include "ui_font_profile.h"
#include "ui_layout.h"
#include "ui_round_shell.h"

static const char *ui_home_widgets_value_sample_text(disp_item_t item)
{
    switch (item) {
    case DISP_ITEM_RPM:
        return "8000";
    case DISP_ITEM_SPEED:
        return "999";
    case DISP_ITEM_BAT:
        return "16.8";
    case DISP_ITEM_OILP:
        return "100.0";
    case DISP_ITEM_BOOST:
        return "-20.0";
    case DISP_ITEM_LOAD:
    case DISP_ITEM_TPS:
        return "100";
    case DISP_ITEM_BKT:
        return "600";
    case DISP_ITEM_CLT:
    case DISP_ITEM_IAT:
    case DISP_ITEM_OIL:
    default:
        return "120";
    }
}

static lv_coord_t ui_home_widgets_value_height_limit(lv_coord_t slot_h, uint8_t slot_count)
{
    if (slot_h < ui_layout_px(48)) {
        slot_h = ui_layout_px(48);
    }

    if (slot_count == 1u) {
        return slot_h * 82 / 100;
    }
    if (slot_count <= 2u) {
        return slot_h * 70 / 100;
    }
    if (slot_count <= 4u) {
        return slot_h * 62 / 100;
    }
    return slot_h * 52 / 100;
}

static bool ui_home_widgets_font_fits_text(const lv_font_t *font,
                                           const char *sample_text,
                                           lv_coord_t max_width,
                                           lv_coord_t max_height)
{
    lv_point_t size = {0};

    if (font == NULL || sample_text == NULL) {
        return false;
    }

    lv_txt_get_size(&size,
                    sample_text,
                    font,
                    0,
                    0,
                    LV_COORD_MAX,
                    LV_TEXT_FLAG_NONE);
    return (size.x <= max_width) && (size.y <= max_height);
}

static lv_coord_t ui_home_widgets_circular_safe_left(lv_coord_t panel_x, lv_coord_t panel_y)
{
    lv_coord_t circle_left = ui_round_shell_circle_left_at_y(panel_y);
    lv_coord_t margin = ui_layout_px(1);

    if (circle_left + margin > panel_x) {
        return (circle_left + margin) - panel_x;
    }
    return 0;
}

static lv_coord_t ui_home_widgets_circular_safe_right(lv_coord_t panel_right, lv_coord_t panel_y)
{
    lv_coord_t circle_right = ui_round_shell_circle_right_at_y(panel_y);
    lv_coord_t margin = ui_layout_px(1);

    if (circle_right - margin < panel_right) {
        return panel_right - (circle_right - margin);
    }
    return 0;
}

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

static void ui_home_widgets_value_host_bounds(lv_coord_t panel_x,
                                              lv_coord_t panel_y,
                                              lv_coord_t panel_w,
                                              lv_coord_t panel_h,
                                              lv_coord_t value_offset_y,
                                              lv_coord_t *host_x_out,
                                              lv_coord_t *host_w_out)
{
    lv_coord_t band_h = panel_h * 30 / 100;
    lv_coord_t band_y;
    lv_coord_t span_left;
    lv_coord_t span_right;
    lv_coord_t host_left;
    lv_coord_t host_right;
    lv_coord_t center_x = ui_center_x();
    lv_coord_t mid_gap = ui_layout_px(4);
    lv_coord_t panel_right = panel_x + panel_w;

    if (host_x_out != NULL) {
        *host_x_out = panel_x;
    }
    if (host_w_out != NULL) {
        *host_w_out = panel_w;
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

    host_left = span_left;
    host_right = span_right;
    if (panel_right <= center_x + ui_layout_px(4)) {
        host_right = center_x - mid_gap;
    } else if (panel_x >= center_x - ui_layout_px(4)) {
        host_left = center_x + mid_gap;
    }

    if (host_right <= host_left) {
        host_left = panel_x;
        host_right = panel_right;
    }

    if (host_x_out != NULL) {
        *host_x_out = host_left;
    }
    if (host_w_out != NULL) {
        *host_w_out = LV_MAX(host_right - host_left, ui_layout_px(56));
    }
}

static int16_t ui_home_widgets_slot_name_font(uint8_t slot_count)
{
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

static int16_t ui_home_widgets_slot_unit_font(uint8_t slot_count)
{
    if (slot_count == 1u) {
        return 22;
    }
    if (slot_count <= 2u) {
        return 20;
    }
    return 16;
}

static int16_t ui_home_widgets_slot_value_font(lv_coord_t text_width,
                                               lv_coord_t slot_h,
                                               disp_item_t item,
                                               uint8_t slot_count)
{
    static const int16_t k_font_candidates[] = {
        140, 132, 124, 116, 108, 100, 92, 84, 76, 68, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16
    };
    lv_coord_t height_limit;
    lv_coord_t width_limit;
    const char *sample_text;

    if (text_width < ui_layout_px(48)) {
        text_width = ui_layout_px(48);
    }
    height_limit = ui_home_widgets_value_height_limit(slot_h, slot_count);
    width_limit = text_width - ui_layout_px(4);
    if (width_limit < ui_layout_px(40)) {
        width_limit = ui_layout_px(40);
    }
    sample_text = ui_home_widgets_value_sample_text(item);

    for (uint8_t i = 0; i < (sizeof(k_font_candidates) / sizeof(k_font_candidates[0])); ++i) {
        int16_t candidate = k_font_candidates[i];
        if (candidate > height_limit) {
            continue;
        }
        if (ui_home_widgets_font_fits_text(ui_font_typoder(candidate),
                                           sample_text,
                                           width_limit,
                                           height_limit)) {
            return candidate;
        }
    }

    return 16;
}

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

void ui_home_runtime_widgets_apply_slot_typography(lv_obj_t *name_label,
                                                   lv_obj_t *value_label,
                                                   lv_obj_t *unit_label,
                                                   disp_item_t item,
                                                   uint8_t slot_count)
{
    lv_obj_t *panel;
    lv_coord_t panel_x;
    lv_coord_t panel_y;
    lv_coord_t panel_w;
    lv_coord_t panel_h;
    lv_coord_t text_width;
    lv_coord_t value_offset_y;
    lv_coord_t value_width;
    int16_t name_font;
    int16_t unit_font;
    int16_t value_font;

    if (name_label == NULL || value_label == NULL || unit_label == NULL) {
        return;
    }

    panel = lv_obj_get_parent(value_label);
    if (panel == NULL) {
        return;
    }

    panel_x = lv_obj_get_x(panel);
    panel_y = lv_obj_get_y(panel);
    panel_w = lv_obj_get_width(panel);
    panel_h = lv_obj_get_height(panel);
    value_offset_y = (slot_count == 1u) ? (panel_h * 3 / 100) : ((slot_count >= 5u) ? -(panel_h * 2 / 100) : 0);
    value_width = ui_home_widgets_value_safe_width(panel_x, panel_y, panel_w, panel_h, value_offset_y, slot_count);
    text_width = value_width;

    name_font = ui_home_widgets_slot_name_font(slot_count);
    unit_font = ui_home_widgets_slot_unit_font(slot_count);
    value_font = ui_home_widgets_slot_value_font(text_width, panel_h, item, slot_count);

    lv_obj_set_width(value_label, value_width);
    lv_obj_set_style_text_font(name_label, ui_font_typoder(name_font), LV_PART_MAIN);
    lv_obj_set_style_text_font(unit_label, ui_font_typoder(unit_font), LV_PART_MAIN);
    lv_obj_set_style_text_font(value_label, ui_font_typoder(value_font), LV_PART_MAIN);
}

void ui_home_runtime_widgets_create_slot_card(lv_obj_t *parent,
                                              lv_coord_t x,
                                              lv_coord_t y,
                                              lv_coord_t w,
                                              lv_coord_t h,
                                              uint8_t slot_count,
                                              lv_obj_t **name_out,
                                              lv_obj_t **value_out,
                                              lv_obj_t **unit_out)
{
    bool is_single_slot = (slot_count == 1u);
    lv_coord_t label_inset_x = w * (is_single_slot ? 3 : 4) / 100;
    lv_coord_t label_inset_y = is_single_slot ? h * 2 / 100 : h * 3 / 100;
    lv_coord_t unit_inset_x = label_inset_x;
    lv_coord_t unit_inset_y = is_single_slot ? h * 2 / 100 : h * 2 / 100;
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
    lv_coord_t value_host_x;
    lv_coord_t value_host_w;
    lv_obj_t *panel;
    lv_obj_t *value_host;
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
    value_offset_y = is_single_slot ? h * 3 / 100 : ((slot_count >= 5u) ? -(h * 2 / 100) : 0);
    ui_home_widgets_value_host_bounds(x, y, w, h, value_offset_y, &value_host_x, &value_host_w);
    value_width = ui_home_widgets_value_safe_width(value_host_x,
                                                   y,
                                                   value_host_w,
                                                   h,
                                                   value_offset_y,
                                                   slot_count);
    value_font = ui_home_widgets_slot_value_font(value_width, h, DISP_ITEM_RPM, slot_count);

    panel = lv_obj_create(parent);
    lv_obj_set_size(panel, w, h);
    lv_obj_align(panel, LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);

    value_host = lv_obj_create(parent);
    lv_obj_set_size(value_host, value_host_w, h);
    lv_obj_align(value_host, LV_ALIGN_TOP_LEFT, value_host_x, y);
    lv_obj_clear_flag(value_host, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(value_host, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(value_host, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(value_host, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(value_host, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(value_host, 0, LV_PART_MAIN);

    name = lv_label_create(panel);
    lv_label_set_text(name, "RPM");
    lv_obj_set_width(name, text_width);
    lv_label_set_long_mode(name, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(name, ui_font_typoder(name_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(name, lv_color_hex(0x9A9A9A), LV_PART_MAIN);
    lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_align(name, LV_ALIGN_TOP_LEFT, label_inset_x, label_inset_y);

    value = lv_label_create(value_host);
    lv_label_set_text(value, "--");
    lv_obj_set_width(value, value_width);
    lv_label_set_long_mode(value, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(value, ui_font_typoder(value_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(value, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(value, LV_ALIGN_CENTER, 0, value_offset_y);

    unit = lv_label_create(panel);
    lv_label_set_text(unit, "");
    lv_obj_set_width(unit, text_width);
    lv_label_set_long_mode(unit, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(unit, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_font(unit, ui_font_typoder(unit_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(unit, lv_color_hex(0x707070), LV_PART_MAIN);
    lv_obj_align(unit, LV_ALIGN_BOTTOM_RIGHT, -unit_inset_x, -unit_inset_y);

    *name_out = name;
    *value_out = value;
    *unit_out = unit;
}
