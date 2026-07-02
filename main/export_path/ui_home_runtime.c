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

#include "app_obd_dsp/aux_sensor_demand.h"
#include "app_obd_dsp/obd_data_cache.h"
#include "app_obd_dsp/default_page_ids.h"
#include "app_obd_dsp/vehicle_profiles.h"
#include "bsp_obd_dsp/elm327_ble_client.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "esp_log.h"
#include "esp_timer.h"

#define UI_NAV_ANIM_MS 0
#define UI_HOME_MAX_TILE_COUNT (1u + UI_DASHBOARD_MAX_PAGES + 1u)
#define UI_HOME_GFORCE_HISTORY_BINS 48
#define UI_HOME_GFORCE_HISTORY_POINTS (UI_HOME_GFORCE_HISTORY_BINS + 1)
#define UI_HOME_PI_F 3.14159265f
#define UI_HOME_GFORCE_MAX_G 1.20f
#define UI_HOME_NEIGHBOR_REFRESH_INTERVAL_TICKS 5u

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
    lv_obj_t *custom_title_label;
    lv_obj_t *custom_value_label;
    lv_obj_t *custom_name_labels[2];
    lv_obj_t *custom_value_labels[2];
    lv_obj_t *custom_unit_labels[2];
    lv_obj_t *gforce_plot;
    lv_obj_t *gforce_outer_ring;
    lv_obj_t *gforce_mid_ring;
    lv_obj_t *gforce_inner_ring;
    lv_obj_t *gforce_axis_h;
    lv_obj_t *gforce_axis_v;
    lv_obj_t *gforce_history_fill;
    lv_obj_t *gforce_vector_line;
    lv_obj_t *gforce_history_glow_line;
    lv_obj_t *gforce_history_line;
    lv_obj_t *gforce_dot;
    lv_obj_t *gforce_dot_glow;
    lv_obj_t *gforce_center_dot;
    lv_obj_t *gforce_quadrant_labels[4];
    lv_point_t gforce_vector_points[2];
    lv_point_t gforce_history_points[UI_HOME_GFORCE_HISTORY_POINTS];
    float gforce_history_radius[UI_HOME_GFORCE_HISTORY_BINS];
    float gforce_display_lat_g;
    float gforce_display_lon_g;
    lv_coord_t gforce_plot_size;
    lv_coord_t gforce_plot_radius;
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
static void ui_home_sync_tile_mounts(uint8_t active_page);
static void ui_home_reset_tile_effect_cache(uint8_t page_id);
static void ui_home_reset_screen_state(void);
static void ui_home_rebuild_tile_descriptors(void);
static void ui_home_reset_runtime(uint8_t tile_id);
static int8_t ui_home_page_to_gauge_index(uint8_t page_id);
static void ui_home_create_menu_content(uint8_t tile_id, lv_obj_t *parent);
static void ui_home_create_add_content(uint8_t tile_id, lv_obj_t *parent);
static void ui_home_create_gauge_content(uint8_t tile_id, lv_obj_t *parent, uint8_t gauge_index);
static void ui_home_create_gear_content(uint8_t tile_id, lv_obj_t *parent);
static void ui_home_create_gforce_content(uint8_t tile_id,
                                          lv_obj_t *parent,
                                          ui_dashboard_page_type_t page_type);
static void ui_home_create_divider(lv_obj_t *parent,
                                   lv_coord_t x,
                                   lv_coord_t y,
                                   lv_coord_t w,
                                   lv_coord_t h);
static uint8_t ui_home_collect_visible_items(const ui_dashboard_page_cfg_t *page,
                                             disp_item_t items[UI_DASHBOARD_MAX_SLOTS]);
static int16_t ui_home_slot_name_font(uint8_t slot_count);
static int16_t ui_home_slot_unit_font(uint8_t slot_count);
static int16_t ui_home_slot_value_font(lv_coord_t text_width,
                                       lv_coord_t slot_h,
                                       disp_item_t item,
                                       uint8_t slot_count);
static void ui_home_apply_slot_typography(lv_obj_t *name_label,
                                          lv_obj_t *value_label,
                                          lv_obj_t *unit_label,
                                          disp_item_t item,
                                          uint8_t slot_count);
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
static void ui_home_runtime_refresh_tiles(bool include_neighbor_tiles);
static ui_dashboard_page_type_t ui_home_page_type_for_gauge(uint8_t gauge_index);
static int32_t ui_home_estimate_long_g_x100(bool *valid_out);
static bool ui_home_read_disp_item_value(disp_item_t item, int32_t *out);
static void ui_home_refresh_menu_tile(ui_home_tile_runtime_t *rt,
                                      const char *vehicle_name,
                                      const char *ble_short);
static void ui_home_refresh_gear_tile(ui_home_tile_runtime_t *rt);
static void ui_home_refresh_gforce_tile(ui_home_tile_runtime_t *rt,
                                        ui_dashboard_page_type_t page_type);
static void ui_home_refresh_metric_tile(ui_home_tile_runtime_t *rt,
                                        const ui_dashboard_page_cfg_t *page,
                                        float sweep_ratio,
                                        int16_t brake_warn_x10,
                                        int16_t oil_warn_x10);
static float ui_home_clampf(float value, float min_value, float max_value);
static void ui_home_gforce_history_draw_event(lv_event_t *e);
static void ui_home_gforce_rebuild_history(ui_home_tile_runtime_t *rt);
static void ui_home_gforce_update_plot(ui_home_tile_runtime_t *rt, float lat_g, float lon_g);

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

static ui_dashboard_page_type_t ui_home_page_type_for_gauge(uint8_t gauge_index)
{
    const ui_dashboard_page_cfg_t *page = ui_home_get_gauge_cfg(gauge_index);

    return ui_dashboard_page_get_type(page);
}

static bool ui_home_read_disp_item_value(disp_item_t item, int32_t *out)
{
    if (out == NULL) {
        return false;
    }

    switch (item) {
    case DISP_ITEM_CLT: {
        int16_t value = obd_data_get_coolant_temp();
        if (value > -40) {
            *out = value;
            return true;
        }
        return false;
    }
    case DISP_ITEM_IAT: {
        int16_t value = obd_data_get_intake_temp();
        if (value > -40) {
            *out = value;
            return true;
        }
        return false;
    }
    case DISP_ITEM_OIL: {
        int16_t value = obd_data_get_oil_temp();
        if (value > -41) {
            *out = value;
            return true;
        }
        return false;
    }
    case DISP_ITEM_LOAD: {
        int16_t value = obd_data_get_load_pct();
        if (value >= 0) {
            *out = value;
            return true;
        }
        return false;
    }
    case DISP_ITEM_TPS: {
        int16_t value = obd_data_get_tps();
        if (value >= 0) {
            *out = value;
            return true;
        }
        return false;
    }
    case DISP_ITEM_RPM:
        *out = (int32_t)obd_data_get_rpm();
        return true;
    case DISP_ITEM_SPEED:
        *out = (int32_t)obd_data_get_speed();
        return true;
    case DISP_ITEM_BAT: {
        int32_t value = obd_data_get_bat_mv();
        if (value > 0) {
            *out = value;
            return true;
        }
        return false;
    }
    case DISP_ITEM_OILP: {
        int16_t value = obd_data_get_oil_pressure_x10();
        if (value >= 0) {
            *out = value;
            return true;
        }
        return false;
    }
    case DISP_ITEM_BKT: {
        int16_t value = obd_data_get_brake_temp_x10();
        if (value > -1000) {
            *out = value;
            return true;
        }
        return false;
    }
    case DISP_ITEM_BOOST: {
        int16_t value = obd_data_get_boost_x10();
        if (value != -32768) {
            *out = value;
            return true;
        }
        return false;
    }
    default:
        return false;
    }
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

    if (s_home_content_roots[page_id] != NULL) {
        lv_obj_clear_flag(s_home_content_roots[page_id], LV_OBJ_FLAG_HIDDEN);
    }
}

static void ui_home_reset_screen_state(void)
{
    memset(s_home_tiles, 0, sizeof(s_home_tiles));
    memset(s_home_content_roots, 0, sizeof(s_home_content_roots));
    memset(s_home_tile_mounted, 0, sizeof(s_home_tile_mounted));
    for (uint8_t i = 0; i < UI_HOME_MAX_TILE_COUNT; ++i) {
        ui_home_reset_runtime(i);
        ui_home_reset_tile_effect_cache(i);
    }
}

static int8_t ui_home_page_to_gauge_index(uint8_t page_id)
{
    if (page_id >= s_home_tile_count || s_home_tile_descs[page_id].kind != UI_HOME_TILE_GAUGE) {
        return -1;
    }

    return s_home_tile_descs[page_id].gauge_index;
}

