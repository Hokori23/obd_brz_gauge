#ifndef OBD_PRJ_UI_HOME_RUNTIME_H
#define OBD_PRJ_UI_HOME_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "lvgl.h"
#include "ui_runtime_common.h"

#define UI_HOME_PAGE_MENU_ID 0u

extern lv_obj_t *ui_ScreenPageHome;

void ui_home_runtime_reset(uint8_t initial_page_id);
void ui_home_runtime_screen_init(void);
void ui_home_runtime_show_page(uint8_t page_id, lv_scr_load_anim_t anim);
void ui_home_runtime_refresh_active_tile(void);
uint8_t ui_home_runtime_page_from_default(uint8_t default_page);
void ui_home_runtime_rebuild_and_load(uint8_t page_id, lv_scr_load_anim_t anim);
bool ui_home_runtime_active_page_uses_item(disp_item_t item);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
