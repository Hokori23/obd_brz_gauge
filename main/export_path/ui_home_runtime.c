#include "ui_home_runtime.h"

#include "ui_dashboard_config.h"
#include "ui.h"
#include "ui_font_profile.h"
#include "ui_helpers.h"
#include "ui_layout.h"
#include "ui_round_shell.h"
#include "ui_runtime_common.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "app_obd_dsp/default_page_ids.h"
#include "app_obd_dsp/vehicle_profiles.h"
#include "bsp_obd_dsp/elm327_ble_client.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "esp_log.h"

static const char *TAG = "ui_home";

#define UI_NAV_ANIM_MS 280
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
    lv_coord_t x;
    lv_coord_t y;
    lv_coord_t w;
    lv_coord_t h;
} ui_home_slot_layout_t;

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
static lv_timer_t *s_home_refresh_timer = NULL;

static void ui_home_load(lv_scr_load_anim_t anim, uint8_t page_id);
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
static void ui_home_create_divider(lv_obj_t *parent,
                                   lv_coord_t x,
                                   lv_coord_t y,
                                   lv_coord_t w,
                                   lv_coord_t h);
static lv_coord_t ui_home_pct(lv_coord_t total, uint8_t percent);
static void ui_home_build_gauge_layout(lv_obj_t *parent,
                                       uint8_t slot_count,
                                       ui_home_slot_layout_t layouts[UI_DASHBOARD_MAX_SLOTS]);
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
static void ui_home_refresh_timer_cb(lv_timer_t *timer);

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
    uint8_t page_count = nvs_cfg_get()->dashboard_cfg.gauge_page_count;

    if (default_page == 0u || page_count == 0u) {
        return UI_HOME_PAGE_MENU_ID;
    }

    if (default_page > page_count) {
        return UI_HOME_PAGE_MENU_ID;
    }

    return default_page;
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
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
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
    ui_home_nav_vertical(LV_SCR_LOAD_ANIM_OVER_BOTTOM, &ui_ScreenPageSettings, &ui_ScreenPageSettings_screen_init);
}

static void ui_home_menu_open_ble_scan(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ui_home_nav_vertical(LV_SCR_LOAD_ANIM_OVER_TOP, &ui_ScreenPageBLEScan, &ui_ScreenPageBLEScan_screen_init);
}

static void ui_home_add_page_click(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ui_home_add_page();
}

