#ifndef OBD_PRJ_UI_DASHBOARD_CONFIG_H
#define OBD_PRJ_UI_DASHBOARD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "lvgl.h"

void ui_dashboard_config_open(uint8_t gauge_index);
void ui_dashboard_config_reset(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
