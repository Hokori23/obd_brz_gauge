#include "ui_home_runtime.h"

#include "ui.h"
#include "ui_font_profile.h"
#include "ui_helpers.h"
#include "ui_layout.h"
#include "ui_round_shell.h"
#include "ui_runtime_common.h"

#include <stdio.h>
#include <string.h>

#include "app_obd_dsp/vehicle_profiles.h"
#include "bsp_obd_dsp/elm327_ble_client.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "esp_log.h"

static const char *TAG = "ui_home";

#define UI_NAV_ANIM_MS 200
#define UI_HOME_MAX_TILE_COUNT (1u + UI_DASHBOARD_MAX_PAGES + 1u)
#define UI_HOME_PAGE_PEEK_PX 28
#define UI_HOME_PAGE_MIN_OPA 168
#define UI_HOME_PAGE_MIN_ZOOM 236
#define UI_HOME_PAGE_MAX_Y_SHIFT_PX 8

typedef enum {
    UI_HOME_TILE_MENU = 0,
    UI_HOME_TILE_GAUGE,
    UI_HOME_TILE_ADD
} ui_home_tile_kind_t;

typedef struct {
    ui_home_tile_kind_t kind;
    int8_t gauge_index;
} ui_home_tile_desc_t;

typedef struct {
    lv_obj_t *root;
    lv_obj_t *name_labels[UI_DASHBOARD_MAX_SLOTS];
    lv_obj_t *value_labels[UI_DASHBOARD_MAX_SLOTS];
    lv_obj_t *unit_labels[UI_DASHBOARD_MAX_SLOTS];
    uint8_t item_cache[UI_DASHBOARD_MAX_SLOTS];
    uint8_t slot_count;
    lv_obj_t *menu_vehicle_label;
    lv_obj_t *menu_ble_label;
    lv_obj_t *menu_settings_btn;
} ui_home_tile_runtime_t;

lv_obj_t *ui_ScreenPageHome = NULL;

static lv_obj_t *s_home_tileview = NULL;
static lv_obj_t *s_home_tiles[UI_HOME_MAX_TILE_COUNT];
static lv_obj_t *s_home_content_roots[UI_HOME_MAX_TILE_COUNT];
static bool s_home_tile_mounted[UI_HOME_MAX_TILE_COUNT];
static ui_home_tile_desc_t s_home_tile_descs[UI_HOME_MAX_TILE_COUNT];
static ui_home_tile_runtime_t s_home_tile_runtime[UI_HOME_MAX_TILE_COUNT];
static uint8_t s_home_tile_count = 0;
static uint8_t s_home_active_page = UI_HOME_PAGE_MENU_ID;
static int32_t s_home_last_scroll_left = INT32_MIN;
static int16_t s_home_last_translate_x[UI_HOME_MAX_TILE_COUNT] = {INT16_MIN};
static int16_t s_home_last_translate_y[UI_HOME_MAX_TILE_COUNT] = {INT16_MIN};
static int16_t s_home_last_zoom[UI_HOME_MAX_TILE_COUNT] = {INT16_MIN};
static uint8_t s_home_last_opa[UI_HOME_MAX_TILE_COUNT] = {0xFF};
static bool s_home_edit_mode = false;
static uint8_t s_home_edit_page = UI_HOME_PAGE_MENU_ID;
static lv_obj_t *s_home_edit_overlay = NULL;
static lv_obj_t *s_home_edit_target_root = NULL;
static lv_obj_t *s_home_delete_msgbox = NULL;
static lv_obj_t *s_home_notice_msgbox = NULL;
static lv_obj_t *s_home_screen_pending_delete = NULL;
static lv_obj_t *s_dashboard_config_screen = NULL;
static uint8_t s_dashboard_config_gauge_index = 0;
static lv_obj_t *s_dashboard_cfg_slot_count_roller = NULL;
static lv_obj_t *s_dashboard_cfg_slot_item_rollers[UI_DASHBOARD_MAX_SLOTS] = {0};
static lv_obj_t *s_dashboard_cfg_slot_rows[UI_DASHBOARD_MAX_SLOTS] = {0};

static void ui_home_load(lv_scr_load_anim_t anim, uint8_t page_id);
static void ui_home_rebuild_and_load(uint8_t page_id, lv_scr_load_anim_t anim);
static void ui_home_add_page(void);
static void ui_home_tileview_value_changed(lv_event_t *e);
static void ui_home_tileview_gesture(lv_event_t *e);
static void ui_home_tileview_scroll(lv_event_t *e);
static void ui_home_sync_tile_mounts(uint8_t active_page);
static void ui_home_update_tile_effects(void);
static void ui_home_invalidate_tile_effects(void);
static void ui_home_reset_tile_effect_cache(uint8_t page_id);
static void ui_home_reset_screen_state(void);
static void ui_home_rebuild_tile_descriptors(void);
static void ui_home_reset_runtime(uint8_t tile_id);
static void ui_home_create_menu_content(uint8_t tile_id, lv_obj_t *parent);
static void ui_home_create_add_content(uint8_t tile_id, lv_obj_t *parent);
static void ui_home_create_gauge_content(uint8_t tile_id, lv_obj_t *parent, uint8_t gauge_index);
static void ui_home_menu_open_settings(lv_event_t *e);
static void ui_home_open_menu_overlay(lv_dir_t dir);
static void ui_home_gauge_long_pressed(lv_event_t *e);
static void ui_home_edit_back(lv_event_t *e);
static void ui_home_edit_delete(lv_event_t *e);
static void ui_home_edit_config(lv_event_t *e);
static void ui_home_enter_edit_mode(uint8_t page_id);
static void ui_home_exit_edit_mode(void);
static void ui_home_delete_confirm_result(lv_event_t *e);
static void ui_home_notice_msgbox_event(lv_event_t *e);
static void ui_dashboard_config_screen_init(void);
static void ui_dashboard_config_open(uint8_t gauge_index);
static void ui_dashboard_config_deleted(lv_event_t *e);
static void ui_dashboard_config_close_to_home(lv_scr_load_anim_t anim);
static void ui_dashboard_config_slot_count_changed(lv_event_t *e);
static void ui_dashboard_config_slot_item_changed(lv_event_t *e);
static void ui_dashboard_config_refresh_rows(void);
static void ui_dashboard_config_background(lv_event_t *e);

static void ui_home_screen_change_with_anim(lv_obj_t **target_scr,
                                            lv_scr_load_anim_t anim,
                                            void (*target_init)(void))
{
    _ui_screen_change(target_scr, anim, UI_NAV_ANIM_MS, 0, target_init);
}

static void ui_home_nav_vertical(lv_scr_load_anim_t anim, lv_obj_t **target_scr, void (*target_init)(void))
{
    ui_home_screen_change_with_anim(target_scr, anim, target_init);
}

