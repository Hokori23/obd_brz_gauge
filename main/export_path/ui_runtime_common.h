#ifndef OBD_PRJ_UI_RUNTIME_COMMON_H
#define OBD_PRJ_UI_RUNTIME_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

typedef enum {
    DISP_ITEM_CLT = 0,
    DISP_ITEM_IAT,
    DISP_ITEM_OIL,
    DISP_ITEM_LOAD,
    DISP_ITEM_TPS,
    DISP_ITEM_RPM,
    DISP_ITEM_SPEED,
    DISP_ITEM_BAT,
    DISP_ITEM_OILP,
    DISP_ITEM_BKT,
    DISP_ITEM_BOOST,
    DISP_ITEM_COUNT
} disp_item_t;

bool disp_item_read_value(disp_item_t item,
                          int16_t clt,
                          int16_t iat,
                          int16_t oil,
                          int16_t load_pct,
                          int16_t tps,
                          int32_t bat_mv,
                          int16_t oilp_x10,
                          int16_t brake_x10,
                          uint16_t rpm,
                          uint16_t speed,
                          int16_t boost_x10,
                          int32_t *out);
int32_t disp_item_sweep_value(disp_item_t item, float ratio);
void disp_item_set_text(lv_obj_t *label, disp_item_t item, int32_t value, bool valid);
void disp_item_set_value_color(lv_obj_t *label,
                               disp_item_t item,
                               int32_t value,
                               bool valid,
                               int16_t brake_warn_x10,
                               int16_t oil_warn_x10);
void disp_item_sync_meta(lv_obj_t *name_label,
                         lv_obj_t *unit_label,
                         uint8_t *cache_slot,
                         disp_item_t item);
int ui_runtime_sweep_step_get(void);

void ui_label_set_text_if_changed(lv_obj_t *label, const char *text);
void ui_label_set_text_fmt_if_changed(lv_obj_t *label, const char *fmt, ...);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
