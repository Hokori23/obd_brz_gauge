// Settings Page
// Unified horizontal settings pages with round-screen-safe layout

#include "../ui.h"
#include "../ui_font_profile.h"
#include "../ui_home_runtime.h"
#include "../ui_layout.h"
#include "../ui_round_shell.h"

#include <stdio.h>
#include <string.h>

#include "app_obd_dsp/aux_sensor_demand.h"
#include "app_obd_dsp/vehicle_profiles.h"
#include "bsp_obd_dsp/boards/board_api.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "esp_system.h"
#if CONFIG_OBD_BOARD_WS_175_AMOLED
#include "bsp_obd_dsp/boards/board_ws_175_amoled_spec.h"
#endif

typedef enum {
    UI_SETTINGS_PAGE_DISPLAY = 0,
    UI_SETTINGS_PAGE_DASHBOARD,
    UI_SETTINGS_PAGE_VEHICLE,
    UI_SETTINGS_PAGE_OBD,
    UI_SETTINGS_PAGE_COUNT
} ui_settings_page_t;

static lv_obj_t *s_settings_tileview = NULL;
static lv_obj_t *s_settings_tiles[UI_SETTINGS_PAGE_COUNT] = {0};
static lv_obj_t *s_settings_page_dots[UI_SETTINGS_PAGE_COUNT] = {0};
static lv_obj_t *s_settings_header_title = NULL;
static lv_obj_t *s_settings_reboot_msgbox = NULL;
static lv_obj_t *s_roller_page = NULL;
static lv_obj_t *s_roller_vehicle = NULL;
static lv_obj_t *s_roller_protocol = NULL;
static lv_obj_t *s_roller_obd_poll = NULL;
static lv_obj_t *s_roller_racechrono = NULL;
static lv_obj_t *s_roller_oil_pressure = NULL;
#if CONFIG_OBD_BOARD_WS_175_AMOLED
static lv_obj_t *s_roller_rotation = NULL;
#endif
static lv_obj_t *s_slider_bright = NULL;
static lv_obj_t *s_label_bright_val = NULL;
static ui_settings_page_t s_settings_active_page = UI_SETTINGS_PAGE_DISPLAY;
static bool s_settings_requires_home_rebuild = false;
static bool s_settings_suppress_reboot_roller_event = false;
static uint8_t s_settings_pending_reboot_value = 0u;
typedef enum {
    UI_SETTINGS_REBOOT_NONE = 0,
    UI_SETTINGS_REBOOT_PROTOCOL,
    UI_SETTINGS_REBOOT_ROTATION,
} ui_settings_reboot_target_t;
static ui_settings_reboot_target_t s_settings_pending_reboot_target = UI_SETTINGS_REBOOT_NONE;

static const char *s_settings_page_titles[UI_SETTINGS_PAGE_COUNT] = {
    "DISPLAY",
    "DASHBOARD",
    "VEHICLE",
    "OBD",
};

static void ui_settings_reset_control_refs(void)
{
    s_roller_page = NULL;
    s_roller_vehicle = NULL;
    s_roller_protocol = NULL;
    s_roller_obd_poll = NULL;
    s_roller_racechrono = NULL;
    s_roller_oil_pressure = NULL;
#if CONFIG_OBD_BOARD_WS_175_AMOLED
    s_roller_rotation = NULL;
#endif
    s_slider_bright = NULL;
    s_label_bright_val = NULL;
}

static void ui_settings_screen_reset_state(void)
{
    ui_ScreenPageSettings = NULL;
    s_settings_tileview = NULL;
    memset(s_settings_tiles, 0, sizeof(s_settings_tiles));
    memset(s_settings_page_dots, 0, sizeof(s_settings_page_dots));
    s_settings_header_title = NULL;
    s_settings_reboot_msgbox = NULL;
    ui_settings_reset_control_refs();
    s_settings_active_page = UI_SETTINGS_PAGE_DISPLAY;
    s_settings_requires_home_rebuild = false;
    s_settings_suppress_reboot_roller_event = false;
    s_settings_pending_reboot_target = UI_SETTINGS_REBOOT_NONE;
    s_settings_pending_reboot_value = 0u;
}

static void ui_settings_screen_deleted(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_settings_screen_reset_state();
}