static uint8_t ui_home_page_from_default_page(uint8_t default_page)
{
    (void)default_page;
    return UI_HOME_PAGE_MENU_ID;
}

static void ui_dashboard_page_set_defaults(ui_dashboard_page_cfg_t *page, uint8_t slot_count)
{
    if (page == NULL) {
        return;
    }

    memset(page, 0, sizeof(*page));
    if (slot_count < 1u || slot_count > UI_DASHBOARD_MAX_SLOTS) {
        slot_count = 1u;
    }
    page->slot_count = slot_count;
    for (uint8_t i = 0; i < UI_DASHBOARD_MAX_SLOTS; ++i) {
        page->slot_items[i] = (i == 0u) ? DISP_ITEM_RPM : DISP_ITEM_SPEED;
    }
}

static const ui_dashboard_page_cfg_t *ui_home_get_gauge_cfg(uint8_t gauge_index)
{
    const ui_dashboard_cfg_t *dashboard = &nvs_cfg_get()->dashboard_cfg;
    if (gauge_index >= dashboard->gauge_page_count || gauge_index >= UI_DASHBOARD_MAX_PAGES) {
        return NULL;
    }
    return &dashboard->pages[gauge_index];
}

static void ui_home_rebuild_tile_descriptors(void)
{
    const ui_dashboard_cfg_t *dashboard = &nvs_cfg_get()->dashboard_cfg;
    s_home_tile_count = (uint8_t)(dashboard->gauge_page_count + 2u);

    s_home_tile_descs[0].kind = UI_HOME_TILE_MENU;
    s_home_tile_descs[0].gauge_index = -1;

    for (uint8_t i = 0; i < dashboard->gauge_page_count; ++i) {
        s_home_tile_descs[i + 1u].kind = UI_HOME_TILE_GAUGE;
        s_home_tile_descs[i + 1u].gauge_index = (int8_t)i;
    }

    s_home_tile_descs[s_home_tile_count - 1u].kind = UI_HOME_TILE_ADD;
    s_home_tile_descs[s_home_tile_count - 1u].gauge_index = -1;
}

static void ui_home_reset_runtime(uint8_t tile_id)
{
    if (tile_id >= UI_HOME_MAX_TILE_COUNT) {
        return;
    }

    memset(&s_home_tile_runtime[tile_id], 0, sizeof(s_home_tile_runtime[tile_id]));
    for (uint8_t i = 0; i < UI_DASHBOARD_MAX_SLOTS; ++i) {
        s_home_tile_runtime[tile_id].item_cache[i] = 0xFF;
    }
}

static void ui_home_reset_tile_effect_cache(uint8_t page_id)
{
    if (page_id >= UI_HOME_MAX_TILE_COUNT) {
        return;
    }

    s_home_last_translate_x[page_id] = INT16_MIN;
    s_home_last_translate_y[page_id] = INT16_MIN;
    s_home_last_zoom[page_id] = INT16_MIN;
    s_home_last_opa[page_id] = 0xFF;
}

static void ui_home_reset_screen_state(void)
{
    memset(s_home_tiles, 0, sizeof(s_home_tiles));
    memset(s_home_content_roots, 0, sizeof(s_home_content_roots));
    memset(s_home_tile_mounted, 0, sizeof(s_home_tile_mounted));
    s_home_last_scroll_left = INT32_MIN;
    for (uint8_t i = 0; i < UI_HOME_MAX_TILE_COUNT; ++i) {
        ui_home_reset_runtime(i);
        ui_home_reset_tile_effect_cache(i);
    }
}

static int32_t ui_home_abs32(int32_t value)
{
    return (value < 0) ? -value : value;
}

static const char *ui_home_gesture_dir_name(lv_dir_t dir)
{
    switch (dir) {
    case LV_DIR_LEFT:
        return "LEFT";
    case LV_DIR_RIGHT:
        return "RIGHT";
    case LV_DIR_TOP:
        return "TOP";
    case LV_DIR_BOTTOM:
        return "BOTTOM";
    default:
        return "NONE";
    }
}

static void ui_home_log_gesture(const char *screen_name, lv_dir_t dir)
{
    ESP_LOGI(TAG, "%s gesture=%s", screen_name, ui_home_gesture_dir_name(dir));
}

static void ui_home_set_active_page(uint8_t page_id, lv_anim_enable_t anim_en)
{
    if (page_id >= s_home_tile_count) {
        page_id = UI_HOME_PAGE_MENU_ID;
    }

    if (s_home_tileview != NULL &&
        s_home_active_page == page_id &&
        lv_tileview_get_tile_act(s_home_tileview) == s_home_tiles[page_id]) {
        ui_home_update_tile_effects();
        return;
    }

    s_home_active_page = page_id;
    ui_home_sync_tile_mounts(page_id);
    if (s_home_tileview != NULL) {
        lv_obj_set_tile_id(s_home_tileview, page_id, 0, anim_en);
        ui_home_update_tile_effects();
    }
}

