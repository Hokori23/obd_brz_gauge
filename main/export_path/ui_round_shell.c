#include "ui_round_shell.h"
#include "ui_font_profile.h"
#include <math.h>

/** 测量指定字体在样本文本下的高度。 */
static lv_coord_t ui_round_shell_measure_text_height(const lv_font_t *font, const char *sample_text)
{
    lv_point_t size = {0};

    if (font == NULL) {
        return 0;
    }

    lv_txt_get_size(&size,
                    (sample_text != NULL) ? sample_text : "0",
                    font,
                    0,
                    0,
                    LV_COORD_MAX,
                    LV_TEXT_FLAG_NONE);
    return size.y;
}

/**
 * 为滚轮计算更紧凑的圆屏几何尺寸
 *
 * 让文本高度、包裹高度和触摸区在圆屏上保持更平衡的观感。
 */
static void ui_round_shell_apply_compact_roller_geometry(lv_obj_t *roller,
                                                         lv_coord_t requested_height,
                                                         lv_coord_t font_size)
{
    const lv_font_t *font;
    lv_coord_t text_h;
    lv_coord_t legacy_item_h;
    lv_coord_t target_item_h;
    lv_coord_t wrapper_h;
    lv_coord_t pad_ver;

    if (roller == NULL) {
        return;
    }

    font = ui_font_typoder(font_size);
    text_h = ui_round_shell_measure_text_height(font, "000");
    if (text_h <= 0) {
        text_h = ui_layout_px(16);
    }

    legacy_item_h = text_h + (ui_layout_px(3) * 2);
    target_item_h = legacy_item_h + LV_MAX(1, legacy_item_h / 10);
    wrapper_h = LV_MAX(target_item_h, text_h + ui_layout_px(4));
    if (requested_height > 0) {
        wrapper_h = LV_MIN(wrapper_h, requested_height);
    }
    if (wrapper_h < ui_layout_px(24)) {
        wrapper_h = ui_layout_px(24);
    }

    pad_ver = (lv_coord_t)((wrapper_h - text_h) / 2);
    if (pad_ver < 1) {
        pad_ver = 1;
    }

    lv_obj_set_height(roller, wrapper_h);
    lv_obj_set_style_pad_top(roller, pad_ver, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(roller, pad_ver, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(roller, pad_ver, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(roller, pad_ver, LV_PART_SELECTED | LV_STATE_DEFAULT);
}

/** 给圆屏页面应用基础外观样式。 */
void ui_round_screen_apply_base(lv_obj_t *screen, lv_color_t bg_color)
{
    if (screen == NULL) {
        return;
    }

    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(screen, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(screen, true, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(screen, bg_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(screen, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(screen, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(screen, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/** 创建圆屏外圈装饰弧。 */
lv_obj_t *ui_round_shell_create_ring(lv_obj_t *parent, const ui_round_shell_layout_t *shell)
{
    if (parent == NULL || shell == NULL) {
        return NULL;
    }

    lv_obj_t *ring = lv_arc_create(parent);
    lv_obj_set_size(ring, shell->ring_diameter, shell->ring_diameter);
    lv_obj_set_align(ring, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_arc_set_value(ring, 0);
    lv_arc_set_bg_angles(ring, 0, 360);
    lv_obj_set_style_bg_opa(ring, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ring, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ring, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ring, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ring, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ring, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ring, shell->ring_arc_width, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(ring, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ring, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ring, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ring, shell->ring_arc_width, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ring, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    return ring;
}

/** 创建页头区域使用的胶囊按钮。 */
lv_obj_t *ui_round_shell_create_header_button(lv_obj_t *parent,
                                              const char *text,
                                              lv_coord_t width,
                                              lv_coord_t height)
{
    lv_obj_t *btn;
    lv_obj_t *label;

    if (parent == NULL) {
        return NULL;
    }

    btn = lv_btn_create(parent);
    lv_obj_set_size(btn, width, height);
    lv_obj_set_style_radius(btn, height / 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x161616), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x2D2D2D), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);

    label = lv_label_create(btn);
    lv_label_set_text(label, (text != NULL) ? text : "");
    lv_obj_set_style_text_font(label, ui_font_typoder(13), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(label);

    return btn;
}

/** 创建页面主标题。 */
lv_obj_t *ui_round_shell_create_page_title(lv_obj_t *parent,
                                           const char *title_text,
                                           lv_coord_t title_y,
                                           lv_coord_t title_font_size,
                                           lv_color_t title_color)
{
    lv_obj_t *title;
    lv_coord_t title_width;

    if (parent == NULL || title_text == NULL) {
        return NULL;
    }

    title = lv_label_create(parent);
    lv_label_set_text(title, title_text);
    lv_label_set_long_mode(title, LV_LABEL_LONG_CLIP);
    title_width = (lv_coord_t)(ui_screen_width() - ((ui_safe_margin() + ui_layout_px(8)) * 2));
    if (title_width < ui_layout_px(160)) {
        title_width = ui_layout_px(160);
    }
    lv_obj_set_width(title, title_width);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, ui_font_typoder(title_font_size), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, title_color, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, title_y);
    return title;
}

/**
 * 创建标题和副标题组合块
 *
 * 会根据圆屏安全区域自动收敛副标题宽度。
 */
void ui_round_shell_create_title_block(lv_obj_t *parent,
                                       const char *title_text,
                                       const char *subtitle_text,
                                       lv_coord_t title_y,
                                       lv_coord_t title_font_size,
                                       lv_coord_t subtitle_gap,
                                       lv_coord_t subtitle_font_size,
                                       lv_obj_t **title_out,
                                       lv_obj_t **subtitle_out)
{
    lv_obj_t *title;
    lv_obj_t *subtitle = NULL;
    lv_coord_t subtitle_left;
    lv_coord_t subtitle_right;
    lv_coord_t subtitle_width;

    if (title_out != NULL) {
        *title_out = NULL;
    }
    if (subtitle_out != NULL) {
        *subtitle_out = NULL;
    }
    if (parent == NULL || title_text == NULL) {
        return;
    }

    title = ui_round_shell_create_page_title(parent,
                                             title_text,
                                             title_y,
                                             title_font_size,
                                             lv_color_hex(0xFFFFFF));

    if (subtitle_text != NULL && subtitle_text[0] != '\0') {
        subtitle = lv_label_create(parent);
        lv_label_set_text(subtitle, subtitle_text);
        lv_label_set_long_mode(subtitle, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(subtitle, ui_font_hint(subtitle_font_size), LV_PART_MAIN);
        lv_obj_set_style_text_color(subtitle, lv_color_hex(0x8D8D8D), LV_PART_MAIN);
        ui_round_shell_safe_span_for_band((lv_coord_t)(title_y + title_font_size + subtitle_gap),
                                          ui_layout_px(subtitle_font_size + 8),
                                          ui_safe_margin() + ui_layout_px(18),
                                          &subtitle_left,
                                          &subtitle_right);
        subtitle_width = (lv_coord_t)(subtitle_right - subtitle_left);
        if (subtitle_width < ui_layout_px(120)) {
            subtitle_width = ui_layout_px(120);
        }
        lv_obj_set_width(subtitle, subtitle_width);
        lv_obj_set_style_text_align(subtitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_align_to(subtitle, title, LV_ALIGN_OUT_BOTTOM_MID, 0, subtitle_gap);
    }

    if (title_out != NULL) {
        *title_out = title;
    }
    if (subtitle_out != NULL) {
        *subtitle_out = subtitle;
    }
}

/** 创建顶部小胶囊信息块。 */
lv_obj_t *ui_round_shell_create_chip(lv_obj_t *parent,
                                     lv_coord_t width,
                                     lv_coord_t height,
                                     lv_coord_t y_offset,
                                     lv_color_t border_color)
{
    lv_obj_t *chip;

    if (parent == NULL) {
        return NULL;
    }

    chip = lv_obj_create(parent);
    lv_obj_set_size(chip, width, height);
    lv_obj_align(chip, LV_ALIGN_TOP_MID, 0, y_offset);
    ui_round_shell_apply_dark_card_theme(chip, height / 2, ui_layout_px(8));
    lv_obj_set_style_border_width(chip, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(chip, border_color, LV_PART_MAIN);
    lv_obj_clear_flag(chip, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return chip;
}

/** 创建居中的圆角卡片容器。 */
lv_obj_t *ui_round_shell_create_center_card(lv_obj_t *parent,
                                            lv_coord_t width,
                                            lv_coord_t min_height,
                                            lv_coord_t y_offset,
                                            lv_coord_t radius,
                                            lv_coord_t pad_all,
                                            lv_color_t border_color)
{
    lv_obj_t *card;

    if (parent == NULL) {
        return NULL;
    }

    card = lv_obj_create(parent);
    lv_obj_set_size(card, width, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(card, min_height, LV_PART_MAIN);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, y_offset);
    ui_round_shell_apply_dark_card_theme(card, radius, pad_all);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, border_color, LV_PART_MAIN);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, ui_layout_px(8), LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

/** 创建一条简单的分割线。 */
lv_obj_t *ui_round_shell_create_divider(lv_obj_t *parent,
                                        lv_coord_t x,
                                        lv_coord_t y,
                                        lv_coord_t w,
                                        lv_coord_t h,
                                        lv_color_t color,
                                        lv_opa_t opa)
{
    lv_obj_t *divider;

    if (parent == NULL) {
        return NULL;
    }

    divider = lv_obj_create(parent);
    lv_obj_remove_style_all(divider);
    lv_obj_set_size(divider, w, h);
    lv_obj_align(divider, LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(divider, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(divider, opa, LV_PART_MAIN);
    return divider;
}

/** 创建覆盖层里的中心行动卡片。 */
lv_obj_t *ui_round_shell_create_overlay_action_card(lv_obj_t *parent,
                                                    lv_coord_t width,
                                                    lv_coord_t height,
                                                    lv_color_t accent_color,
                                                    const char *text,
                                                    lv_coord_t font_size)
{
    lv_obj_t *card;
    lv_obj_t *label;

    if (parent == NULL) {
        return NULL;
    }

    card = lv_obj_create(parent);
    lv_obj_set_size(card, width, height);
    lv_obj_center(card);
    ui_round_shell_apply_dark_card_theme(card, ui_layout_px(26), ui_layout_px(12));
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, accent_color, LV_PART_MAIN);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x141414), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(card, ui_layout_px(14), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(card, 52, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(card, accent_color, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    label = lv_label_create(card);
    lv_label_set_text(label, (text != NULL) ? text : "");
    lv_obj_set_style_text_font(label, ui_font_typoder(font_size), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(label);
    return card;
}

/** 给卡片对象应用深色主题。 */
void ui_round_shell_apply_dark_card_theme(lv_obj_t *obj,
                                          lv_coord_t radius,
                                          lv_coord_t pad_all)
{
    if (obj == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(obj, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, radius, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, pad_all, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
}

/** 给滚轮应用主态和选中态的完整主题。 */
void ui_round_shell_apply_roller_theme(lv_obj_t *roller,
                                       lv_coord_t radius,
                                       lv_color_t main_bg,
                                       lv_color_t main_border,
                                       lv_color_t main_text,
                                       lv_color_t selected_bg,
                                       lv_color_t selected_border,
                                       lv_color_t selected_text)
{
    if (roller == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(roller, main_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(roller, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(roller, main_text, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(roller, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(roller, main_border, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(roller, radius, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(roller, true, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(roller, selected_bg, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(roller, LV_OPA_COVER, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(roller, selected_text, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(roller, 1, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(roller, selected_border, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(roller, LV_OPA_COVER, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(roller, radius, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(roller, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
}

/** 给滚轮应用圆屏深色预设。 */
void ui_round_shell_apply_dark_roller_preset(lv_obj_t *roller,
                                             lv_coord_t radius,
                                             lv_coord_t min_height,
                                             lv_coord_t font_size)
{
    if (roller == NULL) {
        return;
    }

    lv_obj_clear_flag(roller, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_roller_set_visible_row_count(roller, 1);
    lv_obj_set_style_text_font(roller, ui_font_typoder(font_size), LV_PART_MAIN);
    lv_obj_set_style_text_font(roller, ui_font_typoder(font_size), LV_PART_SELECTED);
    lv_obj_set_style_text_align(roller, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(roller, LV_TEXT_ALIGN_CENTER, LV_PART_SELECTED | LV_STATE_DEFAULT);
    ui_round_shell_apply_roller_theme(roller,
                                      radius,
                                      lv_color_hex(0x202020),
                                      lv_color_hex(0x444444),
                                      lv_color_hex(0xCFCFCF),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0x000000));
    ui_round_shell_apply_compact_roller_geometry(roller, min_height, font_size);
    ui_round_shell_set_roller_touch_target(roller, min_height, ui_layout_px(6), 0);
}

/** 调整滚轮的触摸热区和文字内边距。 */
void ui_round_shell_set_roller_touch_target(lv_obj_t *roller,
                                            lv_coord_t min_height,
                                            lv_coord_t pad_hor,
                                            lv_coord_t pad_ver)
{
    if (roller == NULL) {
        return;
    }

    if (min_height > 0 && lv_obj_get_height(roller) < min_height && pad_ver > 0) {
        lv_obj_set_height(roller, min_height);
    }

    lv_obj_set_style_pad_left(roller, pad_hor, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(roller, pad_hor, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(roller, pad_hor, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(roller, pad_hor, LV_PART_SELECTED | LV_STATE_DEFAULT);
    if (pad_ver > 0) {
        lv_obj_set_style_pad_top(roller, pad_ver, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(roller, pad_ver, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(roller, pad_ver, LV_PART_SELECTED | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(roller, pad_ver, LV_PART_SELECTED | LV_STATE_DEFAULT);
    }
    lv_obj_set_style_text_line_space(roller, ui_layout_px(2), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(roller, ui_layout_px(2), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(roller, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(roller, LV_TEXT_ALIGN_CENTER, LV_PART_SELECTED | LV_STATE_DEFAULT);
}

/** 给切换按钮应用统一主题。 */
void ui_round_shell_apply_toggle_button_theme(lv_obj_t *btn,
                                              lv_color_t accent_color)
{
    if (btn == NULL) {
        return;
    }

    lv_obj_set_height(btn, ui_layout_px(40));
    lv_obj_set_style_radius(btn, ui_layout_px(14), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x3A3A3A), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(btn, ui_font_typoder(14), LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, lv_color_hex(0xD2D2D2), LV_PART_MAIN);
    lv_obj_set_style_pad_left(btn, ui_layout_px(8), LV_PART_MAIN);
    lv_obj_set_style_pad_right(btn, ui_layout_px(8), LV_PART_MAIN);

    lv_obj_set_style_border_color(btn, accent_color, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(btn, accent_color, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_CHECKED);
}

/** 给普通操作按钮应用统一主题。 */
void ui_round_shell_apply_action_button_theme(lv_obj_t *btn,
                                              lv_color_t accent_color,
                                              bool emphasize_accent,
                                              lv_coord_t radius,
                                              lv_coord_t font_size)
{
    if (btn == NULL) {
        return;
    }

    lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, accent_color, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(btn, ui_font_typoder(font_size), LV_PART_MAIN);
    lv_obj_set_style_text_color(btn,
                                emphasize_accent ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xE7EEF9),
                                LV_PART_MAIN);
    lv_obj_set_style_pad_left(btn, ui_layout_px(12), LV_PART_MAIN);
    lv_obj_set_style_pad_right(btn, ui_layout_px(12), LV_PART_MAIN);
    lv_obj_set_style_pad_top(btn, ui_layout_px(8), LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(btn, ui_layout_px(8), LV_PART_MAIN);

    if (emphasize_accent) {
        lv_obj_set_style_bg_color(btn, accent_color, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x161B24), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    }
}

/** 给危险操作按钮应用统一主题。 */
void ui_round_shell_apply_danger_button_theme(lv_obj_t *btn,
                                              lv_coord_t radius,
                                              lv_coord_t pad_all)
{
    if (btn == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(btn, lv_color_hex(0xBB2222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, pad_all, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
}

/** 给列表按钮应用统一主题。 */
void ui_round_shell_apply_list_button_theme(lv_obj_t *btn,
                                            lv_coord_t font_size)
{
    if (btn == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(btn, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(btn, ui_font_typoder(font_size), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
}

/**
 * 给消息框应用圆屏弹窗主题
 *
 * 同时重新排版按钮区域，保证主按钮更醒目。
 */
void ui_round_shell_apply_modal_theme(lv_obj_t *msgbox,
                                      lv_color_t accent_color,
                                      int32_t primary_btn_index)
{
    lv_obj_t *title;
    lv_obj_t *content;
    lv_obj_t *btns;
    uint32_t child_cnt;

    if (msgbox == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(msgbox, lv_color_hex(0x161616), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(msgbox, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(msgbox, lv_color_hex(0x363636), LV_PART_MAIN);
    lv_obj_set_style_border_width(msgbox, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(msgbox, ui_layout_px(18), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(msgbox, ui_layout_px(16), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(msgbox, 64, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(msgbox, accent_color, LV_PART_MAIN);
    lv_obj_set_style_text_color(msgbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_pad_all(msgbox, ui_layout_px(16), LV_PART_MAIN);
    lv_obj_set_style_pad_row(msgbox, ui_layout_px(10), LV_PART_MAIN);

    title = lv_msgbox_get_title(msgbox);
    if (title != NULL) {
        lv_obj_set_style_text_font(title, ui_font_typoder(22), LV_PART_MAIN);
        lv_obj_set_style_text_color(title, accent_color, LV_PART_MAIN);
    }

    content = lv_msgbox_get_text(msgbox);
    if (content != NULL) {
        lv_obj_set_style_text_font(content, ui_font_hint(12), LV_PART_MAIN);
        lv_obj_set_style_text_color(content, lv_color_hex(0xD6D6D6), LV_PART_MAIN);
        lv_obj_set_style_text_line_space(content, ui_layout_px(3), LV_PART_MAIN);
    }

    btns = lv_msgbox_get_btns(msgbox);
    if (btns == NULL) {
        return;
    }

    lv_obj_set_width(btns, LV_PCT(100));
    lv_obj_set_height(btns, ui_layout_px(58));
    lv_obj_set_style_pad_column(btns, ui_layout_px(10), LV_PART_MAIN);
    lv_obj_set_style_pad_all(btns, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btns, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_layout(btns, LV_LAYOUT_GRID);
    static lv_coord_t grid_cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_rows[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(btns, grid_cols, grid_rows);

    child_cnt = lv_obj_get_child_cnt(btns);
    for (uint32_t i = 0; i < child_cnt; ++i) {
        lv_obj_t *btn = lv_obj_get_child(btns, i);
        bool is_primary = ((int32_t)i == primary_btn_index) || (child_cnt == 1u);

        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, (int32_t)i, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
        lv_obj_set_height(btn, ui_layout_px(58));
        lv_obj_set_style_radius(btn, ui_layout_px(16), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_text_font(btn, ui_font_typoder(20), LV_PART_MAIN);
        lv_obj_set_style_text_align(btn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_pad_left(btn, ui_layout_px(6), LV_PART_MAIN);
        lv_obj_set_style_pad_right(btn, ui_layout_px(6), LV_PART_MAIN);

        if (is_primary) {
            lv_obj_set_style_bg_color(btn, accent_color, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(btn, accent_color, LV_PART_MAIN);
            lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x222222), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x424242), LV_PART_MAIN);
            lv_obj_set_style_text_color(btn, lv_color_hex(0xD0D0D0), LV_PART_MAIN);
        }
    }
}

/** 计算圆屏在指定 y 坐标处的左边界。 */
lv_coord_t ui_round_shell_circle_left_at_y(lv_coord_t y)
{
    int32_t r = (int32_t)ui_round_radius();
    int32_t dy = (int32_t)y - r;
    int32_t r2_dy2 = r * r - dy * dy;

    if (r2_dy2 <= 0) {
        return 0;
    }

    return (lv_coord_t)(r - (int32_t)sqrtf((float)r2_dy2));
}

/** 计算圆屏在指定 y 坐标处的右边界。 */
lv_coord_t ui_round_shell_circle_right_at_y(lv_coord_t y)
{
    return (lv_coord_t)(ui_screen_width() - ui_round_shell_circle_left_at_y(y));
}

/**
 * 计算某条水平带状区域在圆屏内的安全左右边界
 *
 * 通过多点采样避免控件在圆屏边缘被裁切。
 */
void ui_round_shell_safe_span_for_band(lv_coord_t y,
                                       lv_coord_t h,
                                       lv_coord_t inset,
                                       lv_coord_t *left_out,
                                       lv_coord_t *right_out)
{
    lv_coord_t samples[3] = {y, y + (h / 2), y + h - 1};
    lv_coord_t left = 0;
    lv_coord_t right = (lv_coord_t)ui_screen_width();

    if (h < 1) {
        h = 1;
        samples[1] = y;
        samples[2] = y;
    }

    for (uint8_t i = 0; i < 3; ++i) {
        lv_coord_t sample_left = ui_round_shell_circle_left_at_y(samples[i]) + inset;
        lv_coord_t sample_right = ui_round_shell_circle_right_at_y(samples[i]) - inset;
        if (sample_left > left) {
            left = sample_left;
        }
        if (sample_right < right) {
            right = sample_right;
        }
    }

    if (right < left) {
        lv_coord_t center = (lv_coord_t)(ui_screen_width() / 2);
        left = center;
        right = center;
    }

    if (left_out != NULL) {
        *left_out = left;
    }
    if (right_out != NULL) {
        *right_out = right;
    }
}
