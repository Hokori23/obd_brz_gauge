// Temperature Monitor Page
// CLT / IAT / OIL(SSM 22 10 17) - 3-row layout

#include "../ui.h"
#include "../ui_font_profile.h"
#include "../ui_layout.h"
#include "../ui_round_shell.h"

// Value labels (externally accessible from timer callback)
lv_obj_t *ui_LabelCoolantTempText = NULL;
lv_obj_t *ui_LabelOilTempText     = NULL;  // 真实机油温度 °C (SSM 22 10 17, A-40)
lv_obj_t *ui_LabelIntakeTempText  = NULL;
lv_obj_t *ui_LabelTempValue[3]    = {NULL, NULL, NULL};
lv_obj_t *ui_LabelTempName[3]     = {NULL, NULL, NULL};
lv_obj_t *ui_LabelTempUnit[3]     = {NULL, NULL, NULL};
static ui_temp_layout_t s_temp_layout;

// Helper: colored circle dot
static lv_obj_t *create_color_dot(lv_obj_t *parent, lv_color_t color, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, s_temp_layout.dot_size, s_temp_layout.dot_size);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, 255, LV_PART_MAIN);
    lv_obj_align(dot, LV_ALIGN_LEFT_MID, x, y);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return dot;
}

// Helper: create one data row (dot + name + value label + unit)
static void make_row(lv_obj_t *parent, lv_obj_t **name_out, lv_obj_t **val_out, lv_obj_t **unit_out,
                     lv_coord_t cy, lv_color_t color,
                     const char *name_str, const char *unit_str)
{
    // Left column: value
    // Left boundary = 70px (matches divider line edge, safe for all row Y positions)
    *val_out = lv_label_create(parent);
    lv_label_set_long_mode(*val_out, LV_LABEL_LONG_CLIP);   // 数值过长不换行
    lv_label_set_text(*val_out, "--");
    lv_obj_set_style_text_font(*val_out, ui_font_typoder(36), LV_PART_MAIN);
    lv_obj_set_style_text_color(*val_out, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_width(*val_out, s_temp_layout.value_width);
    lv_obj_set_style_text_align(*val_out, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_align(*val_out, LV_ALIGN_LEFT_MID, s_temp_layout.value_x, cy);

    // Right column: dot + name + unit
    // Right boundary = 290px (x=360-70, matches divider line edge)
    // dot left=185, name left=200..244, unit right=290
    create_color_dot(parent, color, s_temp_layout.dot_x, cy);

    *name_out = lv_label_create(parent);
    lv_label_set_long_mode(*name_out, LV_LABEL_LONG_CLIP);   // 不换行
    lv_label_set_text(*name_out, name_str);
    lv_obj_set_style_text_font(*name_out, ui_font_typoder(16), LV_PART_MAIN);
    lv_obj_set_style_text_color(*name_out, color, LV_PART_MAIN);
    lv_obj_set_width(*name_out, LV_SIZE_CONTENT);            // 宽度随文字, 长名(BOOST/SPEED)不换行
    lv_obj_set_style_text_align(*name_out, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_align(*name_out, LV_ALIGN_LEFT_MID, s_temp_layout.name_x, cy);

    *unit_out = lv_label_create(parent);
    lv_label_set_long_mode(*unit_out, LV_LABEL_LONG_CLIP);   // 不换行
    lv_label_set_text(*unit_out, unit_str);
    lv_obj_set_style_text_font(*unit_out, ui_font_typoder(16), LV_PART_MAIN);
    lv_obj_set_style_text_color(*unit_out, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_width(*unit_out, LV_SIZE_CONTENT);            // 宽度随文字 (km/h 等)
    lv_obj_set_style_text_align(*unit_out, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_align(*unit_out, LV_ALIGN_RIGHT_MID, s_temp_layout.unit_x, cy);
}

// Helper: horizontal divider line
static void make_hdiv(lv_obj_t *parent, lv_coord_t y, lv_coord_t w)
{
    lv_obj_t *div = lv_obj_create(parent);
    lv_obj_remove_style_all(div);
    lv_obj_set_size(div, w, 1);
    lv_obj_align(div, LV_ALIGN_CENTER, 0, y);
    lv_obj_set_style_bg_color(div, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(div, 50, LV_PART_MAIN);
    lv_obj_clear_flag(div, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *ui_page_temp_content_create(lv_obj_t *parent)
{
    ui_temp_layout_get(&s_temp_layout);

    lv_obj_t *arc_bg = lv_arc_create(parent);
    lv_obj_set_width(arc_bg, s_temp_layout.inner_arc_diameter);
    lv_obj_set_height(arc_bg, s_temp_layout.inner_arc_diameter);
    lv_obj_set_align(arc_bg, LV_ALIGN_CENTER);
    lv_obj_clear_flag(arc_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_arc_set_value(arc_bg, 0);
    lv_arc_set_bg_angles(arc_bg, 0, 360);
    lv_obj_set_style_arc_color(arc_bg, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(arc_bg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(arc_bg, s_temp_layout.inner_arc_width, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(arc_bg, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(arc_bg, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(arc_bg, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(arc_bg, s_temp_layout.inner_arc_width, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(arc_bg, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    make_row(parent, &ui_LabelTempName[0], &ui_LabelTempValue[0], &ui_LabelTempUnit[0], s_temp_layout.row1_cy, lv_color_hex(0x44AAFF), "CLT", "掳C");
    make_hdiv(parent, s_temp_layout.divider1_y, s_temp_layout.divider_width);
    make_row(parent, &ui_LabelTempName[1], &ui_LabelTempValue[1], &ui_LabelTempUnit[1], s_temp_layout.row2_cy, lv_color_hex(0x44FF88), "IAT", "掳C");
    make_hdiv(parent, s_temp_layout.divider2_y, s_temp_layout.divider_width);
    make_row(parent, &ui_LabelTempName[2], &ui_LabelTempValue[2], &ui_LabelTempUnit[2], s_temp_layout.row3_cy, lv_color_hex(0xFF7722), "OIL", "掳C");

    ui_LabelCoolantTempText = ui_LabelTempValue[0];
    ui_LabelIntakeTempText = ui_LabelTempValue[1];
    ui_LabelOilTempText = ui_LabelTempValue[2];
    return parent;
}

void ui_ScreenPageTemp_screen_init(void)
{
    ui_temp_layout_get(&s_temp_layout);

    ui_ScreenPageTemp = lv_obj_create(NULL);
    ui_round_screen_apply_base(ui_ScreenPageTemp, lv_color_hex(0x440000));
    ui_round_shell_create_ring(ui_ScreenPageTemp, &s_temp_layout.shell);

    // Inner arc ring (decorative)
    lv_obj_t *arc_bg = lv_arc_create(ui_ScreenPageTemp);
    lv_obj_set_width(arc_bg, s_temp_layout.inner_arc_diameter);
    lv_obj_set_height(arc_bg, s_temp_layout.inner_arc_diameter);
    lv_obj_set_align(arc_bg, LV_ALIGN_CENTER);
    lv_obj_clear_flag(arc_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_arc_set_value(arc_bg, 0);
    lv_arc_set_bg_angles(arc_bg, 0, 360);
    lv_obj_set_style_arc_color(arc_bg, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(arc_bg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(arc_bg, s_temp_layout.inner_arc_width, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(arc_bg, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(arc_bg, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(arc_bg, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(arc_bg, s_temp_layout.inner_arc_width, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(arc_bg, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    // ====== Row 1 (cy=-65): CLT - Blue ======
    make_row(ui_ScreenPageTemp, &ui_LabelTempName[0], &ui_LabelTempValue[0], &ui_LabelTempUnit[0], s_temp_layout.row1_cy, lv_color_hex(0x44AAFF), "CLT", "°C");
    make_hdiv(ui_ScreenPageTemp, s_temp_layout.divider1_y, s_temp_layout.divider_width);

    // ====== Row 2 (cy=+5): IAT - Green ======
    make_row(ui_ScreenPageTemp, &ui_LabelTempName[1], &ui_LabelTempValue[1], &ui_LabelTempUnit[1], s_temp_layout.row2_cy, lv_color_hex(0x44FF88), "IAT", "°C");
    make_hdiv(ui_ScreenPageTemp, s_temp_layout.divider2_y, s_temp_layout.divider_width);

    // ====== Row 3 (cy=+75): OIL - Amber (SSM 22 10 17) ======
    make_row(ui_ScreenPageTemp, &ui_LabelTempName[2], &ui_LabelTempValue[2], &ui_LabelTempUnit[2], s_temp_layout.row3_cy, lv_color_hex(0xFF7722), "OIL", "°C");

    // Backward compatibility for existing update code
    ui_LabelCoolantTempText = ui_LabelTempValue[0];
    ui_LabelIntakeTempText = ui_LabelTempValue[1];
    ui_LabelOilTempText = ui_LabelTempValue[2];

    // Events
    lv_obj_add_event_cb(ui_ScreenPageTemp, ui_event_temp_background, LV_EVENT_GESTURE, NULL);
}
