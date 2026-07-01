#pragma once

#include "ui_layout.h"
#include "lvgl.h"

void ui_round_screen_apply_base(lv_obj_t *screen, lv_color_t bg_color);
lv_obj_t *ui_round_shell_create_ring(lv_obj_t *parent, const ui_round_shell_layout_t *shell);
void ui_round_shell_apply_roller_theme(lv_obj_t *roller,
                                       lv_coord_t radius,
                                       lv_color_t main_bg,
                                       lv_color_t main_border,
                                       lv_color_t main_text,
                                       lv_color_t selected_bg,
                                       lv_color_t selected_border,
                                       lv_color_t selected_text);