static void ui_home_set_active_page(uint8_t page_id, lv_anim_enable_t anim_en)
{
    if (page_id >= s_home_tile_count) {
        page_id = UI_HOME_PAGE_MENU_ID;
    }

    if (s_home_tileview != NULL &&
        s_home_active_page == page_id &&
        lv_tileview_get_tile_act(s_home_tileview) == s_home_tiles[page_id]) {
        return;
    }

    s_home_active_page = page_id;
    ui_home_sync_tile_mounts(page_id);
    if (s_home_tileview != NULL) {
        lv_obj_set_tile_id(s_home_tileview, page_id, 0, anim_en);
    }
    aux_sensor_demand_refresh();
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

static uint8_t ui_home_collect_visible_items(const ui_dashboard_page_cfg_t *page,
                                             disp_item_t items[UI_DASHBOARD_MAX_SLOTS])
{
    uint8_t count = 0;
    uint8_t vehicle_profile_idx;

    if (page == NULL || items == NULL) {
        return 0;
    }

    vehicle_profile_idx = nvs_cfg_get()->vehicle_profile_idx;
    for (uint8_t i = 0; i < page->slot_count && i < UI_DASHBOARD_MAX_SLOTS; ++i) {
        uint8_t item = (uint8_t)(page->slot_items[i] % DISP_ITEM_COUNT);
        if (ui_dashboard_page_slot_is_unsupported(page, i) ||
            !ui_dashboard_item_supported_for_vehicle(vehicle_profile_idx, item)) {
            continue;
        }
        items[count++] = (disp_item_t)item;
    }

    return count;
}

static void ui_home_menu_open_settings(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ui_home_nav_vertical(LV_SCR_LOAD_ANIM_NONE, &ui_ScreenPageSettings, &ui_ScreenPageSettings_screen_init);
}

static void ui_home_menu_open_ble_scan(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ui_home_nav_vertical(LV_SCR_LOAD_ANIM_NONE, &ui_ScreenPageBLEScan, &ui_ScreenPageBLEScan_screen_init);
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
                                     LV_SCR_LOAD_ANIM_NONE);
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
    static uint8_t s_neighbor_refresh_tick = 0u;
    bool include_neighbor_tiles;

    LV_UNUSED(timer);
    s_neighbor_refresh_tick = (uint8_t)((s_neighbor_refresh_tick + 1u) % UI_HOME_NEIGHBOR_REFRESH_INTERVAL_TICKS);
    include_neighbor_tiles = (s_neighbor_refresh_tick == 0u);
    ui_home_runtime_refresh_tiles(include_neighbor_tiles);
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

static lv_coord_t ui_home_circle_left_at_y(lv_coord_t y)
{
    int32_t r = (int32_t)ui_round_radius();
    int32_t dy = (int32_t)y - r;
    int32_t r2_dy2 = r * r - dy * dy;
    if (r2_dy2 <= 0) {
        return 0;
    }
    return (lv_coord_t)(r - (int32_t)sqrtf((float)r2_dy2));
}

static lv_coord_t ui_home_circle_right_at_y(lv_coord_t y)
{
    lv_coord_t screen_w = (lv_coord_t)ui_screen_width();
    return (lv_coord_t)(screen_w - ui_home_circle_left_at_y(y));
}

static void ui_home_circle_safe_span_for_band(lv_coord_t y,
                                              lv_coord_t h,
                                              lv_coord_t inset,
                                              lv_coord_t *left_out,
                                              lv_coord_t *right_out)
{
    lv_coord_t samples[3];
    lv_coord_t left = 0;
    lv_coord_t right = (lv_coord_t)ui_screen_width();

    if (h < 1) {
        h = 1;
    }

    samples[0] = y;
    samples[1] = y + (h / 2);
    samples[2] = y + h - 1;

    for (uint8_t i = 0; i < 3; ++i) {
        lv_coord_t sample_left = ui_home_circle_left_at_y(samples[i]) + inset;
        lv_coord_t sample_right = ui_home_circle_right_at_y(samples[i]) - inset;
        if (sample_left > left) {
            left = sample_left;
        }
        if (sample_right < right) {
            right = sample_right;
        }
    }

    if (right < left) {
        lv_coord_t cx = (lv_coord_t)(ui_screen_width() / 2);
        left = cx;
        right = cx;
    }

    if (left_out != NULL) {
        *left_out = left;
    }
    if (right_out != NULL) {
        *right_out = right;
    }
}

static lv_coord_t ui_home_circular_safe_left(lv_coord_t panel_x, lv_coord_t panel_y)
{
    lv_coord_t circle_left = ui_home_circle_left_at_y(panel_y);
    lv_coord_t margin = ui_layout_px(3);
    if (circle_left + margin > panel_x) {
        return (circle_left + margin) - panel_x;
    }
    return 0;
}

static lv_coord_t ui_home_circular_safe_right(lv_coord_t panel_right, lv_coord_t panel_y)
{
    lv_coord_t circle_right = ui_home_circle_right_at_y(panel_y);
    lv_coord_t margin = ui_layout_px(3);
    if (circle_right - margin < panel_right) {
        return panel_right - (circle_right - margin);
    }
    return 0;
}

static int16_t ui_home_slot_name_font(uint8_t slot_count)
{
    if (slot_count == 1u) {
        return 32;
    }
    if (slot_count <= 2u) {
        return 22;
    }
    if (slot_count <= 4u) {
        return 18;
    }
    return 16;
}

static int16_t ui_home_slot_unit_font(uint8_t slot_count)
{
    if (slot_count == 1u) {
        return 28;
    }
    if (slot_count <= 2u) {
        return 20;
    }
    return 16;
}

static int16_t ui_home_slot_value_font(lv_coord_t text_width,
                                       lv_coord_t slot_h,
                                       disp_item_t item,
                                       uint8_t slot_count)
{
    lv_coord_t height_target;
    lv_coord_t width_target;
    int16_t min_font;

    if (text_width < ui_layout_px(48)) {
        text_width = ui_layout_px(48);
    }
    if (slot_h < ui_layout_px(48)) {
        slot_h = ui_layout_px(48);
    }

    if (slot_count == 1u) {
        height_target = slot_h * 76 / 100;
    } else if (slot_count <= 2u) {
        height_target = slot_h * 66 / 100;
    } else if (slot_count <= 4u) {
        height_target = slot_h * 58 / 100;
    } else {
        height_target = slot_h * 52 / 100;
    }

    switch (item) {
    case DISP_ITEM_RPM:
        width_target = text_width * 10 / 17;
        min_font = (slot_count == 1u) ? 100 : ((slot_count <= 2u) ? 56 : 36);
        break;
    case DISP_ITEM_SPEED:
        width_target = text_width * 10 / 12;
        min_font = (slot_count == 1u) ? 100 : ((slot_count <= 2u) ? 56 : 40);
        break;
    case DISP_ITEM_BAT:
    case DISP_ITEM_OILP:
    case DISP_ITEM_BOOST:
        width_target = text_width * 10 / 15;
        min_font = (slot_count == 1u) ? 56 : ((slot_count <= 2u) ? 44 : 32);
        break;
    default:
        width_target = text_width * 10 / 14;
        min_font = (slot_count == 1u) ? 56 : ((slot_count <= 2u) ? 44 : 32);
        break;
    }

    if (width_target < height_target) {
        height_target = width_target;
    }
    if (height_target < min_font) {
        height_target = min_font;
    }

    return (int16_t)height_target;
}

static void ui_home_apply_slot_typography(lv_obj_t *name_label,
                                          lv_obj_t *value_label,
                                          lv_obj_t *unit_label,
                                          disp_item_t item,
                                          uint8_t slot_count)
{
    lv_obj_t *panel;
    lv_coord_t panel_w;
    lv_coord_t panel_h;
    lv_coord_t text_width;
    int16_t name_font;
    int16_t unit_font;
    int16_t value_font;

    if (name_label == NULL || value_label == NULL || unit_label == NULL) {
        return;
    }

    panel = lv_obj_get_parent(value_label);
    if (panel == NULL) {
        return;
    }

    panel_w = lv_obj_get_width(panel);
    panel_h = lv_obj_get_height(panel);
    text_width = lv_obj_get_width(value_label);
    if (text_width <= 0) {
        text_width = panel_w;
    }

    name_font = ui_home_slot_name_font(slot_count);
    unit_font = ui_home_slot_unit_font(slot_count);
    value_font = ui_home_slot_value_font(text_width, panel_h, item, slot_count);

    lv_obj_set_style_text_font(name_label, ui_font_typoder(name_font), LV_PART_MAIN);
    lv_obj_set_style_text_font(unit_label, ui_font_typoder(unit_font), LV_PART_MAIN);
    lv_obj_set_style_text_font(value_label, ui_font_typoder(value_font), LV_PART_MAIN);
}

static void ui_home_create_slot_card(lv_obj_t *parent,
                                     lv_coord_t x,
                                     lv_coord_t y,
                                     lv_coord_t w,
                                     lv_coord_t h,
                                     uint8_t slot_count,
                                     lv_obj_t **name_out,
                                     lv_obj_t **value_out,
                                     lv_obj_t **unit_out)
{
    bool is_single_slot = (slot_count == 1u);
    lv_coord_t label_inset_x = w * (is_single_slot ? 4 : 5) / 100;
    lv_coord_t label_inset_y = is_single_slot ? h * 2 / 100 : h * 3 / 100;
    lv_coord_t unit_inset_x = label_inset_x;
    lv_coord_t unit_inset_y = is_single_slot ? h * 2 / 100 : h * 2 / 100;
    lv_coord_t label_sample_y;
    lv_coord_t unit_sample_y;
    lv_coord_t left_extra;
    lv_coord_t right_extra;
    lv_coord_t text_width;
    lv_coord_t name_font;
    lv_coord_t unit_font;
    lv_coord_t value_font;
    lv_coord_t value_offset_y;

    if (label_inset_x < ui_layout_px(3)) label_inset_x = ui_layout_px(3);
    if (label_inset_y < ui_layout_px(2)) label_inset_y = ui_layout_px(2);
    if (unit_inset_x < ui_layout_px(3)) unit_inset_x = ui_layout_px(3);
    if (unit_inset_y < ui_layout_px(1)) unit_inset_y = ui_layout_px(1);

    label_sample_y = y + label_inset_y;
    unit_sample_y = y + h - unit_inset_y;
    left_extra = ui_home_circular_safe_left(x, label_sample_y);
    right_extra = ui_home_circular_safe_right(x + w, unit_sample_y);
    if (left_extra > label_inset_x) {
        label_inset_x = left_extra;
    }
    if (right_extra > unit_inset_x) {
        unit_inset_x = right_extra;
    }
    text_width = w - (label_inset_x + unit_inset_x);
    if (text_width < ui_layout_px(40)) {
        text_width = ui_layout_px(40);
    }
    name_font = ui_home_slot_name_font(slot_count);
    unit_font = ui_home_slot_unit_font(slot_count);
    value_font = ui_home_slot_value_font(text_width, h, DISP_ITEM_RPM, slot_count);
    value_offset_y = is_single_slot ? h * 4 / 100 : ((slot_count == 4u) ? -(h * 2 / 100) : 0);

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
    lv_obj_align(value, LV_ALIGN_CENTER, 0, value_offset_y);

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
    lv_coord_t content_top;
    lv_coord_t content_bottom;
    lv_coord_t content_inset;
    lv_coord_t row_gap;
    lv_coord_t col_gap;
    lv_coord_t line_thickness = ui_layout_px(1);
    uint8_t row_slot_counts[3] = {0};
    uint8_t row_count = 0;

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

    memset(layouts, 0, sizeof(ui_home_slot_layout_t) * UI_DASHBOARD_MAX_SLOTS);

    content_inset = ui_safe_margin() + ui_layout_px((slot_count <= 2u) ? 8 : 10);
    content_top = content_inset;
    content_bottom = h - content_inset;
    if (content_bottom <= content_top) {
        content_top = ui_safe_margin();
        content_bottom = h - ui_safe_margin();
    }

    switch (slot_count) {
    case 1:
        row_count = 1;
        row_slot_counts[0] = 1;
        break;
    case 2:
        row_count = 2;
        row_slot_counts[0] = 1;
        row_slot_counts[1] = 1;
        break;
    case 3:
        row_count = 2;
        row_slot_counts[0] = 2;
        row_slot_counts[1] = 1;
        break;
    case 4:
        row_count = 2;
        row_slot_counts[0] = 2;
        row_slot_counts[1] = 2;
        break;
    case 5:
        row_count = 3;
        row_slot_counts[0] = 2;
        row_slot_counts[1] = 2;
        row_slot_counts[2] = 1;
        break;
    default:
        row_count = 3;
        row_slot_counts[0] = 2;
        row_slot_counts[1] = 2;
        row_slot_counts[2] = 2;
        break;
    }

    row_gap = ui_layout_px((row_count >= 3u) ? 5 : 8);
    col_gap = ui_layout_px((slot_count <= 2u) ? 0 : 8);
    if (row_gap < 4) {
        row_gap = 4;
    }
    if (col_gap < 6 && slot_count > 2u) {
        col_gap = 6;
    }

    lv_coord_t total_gap = (lv_coord_t)((row_count - 1u) * (row_gap + line_thickness));
    lv_coord_t row_h = (lv_coord_t)((content_bottom - content_top - total_gap) / row_count);
    lv_coord_t min_row_h = ui_layout_px((row_count >= 3u) ? 72 : 96);
    if (row_h < min_row_h) {
        row_h = min_row_h;
    }

    lv_coord_t used_h = (lv_coord_t)(row_count * row_h + total_gap);
    lv_coord_t start_y = (lv_coord_t)((h - used_h) / 2);
    lv_coord_t current_y = start_y;
    uint8_t slot_index = 0;

    if (slot_count == 1u) {
        lv_coord_t span_left;
        lv_coord_t span_right;
        lv_coord_t card_y = ui_safe_margin() + ui_layout_px(2);
        lv_coord_t card_h = h - (card_y * 2);

        ui_home_circle_safe_span_for_band(card_y, card_h, ui_safe_margin() + ui_layout_px(1), &span_left, &span_right);
        layouts[0] = (ui_home_slot_layout_t){span_left, card_y, (lv_coord_t)(span_right - span_left), card_h};
        return;
    }

    if (slot_count == 4u) {
        lv_coord_t center_x = w / 2;
        lv_coord_t center_y = h / 2;
        lv_coord_t center_gap = ui_layout_px(26);
        lv_coord_t top_band_y = content_top;
        lv_coord_t top_band_h;
        lv_coord_t bottom_band_y;
        lv_coord_t bottom_band_h;
        lv_coord_t card_h;
        lv_coord_t band_ys[2];
        lv_coord_t center_line_y;
        lv_coord_t divider_top;
        lv_coord_t divider_h;
        lv_coord_t hdiv_left;
        lv_coord_t hdiv_right;

        if (center_gap < ui_layout_px(20)) {
            center_gap = ui_layout_px(20);
        }
        top_band_h = (lv_coord_t)(center_y - (center_gap / 2) - top_band_y);
        bottom_band_y = (lv_coord_t)(center_y + (center_gap / 2));
        bottom_band_h = (lv_coord_t)(content_bottom - bottom_band_y);
        card_h = (top_band_h < bottom_band_h) ? top_band_h : bottom_band_h;
        if (card_h < ui_layout_px(72)) {
            card_h = ui_layout_px(72);
        }
        if (card_h > top_band_h) {
            card_h = top_band_h;
        }
        if (card_h > bottom_band_h) {
            card_h = bottom_band_h;
        }
        if (card_h < 1) {
            card_h = 1;
        }

        band_ys[0] = (lv_coord_t)(top_band_y + ((top_band_h - card_h) / 2));
        band_ys[1] = (lv_coord_t)(bottom_band_y + ((bottom_band_h - card_h) / 2));

        for (uint8_t row = 0; row < 2u; ++row) {
            lv_coord_t span_left;
            lv_coord_t span_right;
            lv_coord_t span_w;
            lv_coord_t dynamic_center_gap;
            lv_coord_t card_w;
            lv_coord_t half_left;
            lv_coord_t half_right;
            lv_coord_t max_diag_offset;
            lv_coord_t diag_offset;
            lv_coord_t left_center_x;
            lv_coord_t right_center_x;
            lv_coord_t left_x;
            lv_coord_t right_x;

            ui_home_circle_safe_span_for_band(band_ys[row], card_h, content_inset, &span_left, &span_right);
            span_w = span_right - span_left;
            dynamic_center_gap = (lv_coord_t)((card_h * 9) / 10);
            if (dynamic_center_gap < ui_layout_px(24)) {
                dynamic_center_gap = ui_layout_px(24);
            }
            if (dynamic_center_gap > span_w - (ui_layout_px(80) * 2)) {
                dynamic_center_gap = (lv_coord_t)(span_w - (ui_layout_px(80) * 2));
            }
            if (dynamic_center_gap < 0) {
                dynamic_center_gap = 0;
            }
            card_w = (lv_coord_t)((span_w - dynamic_center_gap) / 2);
            if (card_w > (lv_coord_t)(span_w * 40 / 100)) {
                card_w = (lv_coord_t)(span_w * 40 / 100);
            }
            if (card_w < 1) {
                card_w = 1;
            }

            half_left = center_x - span_left;
            half_right = span_right - center_x;
            max_diag_offset = ((half_left < half_right) ? half_left : half_right) - (card_w / 2);
            if (max_diag_offset < 0) {
                max_diag_offset = 0;
            }
            diag_offset = (lv_coord_t)(max_diag_offset * 92 / 100);
            if (diag_offset < ui_layout_px(8) && max_diag_offset > ui_layout_px(8)) {
                diag_offset = ui_layout_px(8);
            }

            left_center_x = center_x - diag_offset;
            right_center_x = center_x + diag_offset;
            left_x = left_center_x - (card_w / 2);
            right_x = right_center_x - (card_w / 2);

            if (left_x < span_left) {
                left_x = span_left;
            }
            if (right_x + card_w > span_right) {
                right_x = span_right - card_w;
            }

            layouts[slot_index++] = (ui_home_slot_layout_t){left_x, band_ys[row], card_w, card_h};
            layouts[slot_index++] = (ui_home_slot_layout_t){right_x, band_ys[row], card_w, card_h};
        }

        center_line_y = (band_ys[0] + card_h + band_ys[1]) / 2;
        ui_home_circle_safe_span_for_band(center_line_y, line_thickness, content_inset, &hdiv_left, &hdiv_right);
        ui_home_create_divider(parent,
                               hdiv_left,
                               center_line_y,
                               (lv_coord_t)(hdiv_right - hdiv_left),
                               line_thickness);

        divider_top = band_ys[0];
        divider_h = (band_ys[1] + card_h) - divider_top;
        if (divider_h > 0) {
            ui_home_create_divider(parent,
                                   center_x - (line_thickness / 2),
                                   divider_top,
                                   line_thickness,
                                   divider_h);
        }
        return;
    }

    for (uint8_t row = 0; row < row_count; ++row) {
        lv_coord_t span_left;
        lv_coord_t span_right;
        lv_coord_t span_w;

        ui_home_circle_safe_span_for_band(current_y, row_h, content_inset, &span_left, &span_right);
        span_w = span_right - span_left;

        if (row_slot_counts[row] == 1u) {
            layouts[slot_index++] = (ui_home_slot_layout_t){span_left, current_y, span_w, row_h};
        } else {
            lv_coord_t card_w = (lv_coord_t)((span_w - col_gap - line_thickness) / 2);
            lv_coord_t left_x = span_left;
            lv_coord_t divider_x = left_x + card_w + (col_gap / 2);
            lv_coord_t right_x = span_right - card_w;

            layouts[slot_index++] = (ui_home_slot_layout_t){left_x, current_y, card_w, row_h};
            layouts[slot_index++] = (ui_home_slot_layout_t){right_x, current_y, card_w, row_h};

            ui_home_create_divider(parent, divider_x, current_y, line_thickness, row_h);
        }

        if ((uint8_t)(row + 1u) < row_count) {
            lv_coord_t divider_y = current_y + row_h + (row_gap / 2);
            lv_coord_t divider_left;
            lv_coord_t divider_right;
            ui_home_circle_safe_span_for_band(divider_y, line_thickness, content_inset, &divider_left, &divider_right);
            ui_home_create_divider(parent,
                                   divider_left,
                                   divider_y,
                                   (lv_coord_t)(divider_right - divider_left),
                                   line_thickness);
        }

        current_y = (lv_coord_t)(current_y + row_h + row_gap + line_thickness);
    }
}

static void ui_home_create_gauge_content(uint8_t tile_id, lv_obj_t *parent, uint8_t gauge_index)
{
    const ui_dashboard_page_cfg_t *page = ui_home_get_gauge_cfg(gauge_index);
    ui_dashboard_page_type_t page_type = ui_home_page_type_for_gauge(gauge_index);
    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];
    ui_home_slot_layout_t layouts[UI_DASHBOARD_MAX_SLOTS];
    disp_item_t visible_items[UI_DASHBOARD_MAX_SLOTS];
    uint8_t visible_count;

    if (page == NULL) {
        return;
    }

    if (page_type == UI_DASHBOARD_PAGE_TYPE_GEAR) {
        ui_home_create_gear_content(tile_id, parent);
        return;
    }
    if (page_type == UI_DASHBOARD_PAGE_TYPE_G_FORCE_OBD ||
        page_type == UI_DASHBOARD_PAGE_TYPE_G_FORCE_ESP32) {
        ui_home_create_gforce_content(tile_id, parent, page_type);
        return;
    }

    rt->root = parent;
    visible_count = ui_home_collect_visible_items(page, visible_items);
    rt->slot_count = visible_count;
    if (visible_count == 0u) {
        lv_obj_t *placeholder = lv_label_create(parent);
        lv_label_set_text(placeholder, "NO SUPPORTED ITEM");
        lv_obj_set_style_text_font(placeholder, ui_font_typoder(20), LV_PART_MAIN);
        lv_obj_set_style_text_color(placeholder, lv_color_hex(0x7A7A7A), LV_PART_MAIN);
        lv_obj_set_style_text_align(placeholder, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_width(placeholder, ui_layout_px(220));
        lv_obj_center(placeholder);
        return;
    }

    ui_home_build_gauge_layout(parent, visible_count, layouts);

    for (uint8_t i = 0; i < visible_count; ++i) {
        ui_home_create_slot_card(parent,
                                 layouts[i].x,
                                 layouts[i].y,
                                 layouts[i].w,
                                 layouts[i].h,
                                 visible_count,
                                 &rt->name_labels[i],
                                 &rt->value_labels[i],
                                 &rt->unit_labels[i]);
        ui_home_apply_slot_typography(rt->name_labels[i],
                                      rt->value_labels[i],
                                      rt->unit_labels[i],
                                      visible_items[i],
                                      visible_count);
    }
}

static void ui_home_create_gear_content(uint8_t tile_id, lv_obj_t *parent)
{
    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];
    lv_coord_t top_pad = ui_safe_margin() + ui_layout_px(12);
    lv_coord_t side_pad = ui_safe_margin() + ui_layout_px(18);

    rt->root = parent;
    rt->slot_count = 0u;

    rt->custom_title_label = lv_label_create(parent);
    lv_label_set_text(rt->custom_title_label, "GEAR");
    lv_obj_set_style_text_font(rt->custom_title_label, ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_color(rt->custom_title_label, lv_color_hex(0x8A8A8A), LV_PART_MAIN);
    lv_obj_align(rt->custom_title_label, LV_ALIGN_TOP_LEFT, side_pad, top_pad);

    rt->custom_value_label = lv_label_create(parent);
    lv_label_set_text(rt->custom_value_label, "N");
    lv_obj_set_style_text_font(rt->custom_value_label, ui_font_typoder(140), LV_PART_MAIN);
    lv_obj_set_style_text_color(rt->custom_value_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(rt->custom_value_label);
}

static void ui_home_create_gforce_content(uint8_t tile_id,
                                          lv_obj_t *parent,
                                          ui_dashboard_page_type_t page_type)
{
    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];
    lv_coord_t top_pad = ui_safe_margin() + ui_layout_px(12);
    lv_coord_t side_pad = ui_safe_margin() + ui_layout_px(18);
    lv_coord_t bottom_pad = ui_safe_margin() + ui_layout_px(20);
    lv_coord_t title_h = ui_layout_px(24);
    lv_coord_t w;
    lv_coord_t h;
    lv_coord_t plot_size;
    lv_coord_t plot_x;
    lv_coord_t plot_y;
    lv_coord_t radius;
    lv_coord_t axis_thickness;
    lv_coord_t outer_border;
    lv_coord_t mid_size;
    lv_coord_t inner_size;
    lv_coord_t quadrant_gap;
    lv_coord_t quadrant_radius;
    lv_coord_t quadrant_center_x;
    lv_coord_t quadrant_center_y;
    lv_coord_t label_font;
    static const char *quadrants[4] = {"ACC\nLEFT", "ACC\nRIGHT", "BRAKE\nLEFT", "BRAKE\nRIGHT"};

    rt->root = parent;
    rt->slot_count = 0u;

    lv_obj_update_layout(parent);
    w = lv_obj_get_width(parent);
    h = lv_obj_get_height(parent);
    plot_size = LV_MIN(w - (side_pad * 2), h - top_pad - bottom_pad - title_h - ui_layout_px(18));
    plot_size = LV_MAX(plot_size, ui_layout_px(180));
    plot_x = (w - plot_size) / 2;
    plot_y = top_pad + title_h + ui_layout_px(10) +
             LV_MAX(0, (h - top_pad - bottom_pad - title_h - ui_layout_px(10) - plot_size) / 2);
    radius = plot_size / 2;
    axis_thickness = LV_MAX(2, plot_size / 96);
    outer_border = LV_MAX(2, plot_size / 48);
    mid_size = (plot_size * 2) / 3;
    inner_size = plot_size / 3;
    quadrant_gap = LV_MAX(ui_layout_px(10), plot_size / 18);
    quadrant_radius = (lv_coord_t)((float)radius * 0.60f);
    label_font = LV_MAX(12, plot_size / 20);
    rt->gforce_plot_size = plot_size;
    rt->gforce_plot_radius = radius - LV_MAX(ui_layout_px(14), plot_size / 14);

    rt->custom_title_label = lv_label_create(parent);
    lv_label_set_text(rt->custom_title_label,
                      (page_type == UI_DASHBOARD_PAGE_TYPE_G_FORCE_ESP32) ? "G-ESP32" : "G-OBD");
    lv_obj_set_style_text_font(rt->custom_title_label, ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_color(rt->custom_title_label,
                                (page_type == UI_DASHBOARD_PAGE_TYPE_G_FORCE_ESP32)
                                    ? lv_color_hex(0xD9B26B)
                                    : lv_color_hex(0x8A8A8A),
                                LV_PART_MAIN);
    lv_obj_align(rt->custom_title_label, LV_ALIGN_TOP_LEFT, side_pad, top_pad);

    rt->gforce_plot = lv_obj_create(parent);
    lv_obj_remove_style_all(rt->gforce_plot);
    lv_obj_set_size(rt->gforce_plot, plot_size, plot_size);
    lv_obj_set_pos(rt->gforce_plot, plot_x, plot_y);
    lv_obj_clear_flag(rt->gforce_plot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(rt->gforce_plot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(rt->gforce_plot, lv_color_hex(0x111111), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(rt->gforce_plot, lv_color_hex(0x232323), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(rt->gforce_plot, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_plot, 96, LV_PART_MAIN);

    rt->gforce_outer_ring = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_outer_ring);
    lv_obj_set_size(rt->gforce_outer_ring, plot_size, plot_size);
    lv_obj_center(rt->gforce_outer_ring);
    lv_obj_set_style_radius(rt->gforce_outer_ring, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(rt->gforce_outer_ring, outer_border, LV_PART_MAIN);
    lv_obj_set_style_border_color(rt->gforce_outer_ring, lv_color_hex(0xF1F1F1), LV_PART_MAIN);
    lv_obj_set_style_border_opa(rt->gforce_outer_ring, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_outer_ring, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_outer_ring, LV_MAX(10, plot_size / 16), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(rt->gforce_outer_ring, 44, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(rt->gforce_outer_ring, lv_color_hex(0xFFF3D2), LV_PART_MAIN);

    rt->gforce_mid_ring = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_mid_ring);
    lv_obj_set_size(rt->gforce_mid_ring, mid_size, mid_size);
    lv_obj_center(rt->gforce_mid_ring);
    lv_obj_set_style_radius(rt->gforce_mid_ring, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(rt->gforce_mid_ring, LV_MAX(1, outer_border - 1), LV_PART_MAIN);
    lv_obj_set_style_border_color(rt->gforce_mid_ring, lv_color_hex(0xA8A8A8), LV_PART_MAIN);
    lv_obj_set_style_border_opa(rt->gforce_mid_ring, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_mid_ring, LV_OPA_TRANSP, LV_PART_MAIN);

    rt->gforce_inner_ring = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_inner_ring);
    lv_obj_set_size(rt->gforce_inner_ring, inner_size, inner_size);
    lv_obj_center(rt->gforce_inner_ring);
    lv_obj_set_style_radius(rt->gforce_inner_ring, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(rt->gforce_inner_ring, LV_MAX(1, outer_border - 1), LV_PART_MAIN);
    lv_obj_set_style_border_color(rt->gforce_inner_ring, lv_color_hex(0x727272), LV_PART_MAIN);
    lv_obj_set_style_border_opa(rt->gforce_inner_ring, 44, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_inner_ring, LV_OPA_TRANSP, LV_PART_MAIN);

    rt->gforce_axis_h = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_axis_h);
    lv_obj_set_size(rt->gforce_axis_h, plot_size - (plot_size / 7), axis_thickness);
    lv_obj_center(rt->gforce_axis_h);
    lv_obj_set_style_bg_color(rt->gforce_axis_h, lv_color_hex(0xE8E8E8), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_axis_h, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_axis_h, LV_MAX(3, plot_size / 64), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(rt->gforce_axis_h, 34, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(rt->gforce_axis_h, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    rt->gforce_axis_v = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_axis_v);
    lv_obj_set_size(rt->gforce_axis_v, axis_thickness, plot_size - (plot_size / 7));
    lv_obj_center(rt->gforce_axis_v);
    lv_obj_set_style_bg_color(rt->gforce_axis_v, lv_color_hex(0xE8E8E8), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_axis_v, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_axis_v, LV_MAX(3, plot_size / 64), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(rt->gforce_axis_v, 34, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(rt->gforce_axis_v, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    rt->gforce_history_fill = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_history_fill);
    lv_obj_set_size(rt->gforce_history_fill, plot_size, plot_size);
    lv_obj_center(rt->gforce_history_fill);
    lv_obj_clear_flag(rt->gforce_history_fill, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(rt->gforce_history_fill, ui_home_gforce_history_draw_event, LV_EVENT_DRAW_MAIN, rt);

    rt->gforce_history_glow_line = lv_line_create(rt->gforce_plot);
    lv_obj_set_size(rt->gforce_history_glow_line, plot_size, plot_size);
    lv_obj_center(rt->gforce_history_glow_line);
    lv_obj_set_style_line_width(rt->gforce_history_glow_line, LV_MAX(6, plot_size / 26), LV_PART_MAIN);
    lv_obj_set_style_line_color(rt->gforce_history_glow_line, lv_color_hex(0xC8A93D), LV_PART_MAIN);
    lv_obj_set_style_line_opa(rt->gforce_history_glow_line, 92, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(rt->gforce_history_glow_line, true, LV_PART_MAIN);

    rt->gforce_history_line = lv_line_create(rt->gforce_plot);
    lv_obj_set_size(rt->gforce_history_line, plot_size, plot_size);
    lv_obj_center(rt->gforce_history_line);
    lv_obj_set_style_line_width(rt->gforce_history_line, LV_MAX(3, plot_size / 64), LV_PART_MAIN);
    lv_obj_set_style_line_color(rt->gforce_history_line, lv_color_hex(0xF0CF59), LV_PART_MAIN);
    lv_obj_set_style_line_opa(rt->gforce_history_line, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(rt->gforce_history_line, true, LV_PART_MAIN);

    rt->gforce_vector_line = lv_line_create(rt->gforce_plot);
    lv_obj_set_size(rt->gforce_vector_line, plot_size, plot_size);
    lv_obj_center(rt->gforce_vector_line);
    lv_obj_set_style_line_width(rt->gforce_vector_line, LV_MAX(1, plot_size / 96), LV_PART_MAIN);
    lv_obj_set_style_line_color(rt->gforce_vector_line, lv_color_hex(0xFF8E76), LV_PART_MAIN);
    lv_obj_set_style_line_opa(rt->gforce_vector_line, 48, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(rt->gforce_vector_line, true, LV_PART_MAIN);

    rt->gforce_dot_glow = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_dot_glow);
    lv_obj_set_size(rt->gforce_dot_glow,
                    LV_MAX(ui_layout_px(28), plot_size / 8),
                    LV_MAX(ui_layout_px(28), plot_size / 8));
    lv_obj_center(rt->gforce_dot_glow);
    lv_obj_set_style_radius(rt->gforce_dot_glow, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(rt->gforce_dot_glow, lv_color_hex(0xFF5A42), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_dot_glow, 56, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_dot_glow, LV_MAX(16, plot_size / 12), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(rt->gforce_dot_glow, 120, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(rt->gforce_dot_glow, lv_color_hex(0xFF6E4E), LV_PART_MAIN);

    rt->gforce_dot = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_dot);
    lv_obj_set_size(rt->gforce_dot,
                    LV_MAX(ui_layout_px(12), plot_size / 18),
                    LV_MAX(ui_layout_px(12), plot_size / 18));
    lv_obj_center(rt->gforce_dot);
    lv_obj_set_style_radius(rt->gforce_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(rt->gforce_dot, lv_color_hex(0xFF4D3A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(rt->gforce_dot, LV_MAX(2, plot_size / 90), LV_PART_MAIN);
    lv_obj_set_style_border_color(rt->gforce_dot, lv_color_hex(0xFFF6E8), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_dot, LV_MAX(6, plot_size / 28), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(rt->gforce_dot, 120, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(rt->gforce_dot, lv_color_hex(0xFF7A5C), LV_PART_MAIN);

    rt->gforce_center_dot = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_center_dot);
    lv_obj_set_size(rt->gforce_center_dot,
                    LV_MAX(ui_layout_px(8), plot_size / 34),
                    LV_MAX(ui_layout_px(8), plot_size / 34));
    lv_obj_center(rt->gforce_center_dot);
    lv_obj_set_style_radius(rt->gforce_center_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(rt->gforce_center_dot, lv_color_hex(0xFFF6D0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_center_dot, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_center_dot, LV_MAX(4, plot_size / 40), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(rt->gforce_center_dot, 72, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(rt->gforce_center_dot, lv_color_hex(0xFFE39A), LV_PART_MAIN);
    lv_obj_move_foreground(rt->gforce_vector_line);
    lv_obj_move_foreground(rt->gforce_dot_glow);
    lv_obj_move_foreground(rt->gforce_center_dot);
    lv_obj_move_foreground(rt->gforce_dot);

    for (uint8_t i = 0; i < 4u; ++i) {
        rt->gforce_quadrant_labels[i] = lv_label_create(parent);
        lv_label_set_text(rt->gforce_quadrant_labels[i], quadrants[i]);
        lv_label_set_long_mode(rt->gforce_quadrant_labels[i], LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_align(rt->gforce_quadrant_labels[i], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_line_space(rt->gforce_quadrant_labels[i], 0, LV_PART_MAIN);
        lv_obj_set_style_text_font(rt->gforce_quadrant_labels[i], ui_font_typoder(label_font), LV_PART_MAIN);
        lv_obj_set_style_text_color(rt->gforce_quadrant_labels[i], lv_color_hex(0xD8D2C2), LV_PART_MAIN);
        lv_obj_set_style_text_opa(rt->gforce_quadrant_labels[i], 225, LV_PART_MAIN);
    }
    lv_obj_update_layout(parent);
    quadrant_center_x = plot_x + radius;
    quadrant_center_y = plot_y + radius;
    lv_obj_set_pos(rt->gforce_quadrant_labels[0],
                   quadrant_center_x - quadrant_radius - (lv_obj_get_width(rt->gforce_quadrant_labels[0]) / 2),
                   quadrant_center_y - quadrant_radius - (lv_obj_get_height(rt->gforce_quadrant_labels[0]) / 2) +
                       quadrant_gap / 3);
    lv_obj_set_pos(rt->gforce_quadrant_labels[1],
                   quadrant_center_x + quadrant_radius - (lv_obj_get_width(rt->gforce_quadrant_labels[1]) / 2),
                   quadrant_center_y - quadrant_radius - (lv_obj_get_height(rt->gforce_quadrant_labels[1]) / 2) +
                       quadrant_gap / 3);
    lv_obj_set_pos(rt->gforce_quadrant_labels[2],
                   quadrant_center_x - quadrant_radius - (lv_obj_get_width(rt->gforce_quadrant_labels[2]) / 2),
                   quadrant_center_y + quadrant_radius - (lv_obj_get_height(rt->gforce_quadrant_labels[2]) / 2) -
                       quadrant_gap / 3);
    lv_obj_set_pos(rt->gforce_quadrant_labels[3],
                   quadrant_center_x + quadrant_radius - (lv_obj_get_width(rt->gforce_quadrant_labels[3]) / 2),
                   quadrant_center_y + quadrant_radius - (lv_obj_get_height(rt->gforce_quadrant_labels[3]) / 2) -
                       quadrant_gap / 3);

    ui_home_gforce_rebuild_history(rt);
    ui_home_gforce_update_plot(rt, 0.0f, 0.0f);
}

static int32_t ui_home_estimate_long_g_x100(bool *valid_out)
{
    static int64_t s_last_time_us = 0;
    static float s_last_speed_ms = 0.0f;
    int64_t now_us = esp_timer_get_time();
    float speed_ms = (float)obd_data_get_speed() / 3.6f;
    float dt_s;
    float accel_ms2;

    if (valid_out != NULL) {
        *valid_out = false;
    }

    if (s_last_time_us == 0) {
        s_last_time_us = now_us;
        s_last_speed_ms = speed_ms;
        return 0;
    }

    dt_s = (float)(now_us - s_last_time_us) / 1000000.0f;
    s_last_time_us = now_us;
    if (dt_s < 0.05f || dt_s > 0.5f) {
        s_last_speed_ms = speed_ms;
        return 0;
    }

    accel_ms2 = (speed_ms - s_last_speed_ms) / dt_s;
    s_last_speed_ms = speed_ms;
    if (valid_out != NULL) {
        *valid_out = true;
    }

    return (int32_t)((accel_ms2 / 9.80665f) * 100.0f);
}

static float ui_home_clampf(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static void ui_home_gforce_history_draw_event(lv_event_t *e)
{
    ui_home_tile_runtime_t *rt = (ui_home_tile_runtime_t *)lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
    lv_area_t coords;
    lv_draw_rect_dsc_t fill_dsc;
    lv_point_t points[UI_HOME_GFORCE_HISTORY_BINS];
    bool has_history = false;

    if (rt == NULL || target == NULL || draw_ctx == NULL) {
        return;
    }

    for (uint16_t i = 0; i < UI_HOME_GFORCE_HISTORY_BINS; ++i) {
        if (rt->gforce_history_radius[i] > 0.03f) {
            has_history = true;
            break;
        }
    }
    if (!has_history) {
        return;
    }

    lv_draw_rect_dsc_init(&fill_dsc);
    fill_dsc.bg_color = lv_color_hex(0xE2BE47);
    fill_dsc.bg_opa = 92;
    fill_dsc.border_width = 0;
    fill_dsc.shadow_width = LV_MAX(10, rt->gforce_plot_size / 18);
    fill_dsc.shadow_spread = LV_MAX(1, rt->gforce_plot_size / 80);
    fill_dsc.shadow_color = lv_color_hex(0xC59B22);
    fill_dsc.shadow_opa = 96;

    lv_obj_get_coords(target, &coords);
    for (uint16_t i = 0; i < UI_HOME_GFORCE_HISTORY_BINS; ++i) {
        points[i].x = coords.x1 + rt->gforce_history_points[i].x;
        points[i].y = coords.y1 + rt->gforce_history_points[i].y;
    }

    lv_draw_polygon(draw_ctx, &fill_dsc, points, UI_HOME_GFORCE_HISTORY_BINS);
}

static void ui_home_gforce_rebuild_history(ui_home_tile_runtime_t *rt)
{
    float angle_step;
    float radius_scale;
    lv_coord_t center;

    if (rt == NULL || rt->gforce_history_line == NULL || rt->gforce_plot_size <= 0) {
        return;
    }

    angle_step = (2.0f * UI_HOME_PI_F) / (float)UI_HOME_GFORCE_HISTORY_BINS;
    center = rt->gforce_plot_size / 2;
    radius_scale = (float)rt->gforce_plot_radius / UI_HOME_GFORCE_MAX_G;

    for (uint16_t i = 0; i < UI_HOME_GFORCE_HISTORY_BINS; ++i) {
        float angle = (float)i * angle_step;
        float radius = ui_home_clampf(rt->gforce_history_radius[i], 0.0f, UI_HOME_GFORCE_MAX_G) * radius_scale;
        rt->gforce_history_points[i].x = (lv_coord_t)lroundf((float)center + cosf(angle) * radius);
        rt->gforce_history_points[i].y = (lv_coord_t)lroundf((float)center - sinf(angle) * radius);
    }

    rt->gforce_history_points[UI_HOME_GFORCE_HISTORY_BINS] = rt->gforce_history_points[0];
    if (rt->gforce_history_glow_line != NULL) {
        lv_line_set_points(rt->gforce_history_glow_line,
                           rt->gforce_history_points,
                           UI_HOME_GFORCE_HISTORY_POINTS);
    }
    if (rt->gforce_history_fill != NULL) {
        lv_obj_invalidate(rt->gforce_history_fill);
    }
    lv_line_set_points(rt->gforce_history_line,
                       rt->gforce_history_points,
                       UI_HOME_GFORCE_HISTORY_POINTS);
}

static void ui_home_gforce_update_plot(ui_home_tile_runtime_t *rt, float lat_g, float lon_g)
{
    float lat_target;
    float lon_target;
    float radius;
    float angle;
    uint16_t bin_index;
    float plot_scale;
    lv_coord_t center;
    lv_coord_t dot_x;
    lv_coord_t dot_y;
    lv_coord_t dot_size;
    bool history_dirty = false;
    float history_decay;

    if (rt == NULL || rt->gforce_plot == NULL || rt->gforce_dot == NULL ||
        rt->gforce_vector_line == NULL) {
        return;
    }

    lat_target = ui_home_clampf(lat_g, -UI_HOME_GFORCE_MAX_G, UI_HOME_GFORCE_MAX_G);
    lon_target = ui_home_clampf(lon_g, -UI_HOME_GFORCE_MAX_G, UI_HOME_GFORCE_MAX_G);
    if (fabsf(lat_target) < 0.015f) {
        lat_target = 0.0f;
    }
    if (fabsf(lon_target) < 0.015f) {
        lon_target = 0.0f;
    }

    {
        float delta_lat = lat_target - rt->gforce_display_lat_g;
        float delta_lon = lon_target - rt->gforce_display_lon_g;
        float delta_mag = sqrtf((delta_lat * delta_lat) + (delta_lon * delta_lon));
        float follow = 0.14f;

        if (delta_mag > 0.40f) {
            follow = 0.34f;
        } else if (delta_mag > 0.20f) {
            follow = 0.24f;
        }

        rt->gforce_display_lat_g += delta_lat * follow;
        rt->gforce_display_lon_g += delta_lon * follow;
        rt->gforce_display_lat_g = ui_home_clampf(rt->gforce_display_lat_g,
                                                  -UI_HOME_GFORCE_MAX_G,
                                                  UI_HOME_GFORCE_MAX_G);
        rt->gforce_display_lon_g = ui_home_clampf(rt->gforce_display_lon_g,
                                                  -UI_HOME_GFORCE_MAX_G,
                                                  UI_HOME_GFORCE_MAX_G);
    }

    history_decay = ((fabsf(lat_target) < 0.05f) && (fabsf(lon_target) < 0.05f)) ? 0.008f : 0.0035f;
    for (uint16_t i = 0; i < UI_HOME_GFORCE_HISTORY_BINS; ++i) {
        float next_radius = rt->gforce_history_radius[i];

        if (next_radius <= 0.0f) {
            continue;
        }

        next_radius -= history_decay;
        if (next_radius < 0.0f) {
            next_radius = 0.0f;
        }
        if (fabsf(next_radius - rt->gforce_history_radius[i]) > 0.0005f) {
            rt->gforce_history_radius[i] = next_radius;
            history_dirty = true;
        }
    }

    radius = sqrtf((lat_target * lat_target) + (lon_target * lon_target));
    if (radius > 0.02f) {
        static const int8_t k_bin_offsets[3] = {-1, 0, 1};
        static const float k_bin_weights[3] = {0.88f, 1.0f, 0.88f};
        angle = atan2f(lon_target, lat_target);
        if (angle < 0.0f) {
            angle += 2.0f * UI_HOME_PI_F;
        }

        bin_index = (uint16_t)(angle / ((2.0f * UI_HOME_PI_F) / (float)UI_HOME_GFORCE_HISTORY_BINS));
        if (bin_index >= UI_HOME_GFORCE_HISTORY_BINS) {
            bin_index = UI_HOME_GFORCE_HISTORY_BINS - 1u;
        }
        for (uint8_t i = 0; i < 3u; ++i) {
            int32_t target_index = (int32_t)bin_index + (int32_t)k_bin_offsets[i];
            float candidate_radius;

            if (target_index < 0) {
                target_index += UI_HOME_GFORCE_HISTORY_BINS;
            } else if (target_index >= UI_HOME_GFORCE_HISTORY_BINS) {
                target_index -= UI_HOME_GFORCE_HISTORY_BINS;
            }

            candidate_radius = ui_home_clampf(radius * k_bin_weights[i], 0.0f, UI_HOME_GFORCE_MAX_G);
            if (candidate_radius > rt->gforce_history_radius[target_index]) {
                rt->gforce_history_radius[target_index] = candidate_radius;
                history_dirty = true;
            }
        }
    }

    if (history_dirty) {
        ui_home_gforce_rebuild_history(rt);
    }

    center = rt->gforce_plot_size / 2;
    plot_scale = (float)rt->gforce_plot_radius / UI_HOME_GFORCE_MAX_G;
    dot_x = (lv_coord_t)lroundf((float)center + (rt->gforce_display_lat_g * plot_scale));
    dot_y = (lv_coord_t)lroundf((float)center - (rt->gforce_display_lon_g * plot_scale));
    dot_size = lv_obj_get_width(rt->gforce_dot);

    rt->gforce_vector_points[0].x = center;
    rt->gforce_vector_points[0].y = center;
    rt->gforce_vector_points[1].x = dot_x;
    rt->gforce_vector_points[1].y = dot_y;
    lv_line_set_points(rt->gforce_vector_line, rt->gforce_vector_points, 2);

    if (rt->gforce_dot_glow != NULL) {
        lv_coord_t glow_size = lv_obj_get_width(rt->gforce_dot_glow);
        lv_obj_set_pos(rt->gforce_dot_glow, dot_x - (glow_size / 2), dot_y - (glow_size / 2));
    }
    lv_obj_set_pos(rt->gforce_dot, dot_x - (dot_size / 2), dot_y - (dot_size / 2));
}

static void ui_home_create_add_content(uint8_t tile_id, lv_obj_t *parent)
{
    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];
    lv_obj_t *plus_wrap;
    lv_obj_t *plus_h;
    lv_obj_t *plus_v;
    lv_coord_t btn_size;
    lv_coord_t plus_size;
    lv_coord_t bar_long;
    lv_coord_t bar_thick;

    lv_obj_t *btn = lv_btn_create(parent);
    btn_size = ui_layout_px(240);
    lv_obj_set_size(btn, btn_size, btn_size);
    lv_obj_center(btn);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x121212), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x2A2A2A), LV_PART_MAIN);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(btn, ui_home_add_page_click, LV_EVENT_CLICKED, NULL);

    plus_size = LV_MAX(ui_layout_px(96), (lv_coord_t)((btn_size * 52) / 100));
    bar_long = LV_MAX(ui_layout_px(74), (lv_coord_t)((btn_size * 36) / 100));
    bar_thick = LV_MAX(ui_layout_px(10), (lv_coord_t)((btn_size * 7) / 100));

    plus_wrap = lv_obj_create(btn);
    lv_obj_remove_style_all(plus_wrap);
    lv_obj_set_size(plus_wrap, plus_size, plus_size);
    lv_obj_center(plus_wrap);
    lv_obj_clear_flag(plus_wrap, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    plus_h = lv_obj_create(plus_wrap);
    lv_obj_remove_style_all(plus_h);
    lv_obj_set_size(plus_h, bar_long, bar_thick);
    lv_obj_center(plus_h);
    lv_obj_set_style_radius(plus_h, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(plus_h, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(plus_h, LV_OPA_COVER, LV_PART_MAIN);

    plus_v = lv_obj_create(plus_wrap);
    lv_obj_remove_style_all(plus_v);
    lv_obj_set_size(plus_v, bar_thick, bar_long);
    lv_obj_center(plus_v);
    lv_obj_set_style_radius(plus_v, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(plus_v, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(plus_v, LV_OPA_COVER, LV_PART_MAIN);

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
    lv_obj_set_width(s_home_delete_msgbox, ui_layout_px(320));
    lv_obj_set_style_bg_color(s_home_delete_msgbox, lv_color_hex(0x181818), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_home_delete_msgbox, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_home_delete_msgbox, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_home_delete_msgbox, ui_layout_px(18), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_home_delete_msgbox, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_home_delete_msgbox, ui_font_typoder(22), LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_home_delete_msgbox, ui_layout_px(16), LV_PART_MAIN);

    lv_obj_t *btns = lv_msgbox_get_btns(s_home_delete_msgbox);
    if (btns != NULL) {
        lv_coord_t btn_gap = ui_layout_px(10);
        lv_coord_t btn_wrap_width = (lv_coord_t)(ui_layout_px(320) - (ui_layout_px(16) * 2));
        lv_coord_t btn_width = (lv_coord_t)((btn_wrap_width - btn_gap) / 2);

        if (btn_width < ui_layout_px(110)) {
            btn_width = ui_layout_px(110);
        }

        lv_obj_set_width(btns, LV_PCT(100));
        lv_obj_set_style_pad_column(btns, btn_gap, LV_PART_MAIN);
        lv_obj_set_style_pad_left(btns, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_right(btns, 0, LV_PART_MAIN);
        lv_obj_set_style_text_font(btns, ui_font_typoder(22), LV_PART_ITEMS);
        lv_obj_set_height(btns, ui_layout_px(56));
        lv_obj_set_style_pad_ver(btns, ui_layout_px(8), LV_PART_MAIN);
        lv_obj_set_style_pad_hor(btns, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btns, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_layout(btns, LV_LAYOUT_GRID);
        static lv_coord_t grid_cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid_rows[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
        lv_obj_set_grid_dsc_array(btns, grid_cols, grid_rows);

        uint32_t child_cnt = lv_obj_get_child_cnt(btns);
        for (uint32_t i = 0; i < child_cnt; ++i) {
            lv_obj_t *btn = lv_obj_get_child(btns, i);
            lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, (int32_t)i, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
            lv_obj_set_width(btn, btn_width);
            lv_obj_set_height(btn, ui_layout_px(56));
            lv_obj_set_style_pad_left(btn, ui_layout_px(6), LV_PART_MAIN);
            lv_obj_set_style_pad_right(btn, ui_layout_px(6), LV_PART_MAIN);
            lv_obj_set_style_text_align(btn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            lv_obj_set_style_text_align(btn, LV_TEXT_ALIGN_CENTER, LV_PART_ITEMS);
        }
    }

    lv_obj_add_event_cb(s_home_delete_msgbox, ui_home_delete_confirm_result, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_home_delete_msgbox, ui_home_delete_confirm_result, LV_EVENT_DELETE, NULL);
}

static void ui_home_edit_config(lv_event_t *e)
{
    int8_t gauge_index;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED || s_home_edit_page == UI_HOME_PAGE_MENU_ID) {
        return;
    }

    gauge_index = ui_home_page_to_gauge_index(s_home_edit_page);
    if (gauge_index < 0) {
        ui_home_exit_edit_mode();
        return;
    }

    ui_home_exit_edit_mode();
    ui_dashboard_config_open((uint8_t)gauge_index);
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

    {
        lv_obj_t *zone = lv_obj_create(s_home_edit_overlay);
        lv_obj_remove_style_all(zone);
        lv_obj_set_size(zone, LV_PCT(50), LV_PCT(100));
        lv_obj_align(zone, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_add_flag(zone, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(zone, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }

    lv_obj_t *zone_edit = lv_obj_create(s_home_edit_overlay);
    lv_obj_remove_style_all(zone_edit);
    lv_obj_set_size(zone_edit, LV_PCT(50), LV_PCT(50));
    lv_obj_align(zone_edit, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(zone_edit, lv_color_hex(0x2F80ED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(zone_edit, 140, LV_PART_MAIN);
    lv_obj_clear_flag(zone_edit, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(zone_edit, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(zone_edit, ui_home_edit_config, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(zone_edit);
    lv_label_set_text(label, "EDIT");
    lv_obj_set_style_text_font(label, ui_font_typoder(24), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, ui_layout_px(16), ui_layout_px(20));

    lv_obj_t *zone_del = lv_obj_create(s_home_edit_overlay);
    lv_obj_remove_style_all(zone_del);
    lv_obj_set_size(zone_del, LV_PCT(50), LV_PCT(50));
    lv_obj_align(zone_del, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(zone_del, lv_color_hex(0xEB5757), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(zone_del, 140, LV_PART_MAIN);
    lv_obj_clear_flag(zone_del, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(zone_del, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(zone_del, ui_home_edit_delete, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(zone_del);
    lv_label_set_text(label, "DELETE");
    lv_obj_set_style_text_font(label, ui_font_typoder(22), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, ui_layout_px(-12), ui_layout_px(20));

    lv_obj_t *zone_back = lv_obj_create(s_home_edit_overlay);
    lv_obj_remove_style_all(zone_back);
    lv_obj_set_size(zone_back, LV_PCT(100), LV_PCT(50));
    lv_obj_align(zone_back, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(zone_back, lv_color_hex(0x27AE60), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(zone_back, 140, LV_PART_MAIN);
    lv_obj_clear_flag(zone_back, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(zone_back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(zone_back, ui_home_edit_back, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(zone_back);
    lv_label_set_text(label, "BACK");
    lv_obj_set_style_text_font(label, ui_font_typoder(28), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, ui_layout_px(-16));
}

static void ui_home_exit_edit_mode(void)
{
    uint8_t page_id;

    if (!s_home_edit_mode) {
        return;
    }

    page_id = s_home_edit_page;

    if (s_home_delete_msgbox != NULL) {
        lv_obj_del(s_home_delete_msgbox);
        s_home_delete_msgbox = NULL;
    }
    if (s_home_edit_overlay != NULL) {
        lv_obj_del(s_home_edit_overlay);
        s_home_edit_overlay = NULL;
    }

    if (s_home_edit_target_root != NULL) {
        lv_obj_set_style_transform_zoom(s_home_edit_target_root, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_translate_x(s_home_edit_target_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_translate_y(s_home_edit_target_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_opa(s_home_edit_target_root, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (s_home_tileview != NULL) {
        lv_obj_add_flag(s_home_tileview, LV_OBJ_FLAG_SCROLLABLE);
    }

    s_home_edit_mode = false;
    s_home_edit_page = UI_HOME_PAGE_MENU_ID;
    s_home_edit_target_root = NULL;

    ui_home_reset_tile_effect_cache(page_id);
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
        int8_t gauge_index = ui_home_page_to_gauge_index(s_home_edit_page);
        uint8_t page_count = cfg.dashboard_cfg.gauge_page_count;
        uint8_t target_page = UI_HOME_PAGE_MENU_ID;

        if (gauge_index >= 0 && (uint8_t)gauge_index < page_count) {
            for (uint8_t i = (uint8_t)gauge_index; (uint8_t)(i + 1u) < page_count; ++i) {
                cfg.dashboard_cfg.pages[i] = cfg.dashboard_cfg.pages[i + 1u];
            }
            if (page_count > 0u) {
                memset(&cfg.dashboard_cfg.pages[page_count - 1u], 0, sizeof(cfg.dashboard_cfg.pages[page_count - 1u]));
                cfg.dashboard_cfg.gauge_page_count = (uint8_t)(page_count - 1u);
            }
            nvs_cfg_set(&cfg);

            if (cfg.dashboard_cfg.gauge_page_count == 0u) {
                target_page = UI_HOME_PAGE_MENU_ID;
            } else if (gauge_index > 0) {
                target_page = (uint8_t)gauge_index;
            } else {
                target_page = 1u;
            }
        }

        ui_home_exit_edit_mode();
        ui_home_runtime_rebuild_and_load(target_page, LV_SCR_LOAD_ANIM_NONE);
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

}

static void ui_home_refresh_menu_tile(ui_home_tile_runtime_t *rt,
                                      const char *vehicle_name,
                                      const char *ble_short)
{
    ui_label_set_text_if_changed(rt->menu_vehicle_label, vehicle_name);
    ui_label_set_text_fmt_if_changed(rt->menu_ble_label, "BLE: %s", ble_short);
}

static void ui_home_refresh_gear_tile(ui_home_tile_runtime_t *rt)
{
    uint16_t rpm = obd_data_get_rpm();
    uint16_t speed = obd_data_get_speed();
    enGear gear = calculate_gear((float)rpm, (float)speed);
    const char *gear_text = "N";

    switch (gear) {
    case GEAR_1: gear_text = "1"; break;
    case GEAR_2: gear_text = "2"; break;
    case GEAR_3: gear_text = "3"; break;
    case GEAR_4: gear_text = "4"; break;
    case GEAR_5: gear_text = "5"; break;
    case GEAR_6: gear_text = "6"; break;
    case GEAR_NEUTRAL:
    default:
        gear_text = "N";
        break;
    }

    ui_label_set_text_if_changed(rt->custom_value_label, gear_text);
}

static void ui_home_refresh_gforce_tile(ui_home_tile_runtime_t *rt,
                                        ui_dashboard_page_type_t page_type)
{
    int16_t lat_x100 =
        (page_type == UI_DASHBOARD_PAGE_TYPE_G_FORCE_ESP32)
            ? obd_data_get_lat_accel_imu_x100()
            : obd_data_get_lat_accel_x100();
    int16_t lon_x100 =
        (page_type == UI_DASHBOARD_PAGE_TYPE_G_FORCE_ESP32)
            ? obd_data_get_lon_accel_imu_x100()
            : obd_data_get_lon_accel_x100();
    bool lon_valid = false;
    int32_t lon_display_x100;

    if (lon_x100 > -32768) {
        lon_valid = true;
        lon_display_x100 = lon_x100;
    } else if (page_type == UI_DASHBOARD_PAGE_TYPE_G_FORCE_OBD) {
        lon_display_x100 = ui_home_estimate_long_g_x100(&lon_valid);
    } else {
        lon_display_x100 = 0;
    }

    ui_home_gforce_update_plot(rt,
                               (lat_x100 > -32768) ? ((float)lat_x100 / 100.0f) : 0.0f,
                               lon_valid ? ((float)lon_display_x100 / 100.0f) : 0.0f);
}

static void ui_home_refresh_metric_tile(ui_home_tile_runtime_t *rt,
                                        const ui_dashboard_page_cfg_t *page,
                                        float sweep_ratio,
                                        int16_t brake_warn_x10,
                                        int16_t oil_warn_x10)
{
    disp_item_t visible_items[UI_DASHBOARD_MAX_SLOTS];
    uint8_t visible_count = ui_home_collect_visible_items(page, visible_items);

    for (uint8_t i = 0; i < rt->slot_count && i < visible_count; ++i) {
        disp_item_t item = visible_items[i];
        int32_t value = 0;
        bool valid = false;

        ui_home_apply_slot_typography(rt->name_labels[i],
                                      rt->value_labels[i],
                                      rt->unit_labels[i],
                                      item,
                                      rt->slot_count);
        disp_item_sync_meta(rt->name_labels[i], rt->unit_labels[i], &rt->item_cache[i], item);
        if (sweep_ratio >= 0.0f) {
            value = disp_item_sweep_value(item, sweep_ratio);
            valid = true;
        } else {
            valid = ui_home_read_disp_item_value(item, &value);
        }
        disp_item_set_text(rt->value_labels[i], item, value, valid);
        disp_item_set_value_color(rt->value_labels[i], item, value, valid,
                                  brake_warn_x10, oil_warn_x10);
    }
}

static void ui_home_runtime_refresh_tiles(bool include_neighbor_tiles)
{
    const nvs_user_cfg_t *user_cfg = nvs_cfg_get();
    const vehicle_profile_t *vehicle = vehicle_profile_get_active();
    const char *vehicle_name = (vehicle && vehicle->name) ? vehicle->name : "VEHICLE";
    const char *ble_name = elm327_ble_get_connected_name();
    char ble_short[16];
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
        if (!include_neighbor_tiles && tile_id != s_home_active_page) {
            continue;
        }

        switch (s_home_tile_descs[tile_id].kind) {
        case UI_HOME_TILE_MENU:
            ui_home_refresh_menu_tile(rt, vehicle_name, ble_short);
            break;
        case UI_HOME_TILE_GAUGE: {
            const ui_dashboard_page_cfg_t *page =
                ui_home_get_gauge_cfg((uint8_t)s_home_tile_descs[tile_id].gauge_index);
            ui_dashboard_page_type_t page_type =
                ui_home_page_type_for_gauge((uint8_t)s_home_tile_descs[tile_id].gauge_index);
            if (page == NULL) {
                break;
            }
            if (page_type == UI_DASHBOARD_PAGE_TYPE_GEAR) {
                ui_home_refresh_gear_tile(rt);
                break;
            }
            if (page_type == UI_DASHBOARD_PAGE_TYPE_G_FORCE_OBD ||
                page_type == UI_DASHBOARD_PAGE_TYPE_G_FORCE_ESP32) {
                ui_home_refresh_gforce_tile(rt, page_type);
                break;
            }
            ui_home_refresh_metric_tile(rt, page, sweep_ratio, brake_warn_x10, oil_warn_x10);
            break;
        }
        case UI_HOME_TILE_ADD:
        default:
            break;
        }
    }
}

void ui_home_runtime_refresh_active_tile(void)
{
    ui_home_runtime_refresh_tiles(true);
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
            break;
        }
    }
}

static void ui_home_tileview_gesture(lv_event_t *e)
{
    LV_UNUSED(e);
    if (s_home_edit_mode) {
        return;
    }

    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
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
        ui_home_nav_vertical(LV_SCR_LOAD_ANIM_NONE, &ui_ScreenPageBLEScan, &ui_ScreenPageBLEScan_screen_init);
    } else if (dir == LV_DIR_BOTTOM) {
        ui_home_nav_vertical(LV_SCR_LOAD_ANIM_NONE, &ui_ScreenPageSettings, &ui_ScreenPageSettings_screen_init);
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
    aux_sensor_demand_refresh();
}

uint8_t ui_home_runtime_page_from_default(uint8_t default_page)
{
    return ui_home_page_from_default_page(default_page);
}

bool ui_home_runtime_active_page_uses_item(disp_item_t item)
{
    if (item >= DISP_ITEM_COUNT || ui_ScreenPageHome == NULL || lv_scr_act() != ui_ScreenPageHome) {
        return false;
    }
    if (s_home_active_page >= s_home_tile_count || s_home_tile_descs[s_home_active_page].kind != UI_HOME_TILE_GAUGE) {
        return false;
    }

    const ui_dashboard_page_cfg_t *page =
        ui_home_get_gauge_cfg((uint8_t)s_home_tile_descs[s_home_active_page].gauge_index);
    disp_item_t visible_items[UI_DASHBOARD_MAX_SLOTS];
    uint8_t visible_count;
    if (page == NULL) {
        return false;
    }

    visible_count = ui_home_collect_visible_items(page, visible_items);
    for (uint8_t i = 0; i < visible_count; ++i) {
        if (visible_items[i] == item) {
            return true;
        }
    }

    return false;
}

bool ui_home_runtime_active_page_uses_type(ui_dashboard_page_type_t page_type)
{
    const ui_dashboard_page_cfg_t *page;

    if (ui_ScreenPageHome == NULL || lv_scr_act() != ui_ScreenPageHome) {
        return false;
    }
    if (s_home_active_page >= s_home_tile_count || s_home_tile_descs[s_home_active_page].kind != UI_HOME_TILE_GAUGE) {
        return false;
    }

    page = ui_home_get_gauge_cfg((uint8_t)s_home_tile_descs[s_home_active_page].gauge_index);
    if (page == NULL) {
        return false;
    }

    return ui_dashboard_page_get_type(page) == page_type;
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
