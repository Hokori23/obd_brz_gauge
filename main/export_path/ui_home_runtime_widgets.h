#ifndef OBD_PRJ_UI_HOME_RUNTIME_WIDGETS_H
#define OBD_PRJ_UI_HOME_RUNTIME_WIDGETS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "ui_runtime_common.h"

lv_obj_t *ui_home_runtime_widgets_create_menu_card(lv_obj_t *parent,
                                                   lv_coord_t width,
                                                   lv_coord_t min_height,
                                                   lv_coord_t y_offset);
void ui_home_runtime_widgets_apply_add_button_theme(lv_obj_t *btn, lv_coord_t size);
void ui_home_runtime_widgets_apply_slot_typography(lv_obj_t *name_label,
                                                   lv_obj_t *value_label,
                                                   lv_obj_t *unit_label,
                                                   disp_item_t item,
                                                   uint8_t slot_count);
void ui_home_runtime_widgets_create_slot_card(lv_obj_t *parent,
                                              lv_coord_t x,
                                              lv_coord_t y,
                                              lv_coord_t w,
                                              lv_coord_t h,
                                              uint8_t slot_count,
                                              lv_obj_t **name_out,
                                              lv_obj_t **value_out,
                                              lv_obj_t **unit_out);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