static lv_obj_t *ui_home_create_content_root(lv_obj_t *tile)
{
    lv_obj_t *root = lv_obj_create(tile);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_center(root);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_update_layout(root);
    lv_obj_set_style_transform_pivot_x(root, lv_obj_get_width(root) / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_y(root, lv_obj_get_height(root) / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_translate_x(root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_translate_y(root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_opa(root, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_zoom(root, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    return root;
}

static void ui_home_menu_open_settings(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ui_home_nav_vertical(LV_SCR_LOAD_ANIM_MOVE_BOTTOM, &ui_ScreenPageSettings, &ui_ScreenPageSettings_screen_init);
}

static void ui_home_add_page_click(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ui_home_add_page();
}

static void ui_home_rebuild_and_load(uint8_t page_id, lv_scr_load_anim_t anim)
{
    if (ui_ScreenPageHome != NULL) {
        if (lv_scr_act() == ui_ScreenPageHome) {
            s_home_screen_pending_delete = ui_ScreenPageHome;
            lv_obj_add_event_cb(ui_ScreenPageHome,
                                scr_unloaded_delete_cb,
                                LV_EVENT_SCREEN_UNLOADED,
                                &s_home_screen_pending_delete);
        } else {
            lv_obj_del(ui_ScreenPageHome);
        }
        ui_ScreenPageHome = NULL;
        s_home_tileview = NULL;
    }

    s_home_edit_mode = false;
    s_home_edit_page = UI_HOME_PAGE_MENU_ID;
    s_home_edit_overlay = NULL;
    s_home_edit_target_root = NULL;
    s_home_delete_msgbox = NULL;
    s_home_notice_msgbox = NULL;
    ui_home_reset_screen_state();
    s_home_active_page = page_id;
    ui_home_load(anim, page_id);
}

static void ui_home_add_page(void)
{
    static const char *btn_texts[] = {"OK", ""};
    nvs_user_cfg_t cfg = *nvs_cfg_get();
    uint8_t page_count = cfg.dashboard_cfg.gauge_page_count;

    if (page_count >= UI_DASHBOARD_MAX_PAGES) {
        if (ui_ScreenPageHome != NULL && s_home_notice_msgbox == NULL) {
            s_home_notice_msgbox = lv_msgbox_create(ui_ScreenPageHome,
                                                    "Page Limit",
                                                    "Maximum 8 dashboard pages.",
                                                    btn_texts,
                                                    false);
            lv_obj_center(s_home_notice_msgbox);
            lv_obj_set_width(s_home_notice_msgbox, ui_layout_px(300));
            lv_obj_add_event_cb(s_home_notice_msgbox, ui_home_notice_msgbox_event, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_add_event_cb(s_home_notice_msgbox, ui_home_notice_msgbox_event, LV_EVENT_DELETE, NULL);
        }
        return;
    }

    ui_dashboard_page_set_defaults(&cfg.dashboard_cfg.pages[page_count], 1u);
    cfg.dashboard_cfg.gauge_page_count = (uint8_t)(page_count + 1u);
    nvs_cfg_set(&cfg);
    ui_home_rebuild_and_load((uint8_t)(UI_HOME_PAGE_MENU_ID + page_count + 1u),
                             LV_SCR_LOAD_ANIM_MOVE_LEFT);
}

static void ui_home_notice_msgbox_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_DELETE) {
        s_home_notice_msgbox = NULL;
        return;
    }
    if (code == LV_EVENT_VALUE_CHANGED && s_home_notice_msgbox != NULL) {
        lv_obj_del(s_home_notice_msgbox);
        s_home_notice_msgbox = NULL;
    }
}

static void ui_home_format_ble_name(char *buf, size_t buf_size, const char *name)
{
    if (buf == NULL || buf_size == 0u) {
        return;
    }
    if (name == NULL || name[0] == '\0') {
        strlcpy(buf, "Not set", buf_size);
        return;
    }

    if (strlen(name) <= 5u) {
        strlcpy(buf, name, buf_size);
        return;
    }

    snprintf(buf, buf_size, "%.5s...", name);
}

static void ui_home_create_menu_content(uint8_t tile_id, lv_obj_t *parent)
{
    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "MENU");
    lv_obj_set_style_text_font(title, ui_font_typoder(24), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, ui_layout_px(26));

    lv_obj_t *vehicle = lv_label_create(parent);
    lv_label_set_text(vehicle, "--");
    lv_obj_set_style_text_font(vehicle, ui_font_typoder(32), LV_PART_MAIN);
    lv_obj_set_style_text_color(vehicle, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_width(vehicle, ui_layout_px(300));
    lv_obj_set_style_text_align(vehicle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(vehicle, LV_ALIGN_CENTER, 0, ui_layout_px(-56));

    lv_obj_t *ble = lv_label_create(parent);
    lv_label_set_text(ble, "BLE: Not set");
    lv_obj_set_style_text_font(ble, ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_color(ble, lv_color_hex(0xAFAFAF), LV_PART_MAIN);
    lv_obj_set_width(ble, ui_layout_px(280));
    lv_obj_set_style_text_align(ble, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(ble, LV_ALIGN_CENTER, 0, ui_layout_px(6));

    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, ui_layout_px(180), ui_layout_px(48));
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, ui_layout_px(86));
    lv_obj_set_style_radius(btn, ui_layout_px(18), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x202020), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_add_event_cb(btn, ui_home_menu_open_settings, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "SETTINGS");
    lv_obj_set_style_text_font(btn_label, ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(btn_label);

    rt->root = parent;
    rt->menu_vehicle_label = vehicle;
    rt->menu_ble_label = ble;
    rt->menu_settings_btn = btn;
}

static void ui_home_create_slot_card(lv_obj_t *parent,
                                     lv_coord_t x,
                                     lv_coord_t y,
                                     lv_coord_t w,
                                     lv_coord_t h,
                                     lv_obj_t **name_out,
                                     lv_obj_t **value_out,
                                     lv_obj_t **unit_out)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, w, h);
    lv_obj_align(panel, LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(panel, ui_layout_px(18), LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x101010), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x2E2E2E), LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, ui_layout_px(14), LV_PART_MAIN);

    lv_obj_t *name = lv_label_create(panel);
    lv_label_set_text(name, "RPM");
    lv_obj_set_style_text_font(name, ui_font_typoder(16), LV_PART_MAIN);
    lv_obj_set_style_text_color(name, lv_color_hex(0x9A9A9A), LV_PART_MAIN);
    lv_obj_align(name, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *value = lv_label_create(panel);
    lv_label_set_text(value, "--");
    lv_obj_set_style_text_font(value, ui_font_typoder(36), LV_PART_MAIN);
    lv_obj_set_style_text_color(value, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(value, LV_ALIGN_CENTER, 0, ui_layout_px(2));

    lv_obj_t *unit = lv_label_create(panel);
    lv_label_set_text(unit, "");
    lv_obj_set_style_text_font(unit, ui_font_typoder(16), LV_PART_MAIN);
    lv_obj_set_style_text_color(unit, lv_color_hex(0x707070), LV_PART_MAIN);
    lv_obj_align(unit, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    *name_out = name;
    *value_out = value;
    *unit_out = unit;
}

static void ui_home_create_gauge_content(uint8_t tile_id, lv_obj_t *parent, uint8_t gauge_index)
{
    typedef struct {
        lv_coord_t x;
        lv_coord_t y;
        lv_coord_t w;
        lv_coord_t h;
    } ui_home_slot_layout_t;

    const ui_dashboard_page_cfg_t *page = ui_home_get_gauge_cfg(gauge_index);
    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];
    ui_home_slot_layout_t layouts[UI_DASHBOARD_MAX_SLOTS] = {0};

    if (page == NULL) {
        return;
    }

    rt->root = parent;
    rt->slot_count = page->slot_count;

    switch (page->slot_count) {
    case 1:
        layouts[0] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(58), ui_layout_px(396), ui_layout_px(240)};
        break;
    case 2:
        layouts[0] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(30), ui_layout_px(396), ui_layout_px(156)};
        layouts[1] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(214), ui_layout_px(396), ui_layout_px(156)};
        break;
    case 3:
        layouts[0] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(30), ui_layout_px(188), ui_layout_px(156)};
        layouts[1] = (ui_home_slot_layout_t){ui_layout_px(243), ui_layout_px(30), ui_layout_px(188), ui_layout_px(156)};
        layouts[2] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(214), ui_layout_px(396), ui_layout_px(156)};
        break;
    case 4:
        layouts[0] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(30), ui_layout_px(188), ui_layout_px(156)};
        layouts[1] = (ui_home_slot_layout_t){ui_layout_px(243), ui_layout_px(30), ui_layout_px(188), ui_layout_px(156)};
        layouts[2] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(214), ui_layout_px(188), ui_layout_px(156)};
        layouts[3] = (ui_home_slot_layout_t){ui_layout_px(243), ui_layout_px(214), ui_layout_px(188), ui_layout_px(156)};
        break;
    case 5:
        layouts[0] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(18), ui_layout_px(188), ui_layout_px(112)};
        layouts[1] = (ui_home_slot_layout_t){ui_layout_px(243), ui_layout_px(18), ui_layout_px(188), ui_layout_px(112)};
        layouts[2] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(144), ui_layout_px(188), ui_layout_px(112)};
        layouts[3] = (ui_home_slot_layout_t){ui_layout_px(243), ui_layout_px(144), ui_layout_px(188), ui_layout_px(112)};
        layouts[4] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(270), ui_layout_px(396), ui_layout_px(100)};
        break;
    default:
        layouts[0] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(18), ui_layout_px(188), ui_layout_px(100)};
        layouts[1] = (ui_home_slot_layout_t){ui_layout_px(243), ui_layout_px(18), ui_layout_px(188), ui_layout_px(100)};
        layouts[2] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(142), ui_layout_px(188), ui_layout_px(100)};
        layouts[3] = (ui_home_slot_layout_t){ui_layout_px(243), ui_layout_px(142), ui_layout_px(188), ui_layout_px(100)};
        layouts[4] = (ui_home_slot_layout_t){ui_layout_px(35), ui_layout_px(266), ui_layout_px(188), ui_layout_px(100)};
        layouts[5] = (ui_home_slot_layout_t){ui_layout_px(243), ui_layout_px(266), ui_layout_px(188), ui_layout_px(100)};
        break;
    }

    for (uint8_t i = 0; i < page->slot_count; ++i) {
        ui_home_create_slot_card(parent,
                                 layouts[i].x,
                                 layouts[i].y,
                                 layouts[i].w,
                                 layouts[i].h,
                                 &rt->name_labels[i],
                                 &rt->value_labels[i],
                                 &rt->unit_labels[i]);
    }
}

