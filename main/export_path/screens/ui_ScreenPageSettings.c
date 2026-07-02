// Settings Page
// Two-level settings navigation with round-screen-safe category cards and detail panels

#include "../ui.h"
#include "../ui_font_profile.h"
#include "../ui_home_runtime.h"
#include "../ui_layout.h"
#include "../ui_round_shell.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "app_obd_dsp/aux_sensor_demand.h"
#include "app_obd_dsp/vehicle_profiles.h"
#include "bsp_obd_dsp/boards/board_api.h"
#include "bsp_obd_dsp/nvs_storage.h"

typedef enum {
    UI_SETTINGS_SECTION_ROOT = 0,
    UI_SETTINGS_SECTION_DISPLAY,
    UI_SETTINGS_SECTION_DASHBOARD,
    UI_SETTINGS_SECTION_VEHICLE
} ui_settings_section_t;

static lv_obj_t *s_roller_page = NULL;
static lv_obj_t *s_roller_vehicle = NULL;
static lv_obj_t *s_roller_obd_poll = NULL;
#if CONFIG_OBD_BOARD_WS_175_AMOLED
static lv_obj_t *s_roller_rotation = NULL;
#endif
static lv_obj_t *s_slider_bright = NULL;
static lv_obj_t *s_label_bright_val = NULL;
static lv_obj_t *s_settings_header_title = NULL;
static lv_obj_t *s_settings_header_subtitle = NULL;
static lv_obj_t *s_settings_back_btn = NULL;
static lv_obj_t *s_settings_content = NULL;
static bool s_settings_requires_home_rebuild = false;
static ui_settings_section_t s_settings_section = UI_SETTINGS_SECTION_ROOT;

static lv_coord_t ui_settings_circle_left_at_y(lv_coord_t y)
{
    int32_t r = (int32_t)ui_round_radius();
    int32_t dy = (int32_t)y - r;
    int32_t r2_dy2 = r * r - dy * dy;

    if (r2_dy2 <= 0) {
        return 0;
    }

    return (lv_coord_t)(r - (int32_t)sqrtf((float)r2_dy2));
}

static lv_coord_t ui_settings_circle_right_at_y(lv_coord_t y)
{
    return (lv_coord_t)(ui_screen_width() - ui_settings_circle_left_at_y(y));
}

static void ui_settings_safe_span_for_band(lv_coord_t y,
                                           lv_coord_t h,
                                           lv_coord_t inset,
                                           lv_coord_t *left_out,
                                           lv_coord_t *right_out)
{
    lv_coord_t samples[3] = {y, y + (h / 2), y + h - 1};
    lv_coord_t left = 0;
    lv_coord_t right = (lv_coord_t)ui_screen_width();

    if (h < 1) {
        h = 1;
        samples[1] = y;
        samples[2] = y;
    }

    for (uint8_t i = 0; i < 3; ++i) {
        lv_coord_t sample_left = ui_settings_circle_left_at_y(samples[i]) + inset;
        lv_coord_t sample_right = ui_settings_circle_right_at_y(samples[i]) - inset;
        if (sample_left > left) {
            left = sample_left;
        }
        if (sample_right < right) {
            right = sample_right;
        }
    }

    if (right < left) {
        lv_coord_t center = (lv_coord_t)(ui_screen_width() / 2);
        left = center;
        right = center;
    }

    if (left_out != NULL) {
        *left_out = left;
    }
    if (right_out != NULL) {
        *right_out = right;
    }
}

static void ui_settings_screen_reset_state(void)
{
    ui_ScreenPageSettings = NULL;
    s_roller_page = NULL;
    s_roller_vehicle = NULL;
    s_roller_obd_poll = NULL;
#if CONFIG_OBD_BOARD_WS_175_AMOLED
    s_roller_rotation = NULL;
#endif
    s_slider_bright = NULL;
    s_label_bright_val = NULL;
    s_settings_header_title = NULL;
    s_settings_header_subtitle = NULL;
    s_settings_back_btn = NULL;
    s_settings_content = NULL;
    s_settings_requires_home_rebuild = false;
    s_settings_section = UI_SETTINGS_SECTION_ROOT;
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

    lv_obj_clear_flag(roller, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_roller_set_visible_row_count(roller, 1);
    lv_obj_set_style_text_font(roller, ui_font_typoder(18), LV_PART_MAIN);
    lv_obj_set_style_text_font(roller, ui_font_typoder(18), LV_PART_SELECTED);
    ui_round_shell_apply_roller_theme(roller,
                                      radius,
                                      lv_color_hex(0x222222),
                                      lv_color_hex(0x444444),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0xCFCFCF),
                                      lv_color_hex(0x000000));
}