static void ui_settings_apply_roller_theme(lv_obj_t *roller, uint16_t radius)
{
    if (roller == NULL) {
        return;
    }

    ui_round_shell_apply_dark_roller_preset(roller, radius, ui_layout_px(40), 15);
}

#if CONFIG_OBD_BOARD_WS_175_AMOLED
static const char *ui_settings_rotation_options(void)
{
    return "NORMAL\nROTATE 180";
}

static uint8_t ui_settings_rotation_mode_from_roller_index(uint16_t selected_index)
{
    return (selected_index == 0u) ? NVS_DISPLAY_ROTATION_MODE_NORMAL
                                  : NVS_DISPLAY_ROTATION_MODE_180;
}

static uint16_t ui_settings_rotation_roller_index_from_mode(uint8_t mode)
{
    const nvs_user_cfg_t *cfg = nvs_cfg_get();
    const uint16_t effective_degrees = nvs_cfg_get_display_rotation_degrees(
        cfg, BOARD_WS_175_AMOLED_DISPLAY_ROTATION);

    if (mode == NVS_DISPLAY_ROTATION_MODE_NORMAL) {
        return 0u;
    }
    if (mode == NVS_DISPLAY_ROTATION_MODE_180) {
        return 1u;
    }

    return (effective_degrees == 180u) ? 1u : 0u;
}
#endif

static void ui_settings_set_header(ui_settings_page_t page)
{
    if (page >= UI_SETTINGS_PAGE_COUNT) {
        return;
    }

    if (s_settings_header_title != NULL) {
        lv_label_set_text(s_settings_header_title, s_settings_page_titles[page]);
    }
}

static void ui_settings_update_page_dots(void)
{
    for (uint8_t i = 0; i < UI_SETTINGS_PAGE_COUNT; ++i) {
        if (s_settings_page_dots[i] == NULL) {
            continue;
        }

        lv_obj_set_style_bg_color(s_settings_page_dots[i],
                                  (i == s_settings_active_page) ? lv_color_hex(0x2F80ED)
                                                                : lv_color_hex(0x2A2A2A),
                                  LV_PART_MAIN);
        lv_obj_set_style_border_color(s_settings_page_dots[i],
                                      (i == s_settings_active_page) ? lv_color_hex(0x2F80ED)
                                                                    : lv_color_hex(0x3A3A3A),
                                      LV_PART_MAIN);
        lv_obj_set_width(s_settings_page_dots[i], (i == s_settings_active_page) ? ui_layout_px(18) : ui_layout_px(8));
    }
}

static void ui_settings_close_to_home(void)
{
    if (s_settings_requires_home_rebuild) {
        ui_home_runtime_rebuild_and_load(UI_HOME_PAGE_MENU_ID, LV_SCR_LOAD_ANIM_NONE);
    } else {
        ui_home_runtime_show_page(UI_HOME_PAGE_MENU_ID, LV_SCR_LOAD_ANIM_NONE);
    }
}

static void ui_settings_reboot_confirm_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *msgbox = lv_event_get_current_target(e);
    uint16_t active_btn;

    if (code == LV_EVENT_DELETE) {
        s_settings_reboot_msgbox = NULL;
        return;
    }

    if (code != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    if (msgbox == NULL || msgbox != s_settings_reboot_msgbox) {
        return;
    }

    active_btn = lv_msgbox_get_active_btn(msgbox);
    if (active_btn == 1u) {
        nvs_user_cfg_t cfg = *nvs_cfg_get();
        if (s_settings_pending_reboot_target == UI_SETTINGS_REBOOT_PROTOCOL) {
            cfg.protocol = s_settings_pending_reboot_value;
        } else if (s_settings_pending_reboot_target == UI_SETTINGS_REBOOT_ROTATION) {
            cfg.rsv[1] = s_settings_pending_reboot_value;
        }
        nvs_cfg_set(&cfg);
        esp_restart();
        return;
    }

    s_settings_suppress_reboot_roller_event = true;
    if (s_settings_pending_reboot_target == UI_SETTINGS_REBOOT_PROTOCOL && s_roller_protocol != NULL) {
        lv_roller_set_selected(s_roller_protocol, nvs_cfg_get()->protocol, LV_ANIM_OFF);
    }
#if CONFIG_OBD_BOARD_WS_175_AMOLED
    if (s_settings_pending_reboot_target == UI_SETTINGS_REBOOT_ROTATION && s_roller_rotation != NULL) {
        lv_roller_set_selected(s_roller_rotation,
                               ui_settings_rotation_roller_index_from_mode(
                                   nvs_cfg_get_display_rotation_mode(nvs_cfg_get())),
                               LV_ANIM_OFF);
    }
#endif
    s_settings_suppress_reboot_roller_event = false;
    s_settings_pending_reboot_target = UI_SETTINGS_REBOOT_NONE;
    s_settings_pending_reboot_value = 0u;
    lv_obj_del(msgbox);
}