static void ui_home_create_add_content(uint8_t tile_id, lv_obj_t *parent)
{
    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];

    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, ui_layout_px(240), ui_layout_px(240));
    lv_obj_center(btn);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x2A2A2A), LV_PART_MAIN);
    lv_obj_add_event_cb(btn, ui_home_add_page_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *plus = lv_label_create(btn);
    lv_label_set_text(plus, "+");
    lv_obj_set_style_text_font(plus, ui_font_typoder(100), LV_PART_MAIN);
    lv_obj_set_style_text_color(plus, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(plus);

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "ADD PAGE");
    lv_obj_set_style_text_font(hint, ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x8C8C8C), LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, ui_layout_px(146));

    rt->root = parent;
}

static void ui_home_gauge_long_pressed(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_LONG_PRESSED) {
        return;
    }
    ui_home_enter_edit_mode((uint8_t)(uintptr_t)lv_event_get_user_data(e));
}

static void ui_home_edit_back(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ui_home_exit_edit_mode();
}

static void ui_home_edit_delete(lv_event_t *e)
{
    static const char *btn_texts[] = {"Cancel", "Delete", ""};

    if (lv_event_get_code(e) != LV_EVENT_CLICKED || ui_ScreenPageHome == NULL || s_home_delete_msgbox != NULL) {
        return;
    }

    s_home_delete_msgbox = lv_msgbox_create(ui_ScreenPageHome,
                                            "Delete Page",
                                            "Delete this dashboard page?",
                                            btn_texts,
                                            false);
    lv_obj_center(s_home_delete_msgbox);
    lv_obj_set_width(s_home_delete_msgbox, ui_layout_px(300));
    lv_obj_set_style_bg_color(s_home_delete_msgbox, lv_color_hex(0x181818), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_home_delete_msgbox, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_home_delete_msgbox, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_home_delete_msgbox, ui_layout_px(18), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_home_delete_msgbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_add_event_cb(s_home_delete_msgbox, ui_home_delete_confirm_result, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_home_delete_msgbox, ui_home_delete_confirm_result, LV_EVENT_DELETE, NULL);
}

static void ui_home_edit_config(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || s_home_edit_page == UI_HOME_PAGE_MENU_ID) {
        return;
    }

    ui_home_exit_edit_mode();
    ui_dashboard_config_open((uint8_t)(s_home_edit_page - 1u));
}

