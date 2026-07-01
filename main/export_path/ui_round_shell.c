#include "ui_round_shell.h"

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
    lv_obj_set_style_arc_opa(ring, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ring, shell->ring_arc_width, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(ring, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ring, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ring, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ring, shell->ring_arc_width, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ring, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    return ring;
}

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