static void ui_settings_show_reboot_confirm(ui_settings_reboot_target_t target,
                                            uint8_t pending_value,
                                            const char *title,
                                            const char *message)
{
    static const char *btn_texts[] = {"CANCEL", "APPLY", ""};

    if (ui_ScreenPageSettings == NULL) {
        return;
    }
    if (s_settings_reboot_msgbox != NULL) {
        lv_obj_del(s_settings_reboot_msgbox);
        s_settings_reboot_msgbox = NULL;
    }

    s_settings_pending_reboot_target = target;
    s_settings_pending_reboot_value = pending_value;
    s_settings_reboot_msgbox = lv_msgbox_create(ui_ScreenPageSettings, title, message, btn_texts, false);
    lv_obj_center(s_settings_reboot_msgbox);
    lv_obj_set_width(s_settings_reboot_msgbox, ui_layout_px(320));
    ui_round_shell_apply_modal_theme(s_settings_reboot_msgbox, lv_color_hex(0x2F80ED), 1);
    lv_obj_add_event_cb(s_settings_reboot_msgbox, ui_settings_reboot_confirm_event, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_settings_reboot_msgbox, ui_settings_reboot_confirm_event, LV_EVENT_DELETE, NULL);
}

static void on_page_roller_change(lv_event_t *e)
{
    LV_UNUSED(e);
    nvs_user_cfg_t cfg = *nvs_cfg_get();
    cfg.default_page = (uint8_t)lv_roller_get_selected(s_roller_page);
    nvs_cfg_set(&cfg);
}

static void on_bright_slider_change(lv_event_t *e)
{
    int32_t val = lv_slider_get_value(s_slider_bright);

    LV_UNUSED(e);
    if (val < 10) {
        val = 10;
    }

    lv_label_set_text_fmt(s_label_bright_val, "%ld%%", val);

    nvs_user_cfg_t cfg = *nvs_cfg_get();
    cfg.brightness_day = (uint8_t)val;
    nvs_cfg_set(&cfg);
    board_set_brightness((uint8_t)val);
}

#if CONFIG_OBD_BOARD_WS_175_AMOLED
static void on_rotation_roller_change(lv_event_t *e)
{
    LV_UNUSED(e);
}

static void on_rotation_roller_released(lv_event_t *e)
{
    LV_UNUSED(e);

    if (s_settings_suppress_reboot_roller_event || s_roller_rotation == NULL) {
        return;
    }

    uint8_t selected_mode = ui_settings_rotation_mode_from_roller_index(
        (uint16_t)lv_roller_get_selected(s_roller_rotation));
    uint8_t current_mode = nvs_cfg_get_display_rotation_mode(nvs_cfg_get());

    if (ui_settings_rotation_roller_index_from_mode(selected_mode) ==
        ui_settings_rotation_roller_index_from_mode(current_mode)) {
        return;
    }

    ui_settings_show_reboot_confirm(UI_SETTINGS_REBOOT_ROTATION,
                                    selected_mode,
                                    "Apply Rotation",
                                    "Display rotation requires reboot.");
}
#endif

static void on_vehicle_roller_change(lv_event_t *e)
{
    uint8_t selected = (uint8_t)lv_roller_get_selected(s_roller_vehicle);
    nvs_user_cfg_t cfg = *nvs_cfg_get();

    LV_UNUSED(e);
    cfg.vehicle_profile_idx = selected;
    nvs_cfg_set(&cfg);
    vehicle_profile_set_active(selected);
    s_settings_requires_home_rebuild = true;
}

