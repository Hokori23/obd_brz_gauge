#ifndef OBD_PRJ_UI_HOME_RUNTIME_WIDGETS_H
#define OBD_PRJ_UI_HOME_RUNTIME_WIDGETS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "ui_runtime_common.h"

#include <stdbool.h>

typedef struct {
    bool valid;
    lv_coord_t panel_x;
    lv_coord_t panel_y;
    lv_coord_t panel_w;
    lv_coord_t panel_h;
    lv_coord_t value_rect_x;
    lv_coord_t value_rect_y;
    lv_coord_t value_rect_w;
    lv_coord_t value_rect_h;
    lv_coord_t name_y;
    lv_coord_t value_y;
    lv_coord_t unit_y;
    lv_coord_t name_w;
    lv_coord_t value_w;
    lv_coord_t unit_w;
    lv_coord_t value_text_w;
    lv_coord_t content_center_x;
    int16_t name_font;
    int16_t value_font;
    int16_t unit_font;
} ui_home_metric_slot_style_t;

typedef struct {
    bool valid;
    lv_coord_t min_x;
    lv_coord_t max_x;
} ui_home_metric_center_interval_t;

lv_obj_t *ui_home_runtime_widgets_create_menu_card(lv_obj_t *parent,
                                                   lv_coord_t width,
                                                   lv_coord_t min_height,
                                                   lv_coord_t y_offset);
void ui_home_runtime_widgets_apply_add_button_theme(lv_obj_t *btn, lv_coord_t size);
void ui_home_runtime_widgets_apply_slot_typography(lv_obj_t *name_label,
                                                   lv_obj_t *value_label,
                                                   lv_obj_t *unit_label,
                                                   disp_item_t item,
                                                   uint8_t slot_count,
                                                   int16_t forced_value_font);
void ui_home_runtime_widgets_apply_dense_slot_style(lv_obj_t *name_label,
                                                    lv_obj_t *value_label,
                                                    lv_obj_t *unit_label,
                                                    const ui_home_metric_slot_style_t *style);
bool ui_home_runtime_widgets_is_dense_slot_count(uint8_t slot_count);
bool ui_home_runtime_widgets_slot_style_center_interval(const ui_home_metric_slot_style_t *style,
                                                       disp_item_t item,
                                                       ui_home_metric_center_interval_t *interval);
void ui_home_runtime_widgets_apply_content_center(ui_home_metric_slot_style_t *style,
                                                  lv_coord_t center_x);
void ui_home_runtime_widgets_build_dense_slot_style(lv_coord_t x,
                                                   lv_coord_t y,
                                                   lv_coord_t w,
                                                   lv_coord_t h,
                                                   uint8_t slot_count,
                                                   lv_coord_t content_center_x,
                                                   disp_item_t item,
                                                   int16_t forced_value_font,
                                                   bool allow_value_span_expand,
                                                   ui_home_metric_slot_style_t *style);
int16_t ui_home_runtime_widgets_measure_slot_value_font(lv_coord_t slot_w,
                                                        lv_coord_t slot_h,
                                                        disp_item_t item,
                                                        uint8_t slot_count);
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
                                              lv_obj_t **unit_out);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
