#pragma once

#include "ui_layout.h"
#include "lvgl.h"

void ui_round_screen_apply_base(lv_obj_t *screen, lv_color_t bg_color);
lv_obj_t *ui_round_shell_create_ring(lv_obj_t *parent, const ui_round_shell_layout_t *shell);
lv_obj_t *ui_round_shell_create_header_button(lv_obj_t *parent,
                                              const char *text,
                                              lv_coord_t width,
                                              lv_coord_t height);
lv_obj_t *ui_round_shell_create_page_title(lv_obj_t *parent,
                                           const char *title_text,
                                           lv_coord_t title_y,
                                           lv_coord_t title_font_size,
                                           lv_color_t title_color);
void ui_round_shell_create_title_block(lv_obj_t *parent,
                                       const char *title_text,
                                       const char *subtitle_text,
                                       lv_coord_t title_y,
                                       lv_coord_t title_font_size,
                                       lv_coord_t subtitle_gap,
                                       lv_coord_t subtitle_font_size,
                                       lv_obj_t **title_out,
                                       lv_obj_t **subtitle_out);
lv_obj_t *ui_round_shell_create_chip(lv_obj_t *parent,
                                     lv_coord_t width,
                                     lv_coord_t height,
                                     lv_coord_t y_offset,
                                     lv_color_t border_color);
lv_obj_t *ui_round_shell_create_center_card(lv_obj_t *parent,
                                            lv_coord_t width,
                                            lv_coord_t min_height,
                                            lv_coord_t y_offset,
                                            lv_coord_t radius,
                                            lv_coord_t pad_all,
                                            lv_color_t border_color);
lv_obj_t *ui_round_shell_create_divider(lv_obj_t *parent,
                                        lv_coord_t x,
                                        lv_coord_t y,
                                        lv_coord_t w,
                                        lv_coord_t h,
                                        lv_color_t color,
                                        lv_opa_t opa);
lv_obj_t *ui_round_shell_create_overlay_action_card(lv_obj_t *parent,
                                                    lv_coord_t width,
                                                    lv_coord_t height,
                                                    lv_color_t accent_color,
                                                    const char *text,
                                                    lv_coord_t font_size);
void ui_round_shell_apply_dark_card_theme(lv_obj_t *obj,
                                          lv_coord_t radius,
                                          lv_coord_t pad_all);
void ui_round_shell_apply_roller_theme(lv_obj_t *roller,
                                       lv_coord_t radius,
                                       lv_color_t main_bg,
                                       lv_color_t main_border,
                                       lv_color_t main_text,
                                       lv_color_t selected_bg,
                                       lv_color_t selected_border,
                                       lv_color_t selected_text);
void ui_round_shell_apply_dark_roller_preset(lv_obj_t *roller,
                                             lv_coord_t radius,
                                             lv_coord_t min_height,
                                             lv_coord_t font_size);
void ui_round_shell_set_roller_touch_target(lv_obj_t *roller,
                                            lv_coord_t min_height,
                                            lv_coord_t pad_hor,
                                            lv_coord_t pad_ver);
void ui_round_shell_apply_toggle_button_theme(lv_obj_t *btn,
                                              lv_color_t accent_color);
void ui_round_shell_apply_action_button_theme(lv_obj_t *btn,
                                              lv_color_t accent_color,
                                              bool emphasize_accent,
                                              lv_coord_t radius,
                                              lv_coord_t font_size);
void ui_round_shell_apply_danger_button_theme(lv_obj_t *btn,
                                              lv_coord_t radius,
                                              lv_coord_t pad_all);
void ui_round_shell_apply_list_button_theme(lv_obj_t *btn,
                                            lv_coord_t font_size);
void ui_round_shell_apply_modal_theme(lv_obj_t *msgbox,
                                      lv_color_t accent_color,
                                      int32_t primary_btn_index);
lv_coord_t ui_round_shell_circle_left_at_y(lv_coord_t y);
lv_coord_t ui_round_shell_circle_right_at_y(lv_coord_t y);
void ui_round_shell_safe_span_for_band(lv_coord_t y,
                                       lv_coord_t h,
                                       lv_coord_t inset,
                                       lv_coord_t *left_out,
                                       lv_coord_t *right_out);