static void on_protocol_roller_released(lv_event_t *e)
{
    LV_UNUSED(e);
    if (s_settings_suppress_reboot_roller_event || s_roller_protocol == NULL) {
        return;
    }

    uint8_t selected = (uint8_t)lv_roller_get_selected(s_roller_protocol);
    if (selected == nvs_cfg_get()->protocol) {
        return;
    }

    ui_settings_show_reboot_confirm(UI_SETTINGS_REBOOT_PROTOCOL,
                                    selected,
                                    "Apply Protocol",
                                    "Protocol change requires reboot.");
}

static void ui_settings_gesture_event(lv_event_t *e)
{
    lv_dir_t dir;

    if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
        return;
    }

    dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir != LV_DIR_TOP) {
        return;
    }

    lv_indev_wait_release(lv_indev_get_act());
    ui_settings_close_to_home();
}

static void ui_settings_tileview_value_changed(lv_event_t *e)
{
    if (lv_event_get_target(e) != s_settings_tileview) {
        return;
    }

    lv_obj_t *active_tile = lv_tileview_get_tile_act(s_settings_tileview);
    for (uint8_t i = 0; i < UI_SETTINGS_PAGE_COUNT; ++i) {
        if (s_settings_tiles[i] == active_tile) {
            s_settings_active_page = (ui_settings_page_t)i;
            ui_settings_set_header(s_settings_active_page);
            ui_settings_update_page_dots();
            break;
        }
    }
}

static void ui_settings_page_dot_click(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || s_settings_tileview == NULL) {
        return;
    }

    ui_settings_page_t page = (ui_settings_page_t)(uintptr_t)lv_event_get_user_data(e);
    if (page >= UI_SETTINGS_PAGE_COUNT) {
        return;
    }

    lv_obj_set_tile_id(s_settings_tileview, (uint8_t)page, 0, LV_ANIM_ON);
}

static void ui_settings_style_panel(lv_obj_t *panel, lv_coord_t radius)
{
    lv_obj_set_style_radius(panel, radius, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x161616), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x2E2E2E), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, ui_layout_px(10), LV_PART_MAIN);
}

static lv_obj_t *ui_settings_create_page_root(lv_obj_t *tile)
{
    lv_obj_t *root = lv_obj_create(tile);

    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_center(root);
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(root, ui_layout_px(6), LV_PART_MAIN);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    return root;
}