static void ui_settings_style_panel(lv_obj_t *panel, lv_coord_t radius)
{
    lv_obj_set_style_radius(panel, radius, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x161616), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x2E2E2E), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, ui_layout_px(14), LV_PART_MAIN);
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
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(panel, ui_layout_px(8), LV_PART_MAIN);
    return panel;
}

static lv_obj_t *ui_settings_create_category_card(lv_obj_t *parent,
                                                  const char *title,
                                                  const char *subtitle,
                                                  ui_settings_section_t target_section,
                                                  lv_coord_t width,
                                                  lv_coord_t min_height,
                                                  lv_coord_t radius);
static void ui_settings_show_section(ui_settings_section_t section);

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

static void on_obd_poll_roller_change(lv_event_t *e)
{
    LV_UNUSED(e);
    nvs_user_cfg_t cfg = *nvs_cfg_get();
    cfg.rsv[0] = (uint8_t)lv_roller_get_selected(s_roller_obd_poll);
    nvs_cfg_set(&cfg);
}

#if CONFIG_OBD_BOARD_WS_175_AMOLED
static void on_rotation_roller_change(lv_event_t *e)
{
    LV_UNUSED(e);
    nvs_user_cfg_t cfg = *nvs_cfg_get();
    cfg.rsv[1] = (uint8_t)lv_roller_get_selected(s_roller_rotation);
    nvs_cfg_set(&cfg);
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

static void ui_settings_close_to_home(void)
{
    if (s_settings_requires_home_rebuild) {
        ui_home_runtime_rebuild_and_load(UI_HOME_PAGE_MENU_ID, LV_SCR_LOAD_ANIM_NONE);
    } else {
        ui_home_runtime_show_page(UI_HOME_PAGE_MENU_ID, LV_SCR_LOAD_ANIM_MOVE_LEFT);
    }
}

static void ui_settings_back_click(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    if (s_settings_section == UI_SETTINGS_SECTION_ROOT) {
        ui_settings_close_to_home();
        return;
    }

    ui_settings_show_section(UI_SETTINGS_SECTION_ROOT);
}

static void on_settings_background(lv_event_t *e)
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
    if (s_settings_section == UI_SETTINGS_SECTION_ROOT) {
        ui_settings_close_to_home();
    } else {
        ui_settings_show_section(UI_SETTINGS_SECTION_ROOT);
    }
}

static void ui_settings_open_section(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    ui_settings_show_section((ui_settings_section_t)(uintptr_t)lv_event_get_user_data(e));
}