void ui_home_runtime_rebuild_and_load(uint8_t page_id, lv_scr_load_anim_t anim)
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
    ui_home_runtime_rebuild_and_load((uint8_t)(UI_HOME_PAGE_MENU_ID + page_count + 1u),
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

static void ui_home_refresh_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    ui_home_runtime_refresh_active_tile();
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

    lv_obj_t *ble_btn = lv_btn_create(parent);
    lv_obj_set_size(ble_btn, ui_layout_px(220), ui_layout_px(48));
    lv_obj_align(ble_btn, LV_ALIGN_CENTER, 0, ui_layout_px(6));
    lv_obj_set_style_radius(ble_btn, ui_layout_px(24), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ble_btn, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
    lv_obj_set_style_border_width(ble_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(ble_btn, lv_color_hex(0x2F80ED), LV_PART_MAIN);
    lv_obj_add_flag(ble_btn, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(ble_btn, ui_home_menu_open_ble_scan, LV_EVENT_CLICKED, NULL);

    lv_obj_t *ble = lv_label_create(ble_btn);
    lv_label_set_text(ble, "BLE: Not set");
    lv_obj_set_style_text_font(ble, ui_font_typoder(18), LV_PART_MAIN);
    lv_obj_set_style_text_color(ble, lv_color_hex(0xAFCFFF), LV_PART_MAIN);
    lv_obj_center(ble);

    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, ui_layout_px(180), ui_layout_px(48));
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, ui_layout_px(86));
    lv_obj_set_style_radius(btn, ui_layout_px(18), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x202020), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_GESTURE_BUBBLE);
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

static lv_coord_t ui_home_pct(lv_coord_t total, uint8_t percent)
{
    return (lv_coord_t)((total * percent) / 100);
}

static lv_coord_t ui_home_circular_safe_left(lv_coord_t panel_x, lv_coord_t panel_y)
{
    int32_t r = (int32_t)ui_round_radius();
    int32_t dy = (int32_t)panel_y - r;
    int32_t r2_dy2 = r * r - dy * dy;
    if (r2_dy2 <= 0) {
        return 0;
    }
    lv_coord_t circle_left = (lv_coord_t)(r - (int32_t)sqrtf((float)r2_dy2));
    lv_coord_t margin = ui_layout_px(3);
    if (circle_left + margin > panel_x) {
        return (circle_left + margin) - panel_x;
    }
    return 0;
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
    lv_coord_t label_inset_x = w * 4 / 100;
    lv_coord_t label_inset_y = h * 3 / 100;
    lv_coord_t unit_inset_x = label_inset_x;
    lv_coord_t unit_inset_y = h * 2 / 100;
    if (label_inset_x < ui_layout_px(3)) label_inset_x = ui_layout_px(3);
    if (label_inset_y < ui_layout_px(2)) label_inset_y = ui_layout_px(2);
    if (unit_inset_x < ui_layout_px(3)) unit_inset_x = ui_layout_px(3);
    if (unit_inset_y < ui_layout_px(1)) unit_inset_y = ui_layout_px(1);
    lv_coord_t circ_extra = ui_home_circular_safe_left(x, y + label_inset_y);
    if (circ_extra > label_inset_x) {
        label_inset_x = circ_extra;
    }
    lv_coord_t text_width = w - (label_inset_x + unit_inset_x);
    lv_coord_t value_target_px = h * 32 / 100;
    lv_coord_t max_for_width = text_width * 10 / 35;
    if (value_target_px > max_for_width) {
        value_target_px = max_for_width;
    }
    uint16_t scr_min = ui_screen_width() < ui_screen_height()
                       ? ui_screen_width() : ui_screen_height();
    lv_coord_t value_font = (lv_coord_t)((int32_t)value_target_px *
                            UI_PLATFORM_BASE_RES / (int32_t)scr_min);
    if (value_font < 18) value_font = 18;
    lv_coord_t name_font = value_font * 40 / 100;
    if (name_font < 11) name_font = 11;
    lv_coord_t unit_font = name_font;

    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, w, h);
    lv_obj_align(panel, LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);

    lv_obj_t *name = lv_label_create(panel);
    lv_label_set_text(name, "RPM");
    lv_obj_set_width(name, text_width);
    lv_label_set_long_mode(name, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(name, ui_font_typoder(name_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(name, lv_color_hex(0x9A9A9A), LV_PART_MAIN);
    lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_align(name, LV_ALIGN_TOP_LEFT, label_inset_x, label_inset_y);

    lv_obj_t *value = lv_label_create(panel);
    lv_label_set_text(value, "--");
    lv_obj_set_width(value, text_width);
    lv_label_set_long_mode(value, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(value, ui_font_typoder(value_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(value, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(value, LV_ALIGN_CENTER, 0, ui_layout_px(2));

    lv_obj_t *unit = lv_label_create(panel);
    lv_label_set_text(unit, "");
    lv_obj_set_width(unit, text_width);
    lv_label_set_long_mode(unit, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(unit, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_font(unit, ui_font_typoder(unit_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(unit, lv_color_hex(0x707070), LV_PART_MAIN);
    lv_obj_align(unit, LV_ALIGN_BOTTOM_RIGHT, -unit_inset_x, -unit_inset_y);

    *name_out = name;
    *value_out = value;
    *unit_out = unit;
}

static void ui_home_create_divider(lv_obj_t *parent,
                                   lv_coord_t x,
                                   lv_coord_t y,
                                   lv_coord_t w,
                                   lv_coord_t h)
{
    lv_obj_t *divider = lv_obj_create(parent);
    lv_obj_remove_style_all(divider);
    lv_obj_set_size(divider, w, h);
    lv_obj_align(divider, LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0x2E2E2E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, LV_PART_MAIN);
}

static void ui_home_build_gauge_layout(lv_obj_t *parent,
                                       uint8_t slot_count,
                                       ui_home_slot_layout_t layouts[UI_DASHBOARD_MAX_SLOTS])
{
    lv_coord_t w;
    lv_coord_t h;
    lv_coord_t cx;
    lv_coord_t safe_left;
    lv_coord_t safe_right;
    lv_coord_t safe_width;
    lv_coord_t line_thickness = ui_layout_px(1);
    lv_coord_t center_line_y;
    lv_coord_t narrow_col_w;
    lv_coord_t full_col_w;
    lv_coord_t left_col_x;
    lv_coord_t right_col_x;
    lv_coord_t top_y;
    lv_coord_t top_h;
    lv_coord_t bottom_y;
    lv_coord_t bottom_h;
    lv_coord_t mid_row_y;
    lv_coord_t row_h;
    lv_coord_t row_gap;

    lv_obj_update_layout(parent);
    w = lv_obj_get_content_width(parent);
    h = lv_obj_get_content_height(parent);
    if (w <= 0) {
        w = lv_obj_get_width(parent);
    }
    if (h <= 0) {
        h = lv_obj_get_height(parent);
    }
    if (line_thickness < 1) {
        line_thickness = 1;
    }

    cx = w / 2;
    safe_left = ui_home_pct(w, 12);
    safe_right = w - safe_left;
    safe_width = safe_right - safe_left;
    center_line_y = ui_home_pct(h, 49);
    narrow_col_w = ui_home_pct(w, 29);
    full_col_w = safe_width;
    left_col_x = safe_left;
    right_col_x = safe_right - narrow_col_w;

    memset(layouts, 0, sizeof(ui_home_slot_layout_t) * UI_DASHBOARD_MAX_SLOTS);

    switch (slot_count) {
    case 1:
        layouts[0] = (ui_home_slot_layout_t){
            safe_left,
            ui_home_pct(h, 21),
            full_col_w,
            ui_home_pct(h, 38),
        };
        break;
    case 2:
        layouts[0] = (ui_home_slot_layout_t){
            safe_left,
            ui_home_pct(h, 13),
            full_col_w,
            ui_home_pct(h, 34),
        };
        layouts[1] = (ui_home_slot_layout_t){
            safe_left,
            center_line_y + ui_home_pct(h, 2),
            full_col_w,
            ui_home_pct(h, 23),
        };
        ui_home_create_divider(parent, safe_left, center_line_y, full_col_w, line_thickness);
        break;
    case 3:
        top_y = ui_home_pct(h, 12);
        top_h = center_line_y - top_y;
        bottom_y = center_line_y + line_thickness + ui_home_pct(h, 1);
        bottom_h = ui_home_pct(h, 23);

        layouts[0] = (ui_home_slot_layout_t){left_col_x, top_y, narrow_col_w, top_h};
        layouts[1] = (ui_home_slot_layout_t){right_col_x, top_y, narrow_col_w, top_h};
        layouts[2] = (ui_home_slot_layout_t){safe_left, bottom_y, full_col_w, bottom_h};

        ui_home_create_divider(parent, cx, top_y, line_thickness, (center_line_y - top_y) + line_thickness);
        ui_home_create_divider(parent, safe_left, center_line_y, full_col_w, line_thickness);
        break;
    case 4:
        top_y = ui_home_pct(h, 12);
        top_h = center_line_y - top_y;
        bottom_y = center_line_y + line_thickness + ui_home_pct(h, 1);
        bottom_h = ui_home_pct(h, 23);

        layouts[0] = (ui_home_slot_layout_t){left_col_x, top_y, narrow_col_w, top_h};
        layouts[1] = (ui_home_slot_layout_t){right_col_x, top_y, narrow_col_w, top_h};
        layouts[2] = (ui_home_slot_layout_t){left_col_x, bottom_y, narrow_col_w, bottom_h};
        layouts[3] = (ui_home_slot_layout_t){right_col_x, bottom_y, narrow_col_w, bottom_h};

        ui_home_create_divider(parent, cx, top_y, line_thickness, (bottom_y + bottom_h) - top_y);
        ui_home_create_divider(parent, safe_left, center_line_y, full_col_w, line_thickness);
        break;
    case 5:
        top_y = ui_home_pct(h, 7);
        row_h = ui_home_pct(h, 20);
        row_gap = ui_home_pct(h, 3);
        mid_row_y = top_y + row_h + row_gap + line_thickness;
        bottom_y = top_y + (row_h * 2) + (row_gap * 2) + line_thickness;
        bottom_h = ui_home_pct(h, 18);

        layouts[0] = (ui_home_slot_layout_t){left_col_x, top_y, narrow_col_w, row_h};
        layouts[1] = (ui_home_slot_layout_t){right_col_x, top_y, narrow_col_w, row_h};
        layouts[2] = (ui_home_slot_layout_t){left_col_x, mid_row_y, narrow_col_w, row_h};
        layouts[3] = (ui_home_slot_layout_t){right_col_x, mid_row_y, narrow_col_w, row_h};
        layouts[4] = (ui_home_slot_layout_t){safe_left, bottom_y, full_col_w, bottom_h};

        ui_home_create_divider(parent, cx, top_y, line_thickness, (mid_row_y + row_h) - top_y);
        ui_home_create_divider(parent, safe_left, top_y + row_h + row_gap, full_col_w, line_thickness);
        ui_home_create_divider(parent, safe_left, bottom_y - ui_home_pct(h, 2), full_col_w, line_thickness);
        break;
    default:
        top_y = ui_home_pct(h, 7);
        row_h = ui_home_pct(h, 18);
        row_gap = ui_home_pct(h, 4);
        mid_row_y = top_y + row_h + row_gap + line_thickness;
        bottom_y = mid_row_y + row_h + row_gap + line_thickness;

        layouts[0] = (ui_home_slot_layout_t){left_col_x, top_y, narrow_col_w, row_h};
        layouts[1] = (ui_home_slot_layout_t){right_col_x, top_y, narrow_col_w, row_h};
        layouts[2] = (ui_home_slot_layout_t){left_col_x, mid_row_y, narrow_col_w, row_h};
        layouts[3] = (ui_home_slot_layout_t){right_col_x, mid_row_y, narrow_col_w, row_h};
        layouts[4] = (ui_home_slot_layout_t){left_col_x, bottom_y, narrow_col_w, row_h};
        layouts[5] = (ui_home_slot_layout_t){right_col_x, bottom_y, narrow_col_w, row_h};

        ui_home_create_divider(parent, cx, top_y, line_thickness, (bottom_y + row_h) - top_y);
        ui_home_create_divider(parent, safe_left, top_y + row_h + row_gap, full_col_w, line_thickness);
        ui_home_create_divider(parent, safe_left, mid_row_y + row_h + row_gap, full_col_w, line_thickness);
        break;
    }
}

static void ui_home_create_gauge_content(uint8_t tile_id, lv_obj_t *parent, uint8_t gauge_index)
{
    const ui_dashboard_page_cfg_t *page = ui_home_get_gauge_cfg(gauge_index);
    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];
    ui_home_slot_layout_t layouts[UI_DASHBOARD_MAX_SLOTS];

    if (page == NULL) {
        return;
    }

    rt->root = parent;
    rt->slot_count = page->slot_count;
    ui_home_build_gauge_layout(parent, page->slot_count, layouts);

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
    lv_obj_add_flag(btn, LV_OBJ_FLAG_GESTURE_BUBBLE);
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
        ui_home_runtime_rebuild_and_load(target_page, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
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
        ui_home_nav_vertical(LV_SCR_LOAD_ANIM_OVER_TOP, &ui_ScreenPageBLEScan, &ui_ScreenPageBLEScan_screen_init);
    } else if (dir == LV_DIR_BOTTOM) {
        ui_home_nav_vertical(LV_SCR_LOAD_ANIM_OVER_BOTTOM, &ui_ScreenPageSettings, &ui_ScreenPageSettings_screen_init);
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
            dir |= LV_DIR_LEFT;
        }
        if ((uint8_t)(i + 1u) < s_home_tile_count) {
            dir |= LV_DIR_RIGHT;
        }
        s_home_tiles[i] = ui_home_create_tile(s_home_tileview, i, dir);
    }

    ui_home_set_active_page(s_home_active_page, LV_ANIM_OFF);
    ui_home_runtime_refresh_active_tile();
    if (s_home_refresh_timer == NULL) {
        s_home_refresh_timer = lv_timer_create(ui_home_refresh_timer_cb, 200, NULL);
    }
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
    if (s_home_refresh_timer != NULL) {
        lv_timer_del(s_home_refresh_timer);
        s_home_refresh_timer = NULL;
    }
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
    ui_dashboard_config_reset();
    ui_home_reset_screen_state();
}