static lv_obj_t *ui_settings_create_content_panel(lv_obj_t *parent,
                                                  lv_coord_t width,
                                                  lv_coord_t min_height,
                                                  lv_coord_t radius)
{
    lv_obj_t *panel = lv_obj_create(parent);

    lv_obj_set_width(panel, width);
    lv_obj_set_height(panel, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(panel, min_height, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    ui_settings_style_panel(panel, radius);
    lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(panel, ui_layout_px(6), LV_PART_MAIN);
    return panel;
}

static void ui_settings_add_panel_title(lv_obj_t *panel, const char *title, const char *subtitle)
{
    lv_obj_t *title_label = lv_label_create(panel);
    lv_label_set_text(title_label, title);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(title_label, ui_font_typoder(16), LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(title_label, LV_PCT(100));

    if (subtitle != NULL && subtitle[0] != '\0') {
        lv_obj_t *subtitle_label = lv_label_create(panel);
        lv_label_set_text(subtitle_label, subtitle);
        lv_label_set_long_mode(subtitle_label, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(subtitle_label, ui_font_hint(10), LV_PART_MAIN);
        lv_obj_set_style_text_color(subtitle_label, lv_color_hex(0x8A8A8A), LV_PART_MAIN);
        lv_obj_set_style_text_align(subtitle_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_width(subtitle_label, LV_PCT(100));
    }
}

static lv_obj_t *ui_settings_create_inline_row(lv_obj_t *parent,
                                               const char *label_text,
                                               lv_coord_t label_width,
                                               lv_coord_t row_height)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_t *label = lv_label_create(row);

    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, row_height);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(row, ui_layout_px(8), LV_PART_MAIN);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_label_set_text(label, label_text);
    lv_obj_set_style_text_font(label, ui_font_hint(11), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xB8B8B8), LV_PART_MAIN);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(label, label_width);

    return row;
}

static lv_obj_t *ui_settings_create_inline_roller(lv_obj_t *row,
                                                  lv_coord_t width,
                                                  const char *options,
                                                  uint16_t selected,
                                                  lv_event_cb_t cb,
                                                  lv_event_code_t event_code)
{
    lv_obj_t *roller = lv_roller_create(row);

    lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(roller, 1);
    lv_roller_set_selected(roller, selected, LV_ANIM_OFF);
    lv_obj_set_size(roller, width, ui_layout_px(40));
    ui_settings_apply_roller_theme(roller, (uint16_t)ui_layout_px(12));
    if (cb != NULL) {
        lv_obj_add_event_cb(roller, cb, event_code, NULL);
    }

    return roller;
}

static void on_obd_poll_roller_change(lv_event_t *e)
{
    LV_UNUSED(e);
    nvs_user_cfg_t cfg = *nvs_cfg_get();
    cfg.rsv[0] = (uint8_t)lv_roller_get_selected(s_roller_obd_poll);
    nvs_cfg_set(&cfg);
}

static void on_racechrono_roller_change(lv_event_t *e)
{
    LV_UNUSED(e);
    nvs_user_cfg_t cfg = *nvs_cfg_get();
    cfg.theme_cfg.rsv[0] = (uint8_t)lv_roller_get_selected(s_roller_racechrono);
    nvs_cfg_set(&cfg);
}

static void on_oil_pressure_roller_change(lv_event_t *e)
{
    LV_UNUSED(e);
    nvs_user_cfg_t cfg = *nvs_cfg_get();
    cfg.theme_cfg.rsv[1] = (uint8_t)lv_roller_get_selected(s_roller_oil_pressure);
    nvs_cfg_set(&cfg);
    aux_sensor_demand_refresh();
}

static void ui_settings_build_display_page(lv_obj_t *tile, lv_coord_t content_w)
{
    const nvs_user_cfg_t *cfg = nvs_cfg_get();
    lv_coord_t panel_radius = ui_layout_px(18);
    lv_obj_t *root = ui_settings_create_page_root(tile);
    lv_obj_t *panel = ui_settings_create_content_panel(root, content_w, ui_layout_px(86), panel_radius);

    ui_settings_add_panel_title(panel, "BRIGHTNESS", NULL);

    s_label_bright_val = lv_label_create(panel);
    lv_label_set_text_fmt(s_label_bright_val, "%d%%", cfg->brightness_day);
    lv_obj_set_style_text_font(s_label_bright_val, ui_font_hint(11), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_bright_val, lv_color_hex(0xA7A7A7), LV_PART_MAIN);
    lv_obj_set_style_text_align(s_label_bright_val, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(s_label_bright_val, LV_PCT(100));

    s_slider_bright = lv_slider_create(panel);
    lv_slider_set_range(s_slider_bright, 10, 100);
    lv_slider_set_value(s_slider_bright, cfg->brightness_day, LV_ANIM_OFF);
    lv_obj_set_width(s_slider_bright, content_w - ui_layout_px(40));
    lv_obj_set_height(s_slider_bright, ui_layout_px(10));
    lv_obj_set_style_bg_color(s_slider_bright, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_slider_bright, 255, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_slider_bright, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_slider_bright, 255, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_slider_bright, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_slider_bright, ui_layout_px(5), LV_PART_KNOB);
    lv_obj_clear_flag(s_slider_bright, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(s_slider_bright, on_bright_slider_change, LV_EVENT_VALUE_CHANGED, NULL);

#if CONFIG_OBD_BOARD_WS_175_AMOLED
    panel = ui_settings_create_content_panel(root, content_w, ui_layout_px(58), panel_radius);
    lv_obj_t *row = ui_settings_create_inline_row(panel, "ROTATION", ui_layout_px(92), ui_layout_px(40));
    s_roller_rotation = ui_settings_create_inline_roller(row,
                                                         content_w - ui_layout_px(124),
                                                         ui_settings_rotation_options(),
                                                         ui_settings_rotation_roller_index_from_mode(
                                                             nvs_cfg_get_display_rotation_mode(cfg)),
                                                         on_rotation_roller_change,
                                                         LV_EVENT_VALUE_CHANGED);
    lv_obj_add_event_cb(s_roller_rotation, on_rotation_roller_released, LV_EVENT_RELEASED, NULL);
#endif
}

static void ui_settings_build_dashboard_page(lv_obj_t *tile, lv_coord_t content_w)
{
    const nvs_user_cfg_t *cfg = nvs_cfg_get();
    lv_coord_t panel_radius = ui_layout_px(18);
    lv_coord_t label_w = ui_layout_px(98);
    lv_obj_t *root = ui_settings_create_page_root(tile);
    char page_names[64] = {0};
    uint8_t page_option_count = (uint8_t)(cfg->dashboard_cfg.gauge_page_count + 1u);
    uint8_t selected_default_page = cfg->default_page;
    lv_obj_t *panel = ui_settings_create_content_panel(root, content_w, ui_layout_px(200), panel_radius);
    lv_obj_t *row;
    lv_coord_t roller_w = content_w - label_w - ui_layout_px(28);

    if (page_option_count > DEFAULT_PAGE_COUNT) {
        page_option_count = DEFAULT_PAGE_COUNT;
    }
    if (page_option_count == 0u) {
        page_option_count = 1u;
    }

    strlcpy(page_names, "MENU", sizeof(page_names));
    for (uint8_t i = 1; i < page_option_count; ++i) {
        char option[16];
        snprintf(option, sizeof(option), "\nG%u", (unsigned)i);
        strlcat(page_names, option, sizeof(page_names));
    }

    if (selected_default_page >= page_option_count) {
        selected_default_page = 0u;
    }

    row = ui_settings_create_inline_row(panel, "BOOT PAGE", label_w, ui_layout_px(40));
    s_roller_page = ui_settings_create_inline_roller(row, roller_w, page_names, selected_default_page, on_page_roller_change, LV_EVENT_VALUE_CHANGED);

    row = ui_settings_create_inline_row(panel, "OBD POLL", label_w, ui_layout_px(40));
    s_roller_obd_poll = ui_settings_create_inline_roller(row,
                                                         roller_w,
                                                         "NORMAL\nFAST",
                                                         nvs_cfg_get_obd_poll_mode(cfg),
                                                         on_obd_poll_roller_change,
                                                         LV_EVENT_VALUE_CHANGED);

    row = ui_settings_create_inline_row(panel, "RACECHRONO", label_w, ui_layout_px(40));
    s_roller_racechrono = ui_settings_create_inline_roller(row,
                                                           roller_w,
                                                           "ON\nOFF",
                                                           nvs_cfg_get_racechrono_ble_mode(cfg),
                                                           on_racechrono_roller_change,
                                                           LV_EVENT_VALUE_CHANGED);

    row = ui_settings_create_inline_row(panel, "OIL PRESS", label_w, ui_layout_px(40));
    s_roller_oil_pressure = ui_settings_create_inline_roller(row,
                                                             roller_w,
                                                             "ALWAYS\nDEMAND",
                                                             nvs_cfg_get_oil_pressure_mode(cfg),
                                                             on_oil_pressure_roller_change,
                                                             LV_EVENT_VALUE_CHANGED);
}

static void ui_settings_build_vehicle_page(lv_obj_t *tile, lv_coord_t content_w)
{
    const nvs_user_cfg_t *cfg = nvs_cfg_get();
    const vehicle_profile_t *vehicle_list;
    char vehicle_names[128] = {0};
    uint8_t vehicle_count = 0;
    lv_coord_t panel_radius = ui_layout_px(18);
    lv_obj_t *root = ui_settings_create_page_root(tile);
    lv_obj_t *panel;
    lv_obj_t *row;

    vehicle_list = vehicle_profile_get_all(&vehicle_count);
    for (uint8_t i = 0; i < vehicle_count; ++i) {
        if (i > 0u) {
            strlcat(vehicle_names, "\n", sizeof(vehicle_names));
        }
        strlcat(vehicle_names, vehicle_list[i].name, sizeof(vehicle_names));
    }

    panel = ui_settings_create_content_panel(root, content_w, ui_layout_px(58), panel_radius);
    row = ui_settings_create_inline_row(panel, "PROFILE", ui_layout_px(86), ui_layout_px(40));
    s_roller_vehicle = ui_settings_create_inline_roller(row,
                                                        content_w - ui_layout_px(118),
                                                        vehicle_names,
                                                        (cfg->vehicle_profile_idx < vehicle_count) ? cfg->vehicle_profile_idx : 0u,
                                                        on_vehicle_roller_change,
                                                        LV_EVENT_VALUE_CHANGED);
}

static void ui_settings_build_obd_page(lv_obj_t *tile, lv_coord_t content_w)
{
    const nvs_user_cfg_t *cfg = nvs_cfg_get();
    lv_coord_t panel_radius = ui_layout_px(18);
    lv_obj_t *root = ui_settings_create_page_root(tile);
    lv_obj_t *panel = ui_settings_create_content_panel(root, content_w, ui_layout_px(58), panel_radius);
    lv_obj_t *row = ui_settings_create_inline_row(panel, "PROTOCOL", ui_layout_px(86), ui_layout_px(40));

    s_roller_protocol = ui_settings_create_inline_roller(row,
                                                         content_w - ui_layout_px(118),
                                                         "AUTO\nJ1850 PWM\nJ1850 VPW\nISO9141-2\nKWP 5B\nKWP FAST\nCAN 11/500\nCAN 29/500\nCAN 11/250\nCAN 29/250",
                                                         cfg->protocol,
                                                         NULL,
                                                         LV_EVENT_VALUE_CHANGED);
    lv_obj_add_event_cb(s_roller_protocol, on_protocol_roller_released, LV_EVENT_RELEASED, NULL);
}

static void ui_settings_build_pages(lv_coord_t content_w)
{
    ui_settings_reset_control_refs();

    for (uint8_t i = 0; i < UI_SETTINGS_PAGE_COUNT; ++i) {
        if (s_settings_tiles[i] == NULL) {
            continue;
        }

        lv_obj_clean(s_settings_tiles[i]);
        switch ((ui_settings_page_t)i) {
        case UI_SETTINGS_PAGE_DISPLAY:
            ui_settings_build_display_page(s_settings_tiles[i], content_w);
            break;
        case UI_SETTINGS_PAGE_DASHBOARD:
            ui_settings_build_dashboard_page(s_settings_tiles[i], content_w);
            break;
        case UI_SETTINGS_PAGE_VEHICLE:
            ui_settings_build_vehicle_page(s_settings_tiles[i], content_w);
            break;
        case UI_SETTINGS_PAGE_OBD:
            ui_settings_build_obd_page(s_settings_tiles[i], content_w);
            break;
        default:
            break;
        }
    }
}

static void ui_settings_prepare_tileview(lv_coord_t *content_width_out)
{
    lv_coord_t safe_margin = ui_safe_margin();
    lv_coord_t title_top = (lv_coord_t)(safe_margin + ui_layout_px(12));
    lv_coord_t title_h = ui_layout_px(26);
    lv_coord_t dots_h = ui_layout_px(8);
    lv_coord_t dots_bottom_inset = (lv_coord_t)(safe_margin + ui_layout_px(14));
    lv_coord_t content_top = (lv_coord_t)(title_top + title_h + ui_layout_px(12));
    lv_coord_t content_bottom = (lv_coord_t)(ui_screen_height() - dots_bottom_inset - dots_h - ui_layout_px(10));
    lv_coord_t content_h = (lv_coord_t)(content_bottom - content_top);
    lv_coord_t panel_sample_h;
    lv_coord_t panel_sample_y;
    lv_coord_t content_left;
    lv_coord_t content_right;
    lv_coord_t content_w;

    if (content_h < 1) {
        content_h = 1;
    }

    panel_sample_h = LV_MIN(content_h, ui_layout_px(120));
    if (panel_sample_h < ui_layout_px(58)) {
        panel_sample_h = ui_layout_px(58);
    }
    panel_sample_y = content_top + (content_h - panel_sample_h) / 2;

    ui_round_shell_safe_span_for_band(panel_sample_y,
                                      panel_sample_h,
                                      (lv_coord_t)(safe_margin + ui_layout_px(2)),
                                      &content_left,
                                      &content_right);
    content_w = (lv_coord_t)(content_right - content_left);
    if (content_w > ui_layout_px(272)) {
        content_w = ui_layout_px(272);
    }
    if (content_w < ui_layout_px(180)) {
        content_w = ui_layout_px(180);
    }
    content_left = (lv_coord_t)((ui_screen_width() - content_w) / 2);

    s_settings_tileview = lv_tileview_create(ui_ScreenPageSettings);
    lv_obj_set_size(s_settings_tileview, content_w, content_h);
    lv_obj_align(s_settings_tileview, LV_ALIGN_TOP_LEFT, content_left, content_top);
    lv_obj_set_style_bg_opa(s_settings_tileview, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_settings_tileview, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_settings_tileview, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(s_settings_tileview, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(s_settings_tileview, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_add_event_cb(s_settings_tileview, ui_settings_tileview_value_changed, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_settings_tileview, ui_settings_gesture_event, LV_EVENT_GESTURE, NULL);

    for (uint8_t i = 0; i < UI_SETTINGS_PAGE_COUNT; ++i) {
        lv_dir_t dir = 0;
        if (i > 0u) {
            dir |= LV_DIR_LEFT;
        }
        if ((uint8_t)(i + 1u) < UI_SETTINGS_PAGE_COUNT) {
            dir |= LV_DIR_RIGHT;
        }
        s_settings_tiles[i] = lv_tileview_add_tile(s_settings_tileview, i, 0, dir);
        lv_obj_clear_flag(s_settings_tiles[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(s_settings_tiles[i], 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(s_settings_tiles[i], 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(s_settings_tiles[i], 0, LV_PART_MAIN);
    }

    if (content_width_out != NULL) {
        *content_width_out = content_w;
    }
}

void ui_ScreenPageSettings_screen_init(void)
{
    ui_settings_layout_t layout;
    lv_obj_t *title;
    lv_obj_t *dot_row;
    lv_coord_t content_w = 0;

    ui_settings_layout_get(&layout);

    ui_ScreenPageSettings = lv_obj_create(NULL);
    ui_round_screen_apply_base(ui_ScreenPageSettings, lv_color_hex(0x000000));
    ui_round_shell_create_ring(ui_ScreenPageSettings, &layout.shell);
    lv_obj_add_event_cb(ui_ScreenPageSettings,
                        scr_unloaded_delete_cb,
                        LV_EVENT_SCREEN_UNLOADED,
                        &ui_ScreenPageSettings);
    lv_obj_add_event_cb(ui_ScreenPageSettings, ui_settings_screen_deleted, LV_EVENT_DELETE, NULL);
    lv_obj_add_event_cb(ui_ScreenPageSettings, ui_settings_gesture_event, LV_EVENT_GESTURE, NULL);

    ui_round_shell_create_title_block(ui_ScreenPageSettings,
                                      s_settings_page_titles[s_settings_active_page],
                                      NULL,
                                      ui_safe_margin() + ui_layout_px(12),
                                      22,
                                      ui_layout_px(4),
                                      11,
                                      &title,
                                      NULL);
    s_settings_header_title = title;

    dot_row = lv_obj_create(ui_ScreenPageSettings);
    lv_obj_remove_style_all(dot_row);
    lv_obj_set_size(dot_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(dot_row, LV_ALIGN_BOTTOM_MID, 0, -(ui_safe_margin() + ui_layout_px(14)));
    lv_obj_set_layout(dot_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(dot_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dot_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dot_row, ui_layout_px(8), LV_PART_MAIN);

    for (uint8_t i = 0; i < UI_SETTINGS_PAGE_COUNT; ++i) {
        s_settings_page_dots[i] = lv_btn_create(dot_row);
        lv_obj_set_size(s_settings_page_dots[i], ui_layout_px(8), ui_layout_px(8));
        lv_obj_set_style_radius(s_settings_page_dots[i], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(s_settings_page_dots[i], 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(s_settings_page_dots[i], 1, LV_PART_MAIN);
        lv_obj_set_style_pad_all(s_settings_page_dots[i], 0, LV_PART_MAIN);
        lv_obj_add_event_cb(s_settings_page_dots[i], ui_settings_page_dot_click, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }

    ui_settings_prepare_tileview(&content_w);
    ui_settings_build_pages(content_w);
    ui_settings_set_header(s_settings_active_page);
    ui_settings_update_page_dots();
    aux_sensor_demand_refresh();
}