static void ui_settings_prepare_content_root(lv_coord_t *content_width_out)
{
    lv_coord_t safe_margin = ui_safe_margin();
    lv_coord_t content_top = (lv_coord_t)(safe_margin + ui_layout_px(46));
    lv_coord_t content_bottom_inset = (lv_coord_t)(safe_margin + ui_layout_px(18));
    lv_coord_t content_h = (lv_coord_t)(ui_screen_height() - content_top - content_bottom_inset);
    lv_coord_t content_left;
    lv_coord_t content_right;
    lv_coord_t content_w;

    if (content_h < 1) {
        content_h = 1;
    }

    ui_settings_safe_span_for_band(content_top,
                                   content_h,
                                   (lv_coord_t)(safe_margin + ui_layout_px(8)),
                                   &content_left,
                                   &content_right);
    content_w = (lv_coord_t)(content_right - content_left);
    if (content_w < ui_layout_px(160)) {
        content_w = ui_layout_px(160);
        content_left = (lv_coord_t)((ui_screen_width() - content_w) / 2);
    }

    s_settings_content = lv_obj_create(ui_ScreenPageSettings);
    lv_obj_remove_style_all(s_settings_content);
    lv_obj_set_size(s_settings_content, content_w, content_h);
    lv_obj_align(s_settings_content, LV_ALIGN_TOP_LEFT, content_left, content_top);
    lv_obj_clear_flag(s_settings_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(s_settings_content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_settings_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_settings_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_settings_content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(s_settings_content, ui_layout_px(10), LV_PART_MAIN);

    if (content_width_out != NULL) {
        *content_width_out = content_w;
    }
}

static void ui_settings_set_header(const char *title, const char *subtitle, bool show_back)
{
    if (s_settings_header_title != NULL) {
        lv_label_set_text(s_settings_header_title, title);
    }
    if (s_settings_header_subtitle != NULL) {
        lv_label_set_text(s_settings_header_subtitle, subtitle);
    }
    if (s_settings_back_btn != NULL) {
        if (show_back) {
            lv_obj_clear_flag(s_settings_back_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_settings_back_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void ui_settings_build_root(lv_coord_t content_w)
{
    lv_coord_t card_radius = ui_layout_px(18);
    lv_coord_t card_height = ui_layout_px(78);

    ui_settings_set_header("SETTINGS", "Choose a category", false);
    ui_settings_create_category_card(s_settings_content,
                                     "DISPLAY",
                                     "Brightness and screen rotation",
                                     UI_SETTINGS_SECTION_DISPLAY,
                                     content_w,
                                     card_height,
                                     card_radius);
    ui_settings_create_category_card(s_settings_content,
                                     "DASHBOARD",
                                     "Boot page and OBD poll mode",
                                     UI_SETTINGS_SECTION_DASHBOARD,
                                     content_w,
                                     card_height,
                                     card_radius);
    ui_settings_create_category_card(s_settings_content,
                                     "VEHICLE",
                                     "Profile and metric compatibility",
                                     UI_SETTINGS_SECTION_VEHICLE,
                                     content_w,
                                     card_height,
                                     card_radius);
}

static void ui_settings_add_panel_title(lv_obj_t *panel, const char *title, const char *subtitle)
{
    lv_obj_t *title_label = lv_label_create(panel);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, ui_font_typoder(18), LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(title_label, LV_PCT(100));

    if (subtitle != NULL && subtitle[0] != '\0') {
        lv_obj_t *subtitle_label = lv_label_create(panel);
        lv_label_set_text(subtitle_label, subtitle);
        lv_obj_set_style_text_font(subtitle_label, ui_font_hint(11), LV_PART_MAIN);
        lv_obj_set_style_text_color(subtitle_label, lv_color_hex(0x8A8A8A), LV_PART_MAIN);
        lv_obj_set_style_text_align(subtitle_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_width(subtitle_label, LV_PCT(100));
    }
}

static void ui_settings_build_display(lv_coord_t content_w)
{
    const nvs_user_cfg_t *cfg = nvs_cfg_get();
    lv_coord_t panel_radius = ui_layout_px(18);
    lv_coord_t panel_h = ui_layout_px(108);
    lv_obj_t *panel = ui_settings_create_content_panel(s_settings_content, content_w, panel_h, panel_radius);

    ui_settings_set_header("DISPLAY", "Screen behavior", true);
    ui_settings_add_panel_title(panel, "BRIGHTNESS", "Live apply");

    s_slider_bright = lv_slider_create(panel);
    lv_slider_set_range(s_slider_bright, 10, 100);
    lv_slider_set_value(s_slider_bright, cfg->brightness_day, LV_ANIM_OFF);
    lv_obj_set_width(s_slider_bright, content_w - ui_layout_px(56));
    lv_obj_set_height(s_slider_bright, ui_layout_px(10));
    lv_obj_set_style_bg_color(s_slider_bright, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_slider_bright, 255, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_slider_bright, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_slider_bright, 255, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_slider_bright, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_slider_bright, ui_layout_px(5), LV_PART_KNOB);
    lv_obj_clear_flag(s_slider_bright, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(s_slider_bright, on_bright_slider_change, LV_EVENT_VALUE_CHANGED, NULL);

    s_label_bright_val = lv_label_create(panel);
    lv_label_set_text_fmt(s_label_bright_val, "%d%%", cfg->brightness_day);
    lv_obj_set_style_text_font(s_label_bright_val, ui_font_typoder(22), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_bright_val, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

#if CONFIG_OBD_BOARD_WS_175_AMOLED
    panel = ui_settings_create_content_panel(s_settings_content, content_w, panel_h, panel_radius);
    ui_settings_add_panel_title(panel, "ROTATION", "Applies after reboot");

    s_roller_rotation = lv_roller_create(panel);
    lv_roller_set_options(s_roller_rotation, "DEFAULT\nNORMAL\nROTATE 180", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(s_roller_rotation, nvs_cfg_get_display_rotation_mode(cfg), LV_ANIM_OFF);
    lv_obj_set_width(s_roller_rotation, content_w - ui_layout_px(72));
    ui_settings_apply_roller_theme(s_roller_rotation, (uint16_t)ui_layout_px(12));
    lv_obj_add_event_cb(s_roller_rotation, on_rotation_roller_change, LV_EVENT_VALUE_CHANGED, NULL);
#endif
}

static void ui_settings_build_dashboard(lv_coord_t content_w)
{
    const nvs_user_cfg_t *cfg = nvs_cfg_get();
    const ui_dashboard_cfg_t *dashboard_cfg = &cfg->dashboard_cfg;
    char page_names[64] = {0};
    uint8_t page_option_count = (uint8_t)(dashboard_cfg->gauge_page_count + 1u);
    uint8_t selected_default_page = cfg->default_page;
    lv_coord_t panel_radius = ui_layout_px(18);
    lv_coord_t panel_h = ui_layout_px(108);
    lv_obj_t *panel;

    if (page_option_count > DEFAULT_PAGE_COUNT) {
        page_option_count = DEFAULT_PAGE_COUNT;
    }
    if (page_option_count == 0u) {
        page_option_count = 1u;
    }

    strlcpy(page_names, "MENU", sizeof(page_names));
    for (uint8_t i = 1; i < page_option_count; ++i) {
        char option[16];
        snprintf(option, sizeof(option), "\nGAUGE %u", (unsigned)i);
        strlcat(page_names, option, sizeof(page_names));
    }

    if (selected_default_page >= page_option_count) {
        selected_default_page = 0u;
    }

    ui_settings_set_header("DASHBOARD", "Startup and polling", true);

    panel = ui_settings_create_content_panel(s_settings_content, content_w, panel_h, panel_radius);
    ui_settings_add_panel_title(panel, "BOOT PAGE", "Landing page after startup");

    s_roller_page = lv_roller_create(panel);
    lv_roller_set_options(s_roller_page, page_names, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(s_roller_page, selected_default_page, LV_ANIM_OFF);
    lv_obj_set_width(s_roller_page, content_w - ui_layout_px(72));
    ui_settings_apply_roller_theme(s_roller_page, (uint16_t)ui_layout_px(12));
    lv_obj_add_event_cb(s_roller_page, on_page_roller_change, LV_EVENT_VALUE_CHANGED, NULL);

    panel = ui_settings_create_content_panel(s_settings_content, content_w, panel_h, panel_radius);
    ui_settings_add_panel_title(panel, "OBD POLL", "Balance latency and adapter stability");

    s_roller_obd_poll = lv_roller_create(panel);
    lv_roller_set_options(s_roller_obd_poll, "NORMAL\nFAST", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(s_roller_obd_poll, nvs_cfg_get_obd_poll_mode(cfg), LV_ANIM_OFF);
    lv_obj_set_width(s_roller_obd_poll, content_w - ui_layout_px(72));
    ui_settings_apply_roller_theme(s_roller_obd_poll, (uint16_t)ui_layout_px(12));
    lv_obj_add_event_cb(s_roller_obd_poll, on_obd_poll_roller_change, LV_EVENT_VALUE_CHANGED, NULL);
}

static void ui_settings_build_vehicle(lv_coord_t content_w)
{
    const nvs_user_cfg_t *cfg = nvs_cfg_get();
    const vehicle_profile_t *vehicle_list;
    char vehicle_names[128] = {0};
    uint8_t vehicle_count = 0;
    lv_coord_t panel_radius = ui_layout_px(18);
    lv_coord_t panel_h = ui_layout_px(124);
    lv_obj_t *panel;

    vehicle_list = vehicle_profile_get_all(&vehicle_count);
    for (uint8_t i = 0; i < vehicle_count; ++i) {
        if (i > 0u) {
            strlcat(vehicle_names, "\n", sizeof(vehicle_names));
        }
        strlcat(vehicle_names, vehicle_list[i].name, sizeof(vehicle_names));
    }

    ui_settings_set_header("VEHICLE", "Profile-aware dashboard support", true);

    panel = ui_settings_create_content_panel(s_settings_content, content_w, panel_h, panel_radius);
    ui_settings_add_panel_title(panel, "VEHICLE PROFILE", "Changes visible metric compatibility");

    s_roller_vehicle = lv_roller_create(panel);
    lv_roller_set_options(s_roller_vehicle, vehicle_names, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(s_roller_vehicle,
                           (cfg->vehicle_profile_idx < vehicle_count) ? cfg->vehicle_profile_idx : 0u,
                           LV_ANIM_OFF);
    lv_obj_set_width(s_roller_vehicle, content_w - ui_layout_px(72));
    ui_settings_apply_roller_theme(s_roller_vehicle, (uint16_t)ui_layout_px(12));
    lv_obj_add_event_cb(s_roller_vehicle, on_vehicle_roller_change, LV_EVENT_VALUE_CHANGED, NULL);
}

static void ui_settings_show_section(ui_settings_section_t section)
{
    lv_coord_t content_w = 0;

    if (s_settings_content == NULL) {
        return;
    }

    s_settings_section = section;
    s_roller_page = NULL;
    s_roller_vehicle = NULL;
    s_roller_obd_poll = NULL;
#if CONFIG_OBD_BOARD_WS_175_AMOLED
    s_roller_rotation = NULL;
#endif
    s_slider_bright = NULL;
    s_label_bright_val = NULL;

    lv_obj_clean(s_settings_content);
    content_w = lv_obj_get_width(s_settings_content);

    switch (section) {
    case UI_SETTINGS_SECTION_DISPLAY:
        ui_settings_build_display(content_w);
        break;
    case UI_SETTINGS_SECTION_DASHBOARD:
        ui_settings_build_dashboard(content_w);
        break;
    case UI_SETTINGS_SECTION_VEHICLE:
        ui_settings_build_vehicle(content_w);
        break;
    case UI_SETTINGS_SECTION_ROOT:
    default:
        ui_settings_build_root(content_w);
        break;
    }
}

static lv_obj_t *ui_settings_create_category_card(lv_obj_t *parent,
                                                  const char *title,
                                                  const char *subtitle,
                                                  ui_settings_section_t target_section,
                                                  lv_coord_t width,
                                                  lv_coord_t min_height,
                                                  lv_coord_t radius)
{
    lv_obj_t *card = lv_btn_create(parent);
    lv_obj_t *title_label;
    lv_obj_t *subtitle_label;
    lv_obj_t *arrow_label;

    lv_obj_set_width(card, width);
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(card, min_height, LV_PART_MAIN);
    lv_obj_set_style_radius(card, radius, LV_PART_MAIN);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x151515), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(0x313131), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(card, ui_layout_px(14), LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(card, ui_layout_px(14), LV_PART_MAIN);
    lv_obj_set_style_pad_left(card, ui_layout_px(18), LV_PART_MAIN);
    lv_obj_set_style_pad_right(card, ui_layout_px(18), LV_PART_MAIN);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1E1E1E), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(card, ui_settings_open_section, LV_EVENT_CLICKED, (void *)(uintptr_t)target_section);

    lv_obj_t *text_col = lv_obj_create(card);
    lv_obj_remove_style_all(text_col);
    lv_obj_set_size(text_col, LV_PCT(82), LV_SIZE_CONTENT);
    lv_obj_clear_flag(text_col, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(text_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(text_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(text_col, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(text_col, ui_layout_px(4), LV_PART_MAIN);

    title_label = lv_label_create(text_col);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    subtitle_label = lv_label_create(text_col);
    lv_label_set_text(subtitle_label, subtitle);
    lv_obj_set_style_text_font(subtitle_label, ui_font_hint(11), LV_PART_MAIN);
    lv_obj_set_style_text_color(subtitle_label, lv_color_hex(0x8F8F8F), LV_PART_MAIN);
    lv_obj_set_width(subtitle_label, LV_PCT(100));
    lv_obj_set_style_text_align(subtitle_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

    arrow_label = lv_label_create(card);
    lv_label_set_text(arrow_label, ">");
    lv_obj_set_style_text_font(arrow_label, ui_font_typoder(24), LV_PART_MAIN);
    lv_obj_set_style_text_color(arrow_label, lv_color_hex(0xD8D8D8), LV_PART_MAIN);

    return card;
}

void ui_ScreenPageSettings_screen_init(void)
{
    ui_settings_layout_t layout;
    lv_obj_t *title;
    lv_obj_t *subtitle;
    lv_obj_t *back_label;

    ui_settings_layout_get(&layout);

    ui_ScreenPageSettings = lv_obj_create(NULL);
    ui_round_screen_apply_base(ui_ScreenPageSettings, lv_color_hex(0x000000));
    ui_round_shell_create_ring(ui_ScreenPageSettings, &layout.shell);
    lv_obj_add_event_cb(ui_ScreenPageSettings,
                        scr_unloaded_delete_cb,
                        LV_EVENT_SCREEN_UNLOADED,
                        &ui_ScreenPageSettings);
    lv_obj_add_event_cb(ui_ScreenPageSettings, ui_settings_screen_deleted, LV_EVENT_DELETE, NULL);
    lv_obj_add_event_cb(ui_ScreenPageSettings, on_settings_background, LV_EVENT_GESTURE, NULL);

    s_settings_back_btn = lv_btn_create(ui_ScreenPageSettings);
    lv_obj_set_size(s_settings_back_btn, ui_layout_px(82), ui_layout_px(32));
    lv_obj_align(s_settings_back_btn, LV_ALIGN_TOP_LEFT, ui_safe_margin() + ui_layout_px(10), ui_safe_margin() + ui_layout_px(8));
    lv_obj_set_style_radius(s_settings_back_btn, ui_layout_px(16), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_settings_back_btn, lv_color_hex(0x161616), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_settings_back_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_settings_back_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_settings_back_btn, lv_color_hex(0x2D2D2D), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_settings_back_btn, 0, LV_PART_MAIN);
    lv_obj_add_flag(s_settings_back_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_settings_back_btn, ui_settings_back_click, LV_EVENT_CLICKED, NULL);

    back_label = lv_label_create(s_settings_back_btn);
    lv_label_set_text(back_label, "BACK");
    lv_obj_set_style_text_font(back_label, ui_font_typoder(14), LV_PART_MAIN);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(back_label);

    title = lv_label_create(ui_ScreenPageSettings);
    lv_label_set_text(title, "SETTINGS");
    lv_obj_set_style_text_font(title, ui_font_typoder(24), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, ui_safe_margin() + ui_layout_px(8));
    s_settings_header_title = title;

    subtitle = lv_label_create(ui_ScreenPageSettings);
    lv_label_set_text(subtitle, "Choose a category");
    lv_obj_set_style_text_font(subtitle, ui_font_hint(12), LV_PART_MAIN);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x8D8D8D), LV_PART_MAIN);
    lv_obj_align_to(subtitle, title, LV_ALIGN_OUT_BOTTOM_MID, 0, ui_layout_px(6));
    s_settings_header_subtitle = subtitle;

    ui_settings_prepare_content_root(NULL);
    ui_settings_show_section(UI_SETTINGS_SECTION_ROOT);
    aux_sensor_demand_refresh();
}