static void ui_home_enter_edit_mode(uint8_t page_id)
{
    ui_home_tile_runtime_t *rt;

    if (s_home_edit_mode || page_id >= s_home_tile_count || page_id != s_home_active_page) {
        return;
    }
    if (s_home_tile_descs[page_id].kind != UI_HOME_TILE_GAUGE || s_home_tiles[page_id] == NULL) {
        return;
    }

    rt = &s_home_tile_runtime[page_id];
    if (rt->root == NULL) {
        return;
    }

    s_home_edit_mode = true;
    s_home_edit_page = page_id;
    s_home_edit_target_root = rt->root;

    if (s_home_tileview != NULL) {
        lv_obj_clear_flag(s_home_tileview, LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_set_style_transform_zoom(rt->root, (lv_coord_t)(256 * 12 / 10), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_translate_x(rt->root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_translate_y(rt->root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_opa(rt->root, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_home_edit_overlay = lv_obj_create(s_home_tiles[page_id]);
    lv_obj_remove_style_all(s_home_edit_overlay);
    lv_obj_set_size(s_home_edit_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_home_edit_overlay);
    lv_obj_set_style_bg_color(s_home_edit_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_home_edit_overlay, 96, LV_PART_MAIN);
    lv_obj_clear_flag(s_home_edit_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_home_edit_overlay, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *zone = lv_obj_create(s_home_edit_overlay);
    lv_obj_remove_style_all(zone);
    lv_obj_set_size(zone, LV_PCT(50), LV_PCT(100));
    lv_obj_align(zone, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(zone, lv_color_hex(0x27AE60), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(zone, 160, LV_PART_MAIN);
    lv_obj_clear_flag(zone, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(zone, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(zone, ui_home_edit_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(zone);
    lv_label_set_text(label, "BACK");
    lv_obj_set_style_text_font(label, ui_font_typoder(24), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, ui_layout_px(24), ui_layout_px(92));

    zone = lv_obj_create(s_home_edit_overlay);
    lv_obj_remove_style_all(zone);
    lv_obj_set_size(zone, LV_PCT(50), LV_PCT(100));
    lv_obj_align(zone, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(zone, lv_color_hex(0xEB5757), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(zone, 160, LV_PART_MAIN);
    lv_obj_clear_flag(zone, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(zone, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(zone, ui_home_edit_delete, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(zone);
    lv_label_set_text(label, "DELETE");
    lv_obj_set_style_text_font(label, ui_font_typoder(24), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, ui_layout_px(-24), ui_layout_px(92));

    lv_obj_t *edit_btn = lv_btn_create(s_home_edit_overlay);
    lv_obj_set_size(edit_btn, ui_layout_px(132), ui_layout_px(48));
    lv_obj_align(edit_btn, LV_ALIGN_TOP_MID, 0, ui_layout_px(18));
    lv_obj_set_style_radius(edit_btn, ui_layout_px(24), LV_PART_MAIN);
    lv_obj_set_style_bg_color(edit_btn, lv_color_hex(0x2F80ED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(edit_btn, 160, LV_PART_MAIN);
    lv_obj_set_style_border_width(edit_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(edit_btn, ui_home_edit_config, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(edit_btn);
    lv_label_set_text(label, "EDIT");
    lv_obj_set_style_text_font(label, ui_font_typoder(24), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(label);
}

static void ui_home_exit_edit_mode(void)
{
    if (!s_home_edit_mode) {
        return;
    }

    if (s_home_delete_msgbox != NULL) {
        lv_obj_del(s_home_delete_msgbox);
        s_home_delete_msgbox = NULL;
    }
    if (s_home_edit_overlay != NULL) {
        lv_obj_del(s_home_edit_overlay);
        s_home_edit_overlay = NULL;
    }
    if (s_home_tileview != NULL) {
        lv_obj_add_flag(s_home_tileview, LV_OBJ_FLAG_SCROLLABLE);
    }

    s_home_edit_mode = false;
    s_home_edit_page = UI_HOME_PAGE_MENU_ID;
    s_home_edit_target_root = NULL;
    ui_home_invalidate_tile_effects();
    ui_home_update_tile_effects();
}

static void ui_home_delete_confirm_result(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_DELETE) {
        s_home_delete_msgbox = NULL;
        return;
    }
    if (code != LV_EVENT_VALUE_CHANGED || s_home_delete_msgbox == NULL) {
        return;
    }

    if (lv_msgbox_get_active_btn(s_home_delete_msgbox) == 1) {
        nvs_user_cfg_t cfg = *nvs_cfg_get();
        uint8_t gauge_index = (s_home_edit_page > 0u) ? (uint8_t)(s_home_edit_page - 1u) : 0u;
        uint8_t page_count = cfg.dashboard_cfg.gauge_page_count;
        uint8_t target_page = UI_HOME_PAGE_MENU_ID;

        if (gauge_index < page_count) {
            for (uint8_t i = gauge_index; (uint8_t)(i + 1u) < page_count; ++i) {
                cfg.dashboard_cfg.pages[i] = cfg.dashboard_cfg.pages[i + 1u];
            }
            if (page_count > 0u) {
                memset(&cfg.dashboard_cfg.pages[page_count - 1u], 0, sizeof(cfg.dashboard_cfg.pages[page_count - 1u]));
                cfg.dashboard_cfg.gauge_page_count = (uint8_t)(page_count - 1u);
            }
            nvs_cfg_set(&cfg);

            if (cfg.dashboard_cfg.gauge_page_count == 0u) {
                target_page = UI_HOME_PAGE_MENU_ID;
            } else if (gauge_index > 0u) {
                target_page = gauge_index;
            } else {
                target_page = 1u;
            }
        }

        ui_home_exit_edit_mode();
        ui_home_rebuild_and_load(target_page, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
        return;
    }

    lv_obj_del(s_home_delete_msgbox);
    s_home_delete_msgbox = NULL;
}

static void ui_home_mount_page(uint8_t page_id)
{
    if (page_id >= s_home_tile_count || s_home_tile_mounted[page_id] || s_home_tiles[page_id] == NULL) {
        return;
    }

    lv_obj_t *root = ui_home_create_content_root(s_home_tiles[page_id]);
    s_home_content_roots[page_id] = root;
    ui_home_reset_runtime(page_id);
    ui_home_reset_tile_effect_cache(page_id);

    switch (s_home_tile_descs[page_id].kind) {
    case UI_HOME_TILE_MENU:
        ui_home_create_menu_content(page_id, root);
        break;
    case UI_HOME_TILE_GAUGE:
        ui_home_create_gauge_content(page_id, root, (uint8_t)s_home_tile_descs[page_id].gauge_index);
        lv_obj_add_event_cb(s_home_tiles[page_id],
                            ui_home_gauge_long_pressed,
                            LV_EVENT_LONG_PRESSED,
                            (void *)(uintptr_t)page_id);
        break;
    case UI_HOME_TILE_ADD:
        ui_home_create_add_content(page_id, root);
        break;
    }

    s_home_tile_mounted[page_id] = true;
}

static void ui_home_unmount_page(uint8_t page_id)
{
    if (page_id >= UI_HOME_MAX_TILE_COUNT || !s_home_tile_mounted[page_id]) {
        return;
    }

    ui_home_reset_tile_effect_cache(page_id);
    ui_home_reset_runtime(page_id);
    if (s_home_content_roots[page_id] != NULL) {
        lv_obj_del(s_home_content_roots[page_id]);
        s_home_content_roots[page_id] = NULL;
    }
    s_home_tile_mounted[page_id] = false;
}

static void ui_home_sync_tile_mounts(uint8_t active_page)
{
    for (uint8_t i = 0; i < s_home_tile_count; ++i) {
        bool should_mount = (i == active_page) ||
                            (active_page > 0u && i == (uint8_t)(active_page - 1u)) ||
                            (active_page + 1u < s_home_tile_count && i == (uint8_t)(active_page + 1u));
        if (should_mount) {
            ui_home_mount_page(i);
        } else {
            ui_home_unmount_page(i);
        }
    }

    ui_home_invalidate_tile_effects();
}

static void ui_home_invalidate_tile_effects(void)
{
    s_home_last_scroll_left = INT32_MIN;
}

static void ui_home_update_tile_effects(void)
{
    if (s_home_tileview == NULL) {
        return;
    }

    lv_coord_t tile_width = lv_obj_get_content_width(s_home_tileview);
    if (tile_width <= 0) {
        return;
    }

    int32_t scroll_left = lv_obj_get_scroll_left(s_home_tileview);
    if (scroll_left == s_home_last_scroll_left) {
        return;
    }

    for (uint8_t i = 0; i < s_home_tile_count; ++i) {
        lv_obj_t *root = s_home_content_roots[i];
        if (root == NULL) {
            continue;
        }

        int32_t offset = ((int32_t)i * tile_width) - scroll_left;
        int32_t abs_offset = ui_home_abs32(offset);
        if (abs_offset > tile_width) {
            abs_offset = tile_width;
        }

        int32_t visible = tile_width - abs_offset;
        int32_t eased_visible = (visible * visible + (tile_width / 2)) / tile_width;
        int32_t translate_x = (-offset * UI_HOME_PAGE_PEEK_PX) / tile_width;
        int32_t translate_y = (abs_offset * UI_HOME_PAGE_MAX_Y_SHIFT_PX) / tile_width;
        int32_t opa = UI_HOME_PAGE_MIN_OPA +
                      (eased_visible * (LV_OPA_COVER - UI_HOME_PAGE_MIN_OPA)) / tile_width;
        int32_t zoom = UI_HOME_PAGE_MIN_ZOOM +
                       (eased_visible * (256 - UI_HOME_PAGE_MIN_ZOOM)) / tile_width;

        if (s_home_last_translate_x[i] != (int16_t)translate_x) {
            lv_obj_set_style_translate_x(root, (lv_coord_t)translate_x, LV_PART_MAIN | LV_STATE_DEFAULT);
            s_home_last_translate_x[i] = (int16_t)translate_x;
        }
        if (s_home_last_translate_y[i] != (int16_t)translate_y) {
            lv_obj_set_style_translate_y(root, (lv_coord_t)translate_y, LV_PART_MAIN | LV_STATE_DEFAULT);
            s_home_last_translate_y[i] = (int16_t)translate_y;
        }
        if (s_home_last_opa[i] != (uint8_t)opa) {
            lv_obj_set_style_opa(root, (lv_opa_t)opa, LV_PART_MAIN | LV_STATE_DEFAULT);
            s_home_last_opa[i] = (uint8_t)opa;
        }
        if (s_home_last_zoom[i] != (int16_t)zoom) {
            lv_obj_set_style_transform_zoom(root, (lv_coord_t)zoom, LV_PART_MAIN | LV_STATE_DEFAULT);
            s_home_last_zoom[i] = (int16_t)zoom;
        }
    }

    s_home_last_scroll_left = scroll_left;
}

void ui_home_runtime_refresh_active_tile(void)
{
    const nvs_user_cfg_t *user_cfg = nvs_cfg_get();
    const vehicle_profile_t *vehicle = vehicle_profile_get_active();
    const char *vehicle_name = (vehicle && vehicle->name) ? vehicle->name : "VEHICLE";
    const char *ble_name = elm327_ble_get_connected_name();
    char ble_short[16];
    int16_t clt = obd_data_get_coolant_temp();
    int16_t iat = obd_data_get_intake_temp();
    int16_t oil = obd_data_get_oil_temp();
    int16_t oilp_x10 = obd_data_get_oil_pressure_x10();
    int16_t brake_x10 = obd_data_get_brake_temp_x10();
    int16_t load_pct = obd_data_get_load_pct();
    int16_t tps = obd_data_get_tps();
    int32_t bat_mv = obd_data_get_bat_mv();
    int16_t boost_x10 = obd_data_get_boost_x10();
    uint16_t rpm = obd_data_get_rpm();
    uint16_t speed = obd_data_get_speed();
    int16_t brake_warn_x10 = (int16_t)(user_cfg->brake_temp_warn_c * 10);
    int16_t oil_warn_x10 = (int16_t)user_cfg->oil_pressure_warn_x10;
    float sweep_ratio = -1.0f;
    int sweep_step = ui_runtime_sweep_step_get();

    if (sweep_step > 0) {
        int step = sweep_step - 1;
        if (step <= 6) {
            sweep_ratio = (float)step / 6.0f;
        } else {
            sweep_ratio = (float)(12 - step) / 6.0f;
        }
    }

    if ((ble_name == NULL || ble_name[0] == '\0') && user_cfg->ble_device_name[0] != '\0') {
        ble_name = user_cfg->ble_device_name;
    }
    ui_home_format_ble_name(ble_short, sizeof(ble_short), ble_name);

    for (uint8_t tile_id = 0; tile_id < s_home_tile_count; ++tile_id) {
        ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];
        if (!s_home_tile_mounted[tile_id]) {
            continue;
        }

        switch (s_home_tile_descs[tile_id].kind) {
        case UI_HOME_TILE_MENU:
            ui_label_set_text_if_changed(rt->menu_vehicle_label, vehicle_name);
            ui_label_set_text_fmt_if_changed(rt->menu_ble_label, "BLE: %s", ble_short);
            break;
        case UI_HOME_TILE_GAUGE: {
            const ui_dashboard_page_cfg_t *page =
                ui_home_get_gauge_cfg((uint8_t)s_home_tile_descs[tile_id].gauge_index);
            if (page == NULL) {
                break;
            }
            for (uint8_t i = 0; i < rt->slot_count && i < page->slot_count; ++i) {
                disp_item_t item = (disp_item_t)(page->slot_items[i] % DISP_ITEM_COUNT);
                int32_t value = 0;
                bool valid = false;

                disp_item_sync_meta(rt->name_labels[i], rt->unit_labels[i], &rt->item_cache[i], item);
                if (sweep_ratio >= 0.0f) {
                    value = disp_item_sweep_value(item, sweep_ratio);
                    valid = true;
                } else {
                    valid = disp_item_read_value(item,
                                                 clt, iat, oil, load_pct, tps, bat_mv,
                                                 oilp_x10, brake_x10, rpm, speed, boost_x10, &value);
                }
                disp_item_set_text(rt->value_labels[i], item, value, valid);
                disp_item_set_value_color(rt->value_labels[i], item, value, valid,
                                          brake_warn_x10, oil_warn_x10);
            }
            break;
        }
        case UI_HOME_TILE_ADD:
        default:
            break;
        }
    }
}

static void ui_dashboard_config_refresh_rows(void)
{
    uint8_t slot_count = 1u;

    if (s_dashboard_cfg_slot_count_roller != NULL) {
        slot_count = (uint8_t)(lv_roller_get_selected(s_dashboard_cfg_slot_count_roller) + 1u);
    }

    for (uint8_t i = 0; i < UI_DASHBOARD_MAX_SLOTS; ++i) {
        if (s_dashboard_cfg_slot_rows[i] == NULL) {
            continue;
        }
        if (i < slot_count) {
            lv_obj_clear_flag(s_dashboard_cfg_slot_rows[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_dashboard_cfg_slot_rows[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void ui_dashboard_config_slot_count_changed(lv_event_t *e)
{
    nvs_user_cfg_t cfg;
    uint8_t slot_count;
    const ui_dashboard_page_cfg_t *current_page;

    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    cfg = *nvs_cfg_get();
    if (s_dashboard_config_gauge_index >= cfg.dashboard_cfg.gauge_page_count) {
        return;
    }

    slot_count = (uint8_t)(lv_roller_get_selected(s_dashboard_cfg_slot_count_roller) + 1u);
    current_page = &cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index];
    if (slot_count > current_page->slot_count) {
        for (uint8_t i = current_page->slot_count; i < slot_count; ++i) {
            cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index].slot_items[i] = DISP_ITEM_RPM;
            if (s_dashboard_cfg_slot_item_rollers[i] != NULL) {
                lv_roller_set_selected(s_dashboard_cfg_slot_item_rollers[i], DISP_ITEM_RPM, LV_ANIM_OFF);
            }
        }
    }

    cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index].slot_count = slot_count;
    nvs_cfg_set(&cfg);
    ui_dashboard_config_refresh_rows();
}

static void ui_dashboard_config_slot_item_changed(lv_event_t *e)
{
    nvs_user_cfg_t cfg;
    uint8_t slot_index;

    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    cfg = *nvs_cfg_get();
    if (s_dashboard_config_gauge_index >= cfg.dashboard_cfg.gauge_page_count) {
        return;
    }

    slot_index = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    if (slot_index >= UI_DASHBOARD_MAX_SLOTS) {
        return;
    }

    cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index].slot_items[slot_index] =
        (uint8_t)lv_roller_get_selected(s_dashboard_cfg_slot_item_rollers[slot_index]);
    nvs_cfg_set(&cfg);
}

static void ui_dashboard_config_background(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
        return;
    }

    if (lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_TOP) {
        lv_indev_wait_release(lv_indev_get_act());
        ui_dashboard_config_close_to_home(LV_SCR_LOAD_ANIM_MOVE_TOP);
    }
}

static void ui_dashboard_config_deleted(lv_event_t *e)
{
    LV_UNUSED(e);
    s_dashboard_config_screen = NULL;
    memset(s_dashboard_cfg_slot_item_rollers, 0, sizeof(s_dashboard_cfg_slot_item_rollers));
    memset(s_dashboard_cfg_slot_rows, 0, sizeof(s_dashboard_cfg_slot_rows));
    s_dashboard_cfg_slot_count_roller = NULL;
}

static void ui_dashboard_config_screen_init(void)
{
    const char *slot_count_options = "1\n2\n3\n4\n5\n6";
    char item_options[96] = {0};
    const ui_dashboard_page_cfg_t *page;
    lv_coord_t row_y[UI_DASHBOARD_MAX_SLOTS] = {
        ui_layout_px(-102), ui_layout_px(-58), ui_layout_px(-14),
        ui_layout_px(30), ui_layout_px(74), ui_layout_px(118)
    };

    if (s_dashboard_config_screen != NULL) {
        return;
    }

    for (uint8_t i = 0; i < DISP_ITEM_COUNT; ++i) {
        if (i > 0u) {
            strlcat(item_options, "\n", sizeof(item_options));
        }
        strlcat(item_options, ui_disp_item_name(i), sizeof(item_options));
    }

    page = ui_home_get_gauge_cfg(s_dashboard_config_gauge_index);
    if (page == NULL) {
        return;
    }

    s_dashboard_config_screen = lv_obj_create(NULL);
    ui_round_screen_apply_base(s_dashboard_config_screen, lv_color_hex(0x000000));
    lv_obj_add_event_cb(s_dashboard_config_screen, ui_dashboard_config_background, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(s_dashboard_config_screen,
                        scr_unloaded_delete_cb,
                        LV_EVENT_SCREEN_UNLOADED,
                        &s_dashboard_config_screen);
    lv_obj_add_event_cb(s_dashboard_config_screen, ui_dashboard_config_deleted, LV_EVENT_DELETE, NULL);

    lv_obj_t *title = lv_label_create(s_dashboard_config_screen);
    lv_label_set_text(title, "DASHBOARD");
    lv_obj_set_style_text_font(title, ui_font_typoder(24), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, ui_layout_px(20));

    lv_obj_t *count_label = lv_label_create(s_dashboard_config_screen);
    lv_label_set_text(count_label, "SLOTS");
    lv_obj_set_style_text_font(count_label, ui_font_hint(12), LV_PART_MAIN);
    lv_obj_set_style_text_color(count_label, lv_color_hex(0x7A7A7A), LV_PART_MAIN);
    lv_obj_align(count_label, LV_ALIGN_CENTER, ui_layout_px(-130), ui_layout_px(-154));

    s_dashboard_cfg_slot_count_roller = lv_roller_create(s_dashboard_config_screen);
    lv_roller_set_options(s_dashboard_cfg_slot_count_roller, slot_count_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_dashboard_cfg_slot_count_roller, 1);
    lv_roller_set_selected(s_dashboard_cfg_slot_count_roller, (uint16_t)(page->slot_count - 1u), LV_ANIM_OFF);
    lv_obj_set_size(s_dashboard_cfg_slot_count_roller, ui_layout_px(120), ui_layout_px(42));
    lv_obj_align(s_dashboard_cfg_slot_count_roller, LV_ALIGN_CENTER, ui_layout_px(72), ui_layout_px(-154));
    lv_obj_clear_flag(s_dashboard_cfg_slot_count_roller, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_text_font(s_dashboard_cfg_slot_count_roller, ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_dashboard_cfg_slot_count_roller, ui_font_typoder(20), LV_PART_SELECTED);
    ui_round_shell_apply_roller_theme(s_dashboard_cfg_slot_count_roller,
                                      ui_layout_px(18),
                                      lv_color_hex(0x202020),
                                      lv_color_hex(0x444444),
                                      lv_color_hex(0xCFCFCF),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0x000000));
    lv_obj_add_event_cb(s_dashboard_cfg_slot_count_roller,
                        ui_dashboard_config_slot_count_changed,
                        LV_EVENT_VALUE_CHANGED,
                        NULL);

    for (uint8_t i = 0; i < UI_DASHBOARD_MAX_SLOTS; ++i) {
        lv_obj_t *row = lv_obj_create(s_dashboard_config_screen);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, ui_layout_px(360), ui_layout_px(36));
        lv_obj_align(row, LV_ALIGN_CENTER, 0, row_y[i]);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        s_dashboard_cfg_slot_rows[i] = row;

        lv_obj_t *label = lv_label_create(row);
        lv_label_set_text_fmt(label, "SLOT %u", (unsigned)(i + 1u));
        lv_obj_set_style_text_font(label, ui_font_hint(12), LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(0x7A7A7A), LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

        s_dashboard_cfg_slot_item_rollers[i] = lv_roller_create(row);
        lv_roller_set_options(s_dashboard_cfg_slot_item_rollers[i], item_options, LV_ROLLER_MODE_NORMAL);
        lv_roller_set_visible_row_count(s_dashboard_cfg_slot_item_rollers[i], 1);
        lv_roller_set_selected(s_dashboard_cfg_slot_item_rollers[i], page->slot_items[i] % DISP_ITEM_COUNT, LV_ANIM_OFF);
        lv_obj_set_size(s_dashboard_cfg_slot_item_rollers[i], ui_layout_px(180), ui_layout_px(36));
        lv_obj_align(s_dashboard_cfg_slot_item_rollers[i], LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_clear_flag(s_dashboard_cfg_slot_item_rollers[i], LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_set_style_text_font(s_dashboard_cfg_slot_item_rollers[i], ui_font_typoder(16), LV_PART_MAIN);
        lv_obj_set_style_text_font(s_dashboard_cfg_slot_item_rollers[i], ui_font_typoder(16), LV_PART_SELECTED);
        ui_round_shell_apply_roller_theme(s_dashboard_cfg_slot_item_rollers[i],
                                          ui_layout_px(16),
                                          lv_color_hex(0x202020),
                                          lv_color_hex(0x444444),
                                          lv_color_hex(0xCFCFCF),
                                          lv_color_hex(0xFFFFFF),
                                          lv_color_hex(0xFFFFFF),
                                          lv_color_hex(0x000000));
        lv_obj_add_event_cb(s_dashboard_cfg_slot_item_rollers[i],
                            ui_dashboard_config_slot_item_changed,
                            LV_EVENT_VALUE_CHANGED,
                            (void *)(uintptr_t)i);
    }

    lv_obj_t *hint = lv_label_create(s_dashboard_config_screen);
    lv_label_set_text(hint, "Swipe up to go back");
    lv_obj_set_style_text_font(hint, ui_font_hint(12), LV_PART_MAIN);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x555555), LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, ui_layout_px(-26));

    ui_dashboard_config_refresh_rows();
}

static void ui_dashboard_config_open(uint8_t gauge_index)
{
    s_dashboard_config_gauge_index = gauge_index;
    ui_home_screen_change_with_anim(&s_dashboard_config_screen,
                                    LV_SCR_LOAD_ANIM_MOVE_BOTTOM,
                                    ui_dashboard_config_screen_init);
}

static void ui_dashboard_config_close_to_home(lv_scr_load_anim_t anim)
{
    ui_home_rebuild_and_load((uint8_t)(s_dashboard_config_gauge_index + 1u), anim);
}

static void ui_home_tileview_value_changed(lv_event_t *e)
{
    if (lv_event_get_target(e) != s_home_tileview) {
        return;
    }

    lv_obj_t *active_tile = lv_tileview_get_tile_act(s_home_tileview);
    for (uint8_t i = 0; i < s_home_tile_count; ++i) {
        if (s_home_tiles[i] == active_tile) {
            if (s_home_active_page != i) {
                s_home_active_page = i;
                ui_home_sync_tile_mounts(i);
            }
            ui_home_update_tile_effects();
            ESP_LOGI(TAG, "Home tile active=%u", (unsigned)i);
            break;
        }
    }
}

static void ui_home_tileview_scroll(lv_event_t *e)
{
    if (lv_event_get_target(e) != s_home_tileview) {
        return;
    }
    ui_home_update_tile_effects();
}

static void ui_home_tileview_gesture(lv_event_t *e)
{
    LV_UNUSED(e);
    if (s_home_edit_mode) {
        return;
    }

    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    ui_home_log_gesture("Home", dir);
    if (dir == LV_DIR_TOP || dir == LV_DIR_BOTTOM) {
        lv_indev_wait_release(lv_indev_get_act());
        if (s_home_active_page == UI_HOME_PAGE_MENU_ID) {
            ui_home_open_menu_overlay(dir);
        }
    }
}

static void ui_home_open_menu_overlay(lv_dir_t dir)
{
    if (dir == LV_DIR_TOP) {
        ui_home_nav_vertical(LV_SCR_LOAD_ANIM_MOVE_TOP, &ui_ScreenPageBLEScan, &ui_ScreenPageBLEScan_screen_init);
    } else if (dir == LV_DIR_BOTTOM) {
        ui_home_nav_vertical(LV_SCR_LOAD_ANIM_MOVE_BOTTOM, &ui_ScreenPageSettings, &ui_ScreenPageSettings_screen_init);
    }
}

static lv_obj_t *ui_home_create_tile(lv_obj_t *parent, uint8_t col, lv_dir_t dir)
{
    lv_obj_t *tile = lv_tileview_add_tile(parent, col, 0, dir);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(tile, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(tile, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(tile, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    return tile;
}

void ui_home_runtime_screen_init(void)
{
    if (ui_ScreenPageHome != NULL) {
        return;
    }

    ui_home_rebuild_tile_descriptors();
    ui_home_reset_screen_state();

    ui_ScreenPageHome = lv_obj_create(NULL);
    ui_round_screen_apply_base(ui_ScreenPageHome, lv_color_hex(0x000000));
    lv_obj_set_style_border_width(ui_ScreenPageHome, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui_ScreenPageHome, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_home_tileview = lv_tileview_create(ui_ScreenPageHome);
    lv_obj_set_size(s_home_tileview, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_home_tileview);
    lv_obj_set_style_bg_opa(s_home_tileview, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_home_tileview, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_home_tileview, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(s_home_tileview, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(s_home_tileview, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_add_event_cb(s_home_tileview, ui_home_tileview_value_changed, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_home_tileview, ui_home_tileview_gesture, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(s_home_tileview, ui_home_tileview_scroll, LV_EVENT_SCROLL, NULL);

    for (uint8_t i = 0; i < s_home_tile_count; ++i) {
        lv_dir_t dir = 0;
        if (i > 0u) {
            dir |= LV_DIR_RIGHT;
        }
        if ((uint8_t)(i + 1u) < s_home_tile_count) {
            dir |= LV_DIR_LEFT;
        }
        s_home_tiles[i] = ui_home_create_tile(s_home_tileview, i, dir);
    }

    ui_home_set_active_page(s_home_active_page, LV_ANIM_OFF);
    ui_home_runtime_refresh_active_tile();
}

static void ui_home_load(lv_scr_load_anim_t anim, uint8_t page_id)
{
    ui_home_runtime_screen_init();
    ui_home_set_active_page(page_id, LV_ANIM_OFF);
    lv_scr_load_anim(ui_ScreenPageHome, anim, UI_NAV_ANIM_MS, 0, false);
}

void ui_home_runtime_show_page(uint8_t page_id, lv_scr_load_anim_t anim)
{
    ui_home_load(anim, page_id);
}

uint8_t ui_home_runtime_page_from_default(uint8_t default_page)
{
    return ui_home_page_from_default_page(default_page);
}

void ui_home_runtime_reset(uint8_t initial_page_id)
{
    ui_ScreenPageHome = NULL;
    s_home_tileview = NULL;
    s_home_tile_count = 0;
    s_home_active_page = initial_page_id;
    s_home_edit_mode = false;
    s_home_edit_page = UI_HOME_PAGE_MENU_ID;
    s_home_edit_overlay = NULL;
    s_home_edit_target_root = NULL;
    s_home_delete_msgbox = NULL;
    s_home_notice_msgbox = NULL;
    s_home_screen_pending_delete = NULL;
    s_dashboard_config_screen = NULL;
    s_dashboard_config_gauge_index = 0;
    s_dashboard_cfg_slot_count_roller = NULL;
    memset(s_dashboard_cfg_slot_item_rollers, 0, sizeof(s_dashboard_cfg_slot_item_rollers));
    memset(s_dashboard_cfg_slot_rows, 0, sizeof(s_dashboard_cfg_slot_rows));
    ui_home_reset_screen_state();
}
