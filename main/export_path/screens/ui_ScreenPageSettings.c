// Settings Page
// Configure: default boot page and screen brightness

#include "../ui.h"
#include "../ui_font_profile.h"
#include "../ui_home_runtime.h"
#include "../ui_layout.h"
#include "../ui_round_shell.h"
#include <string.h>
#include "bsp_obd_dsp/boards/board_api.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "app_obd_dsp/vehicle_profiles.h"

// Local references for settings widgets
static lv_obj_t *s_roller_page = NULL;
static lv_obj_t *s_roller_vehicle = NULL;
static lv_obj_t *s_slider_bright = NULL;
static lv_obj_t *s_label_bright_val = NULL;

static void ui_settings_screen_reset_state(void)
{
    ui_ScreenPageSettings = NULL;
    s_roller_page = NULL;
    s_roller_vehicle = NULL;
    s_slider_bright = NULL;
    s_label_bright_val = NULL;
}

static void ui_settings_screen_deleted(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_settings_screen_reset_state();
}

// Callbacks
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
    if(val < 10) val = 10;
    lv_label_set_text_fmt(s_label_bright_val, "%ld%%", val);
    nvs_user_cfg_t cfg = *nvs_cfg_get();
    cfg.brightness_day = (uint8_t)val;
    nvs_cfg_set(&cfg);
    board_set_brightness((uint8_t)val);
}

static void on_vehicle_roller_change(lv_event_t *e)
{
    uint8_t selected = (uint8_t)lv_roller_get_selected(s_roller_vehicle);
    // vehicle_profile_set_active 会对越界索引归零，这里无需再 clamp。

    nvs_user_cfg_t cfg = *nvs_cfg_get();
    cfg.vehicle_profile_idx = selected;
    nvs_cfg_set(&cfg);

    // 同步更新当前激活车型，档位识别等运行时逻辑立即生效。
    vehicle_profile_set_active(selected);
}

static void on_settings_background(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
        return;
    }

    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_LEFT) {
        lv_indev_wait_release(lv_indev_get_act());
        ui_home_runtime_show_page(UI_HOME_PAGE_MENU_ID, LV_SCR_LOAD_ANIM_MOVE_LEFT);
    }
}

void ui_ScreenPageSettings_screen_init(void)
{
    const nvs_user_cfg_t *cfg = nvs_cfg_get();
    const ui_dashboard_cfg_t *dashboard_cfg = &cfg->dashboard_cfg;
    ui_settings_layout_t layout;
    char page_names[64] = {0};
    uint8_t page_option_count = (uint8_t)(dashboard_cfg->gauge_page_count + 1u);
    uint8_t selected_default_page = cfg->default_page;
    ui_settings_layout_get(&layout);

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

    ui_ScreenPageSettings = lv_obj_create(NULL);
    ui_round_screen_apply_base(ui_ScreenPageSettings, lv_color_hex(0x000000));
    ui_round_shell_create_ring(ui_ScreenPageSettings, &layout.shell);
    lv_obj_add_event_cb(ui_ScreenPageSettings,
                        scr_unloaded_delete_cb,
                        LV_EVENT_SCREEN_UNLOADED,
                        &ui_ScreenPageSettings);
    lv_obj_add_event_cb(ui_ScreenPageSettings, ui_settings_screen_deleted, LV_EVENT_DELETE, NULL);

    // ====== Title ======
    lv_obj_t *title = lv_label_create(ui_ScreenPageSettings);
    lv_label_set_text(title, "SETTINGS");
    lv_obj_set_style_text_font(title, ui_font_typoder(24), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, layout.title_y);

    // ====== Row 1: Default Page (Boot Page) ======
    lv_obj_t *label_page = lv_label_create(ui_ScreenPageSettings);
    lv_label_set_text(label_page, "BOOT PAGE");
    lv_obj_set_style_text_font(label_page, ui_font_hint(12), LV_PART_MAIN);
    lv_obj_set_style_text_color(label_page, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(label_page, LV_ALIGN_CENTER, 0, layout.label_page_y);

    s_roller_page = lv_roller_create(ui_ScreenPageSettings);
    lv_obj_clear_flag(s_roller_page, LV_OBJ_FLAG_GESTURE_BUBBLE); // 滚动选值时不触发页面手势
    lv_roller_set_options(s_roller_page, page_names, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_roller_page, 1);
    lv_roller_set_selected(s_roller_page, selected_default_page, LV_ANIM_OFF);
    lv_obj_set_width(s_roller_page, layout.roller_width);
    lv_obj_set_style_text_font(s_roller_page, ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_roller_page, ui_font_typoder(20), LV_PART_SELECTED);
    ui_round_shell_apply_roller_theme(s_roller_page,
                                      layout.roller_radius,
                                      lv_color_hex(0x222222),
                                      lv_color_hex(0x444444),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0xCFCFCF),
                                      lv_color_hex(0x000000));
    lv_obj_align(s_roller_page, LV_ALIGN_CENTER, 0, layout.roller_page_y);
    lv_obj_add_event_cb(s_roller_page, on_page_roller_change, LV_EVENT_VALUE_CHANGED, NULL);

    // ====== Divider ======
    lv_obj_t *div1 = lv_obj_create(ui_ScreenPageSettings);
    lv_obj_remove_style_all(div1);
    lv_obj_set_size(div1, layout.divider_width, 1);
    lv_obj_align(div1, LV_ALIGN_CENTER, 0, layout.divider1_y);
    lv_obj_set_style_bg_color(div1, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(div1, 40, LV_PART_MAIN);
    lv_obj_clear_flag(div1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // ====== Row 2: Vehicle ======
    lv_obj_t *label_vehicle = lv_label_create(ui_ScreenPageSettings);
    lv_label_set_text(label_vehicle, "VEHICLE");
    lv_obj_set_style_text_font(label_vehicle, ui_font_hint(12), LV_PART_MAIN);
    lv_obj_set_style_text_color(label_vehicle, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(label_vehicle, LV_ALIGN_CENTER, 0, layout.label_vehicle_y);

    // 从车型配置动态生成 roller 选项（换行分隔），新增车型自动出现
    uint8_t vehicle_count = 0;
    const vehicle_profile_t *vehicle_list = vehicle_profile_get_all(&vehicle_count);
    char vehicle_names[128] = {0};
    for (uint8_t i = 0; i < vehicle_count; i++) {
        if (i > 0) strlcat(vehicle_names, "\n", sizeof(vehicle_names));
        strlcat(vehicle_names, vehicle_list[i].name, sizeof(vehicle_names));
    }

    s_roller_vehicle = lv_roller_create(ui_ScreenPageSettings);
    lv_obj_clear_flag(s_roller_vehicle, LV_OBJ_FLAG_GESTURE_BUBBLE); // 滚动选值时不触发页面手势
    lv_roller_set_options(s_roller_vehicle, vehicle_names, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_roller_vehicle, 1);
    lv_roller_set_selected(s_roller_vehicle, (cfg->vehicle_profile_idx < vehicle_count) ? cfg->vehicle_profile_idx : 0, LV_ANIM_OFF);
    lv_obj_set_width(s_roller_vehicle, layout.roller_width);
    lv_obj_set_style_text_font(s_roller_vehicle, ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_roller_vehicle, ui_font_typoder(20), LV_PART_SELECTED);
    ui_round_shell_apply_roller_theme(s_roller_vehicle,
                                      layout.roller_radius,
                                      lv_color_hex(0x222222),
                                      lv_color_hex(0x444444),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0xCFCFCF),
                                      lv_color_hex(0x000000));
    lv_obj_align(s_roller_vehicle, LV_ALIGN_CENTER, 0, layout.roller_vehicle_y);
    lv_obj_add_event_cb(s_roller_vehicle, on_vehicle_roller_change, LV_EVENT_VALUE_CHANGED, NULL);

    // ====== Divider ======
    lv_obj_t *div2 = lv_obj_create(ui_ScreenPageSettings);
    lv_obj_remove_style_all(div2);
    lv_obj_set_size(div2, layout.divider_width, 1);
    lv_obj_align(div2, LV_ALIGN_CENTER, 0, layout.divider2_y);
    lv_obj_set_style_bg_color(div2, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(div2, 40, LV_PART_MAIN);
    lv_obj_clear_flag(div2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // ====== Row 3: Brightness ======
    lv_obj_t *label_bright = lv_label_create(ui_ScreenPageSettings);
    lv_label_set_text(label_bright, "BRIGHTNESS");
    lv_obj_set_style_text_font(label_bright, ui_font_hint(12), LV_PART_MAIN);
    lv_obj_set_style_text_color(label_bright, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(label_bright, LV_ALIGN_CENTER, 0, layout.label_brightness_y);

    s_slider_bright = lv_slider_create(ui_ScreenPageSettings);
    lv_slider_set_range(s_slider_bright, 10, 100);
    lv_slider_set_value(s_slider_bright, cfg->brightness_day, LV_ANIM_OFF);
    lv_obj_set_width(s_slider_bright, layout.slider_width);
    lv_obj_set_height(s_slider_bright, layout.slider_height);
    lv_obj_align(s_slider_bright, LV_ALIGN_CENTER, 0, layout.slider_y);
    lv_obj_set_style_bg_color(s_slider_bright, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_slider_bright, 255, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_slider_bright, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_slider_bright, 255, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_slider_bright, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_slider_bright, layout.slider_knob_pad, LV_PART_KNOB);
    lv_obj_clear_flag(s_slider_bright, LV_OBJ_FLAG_GESTURE_BUBBLE);      // 防止滑块拖拽触发页面手势
    lv_obj_add_event_cb(s_slider_bright, on_bright_slider_change, LV_EVENT_VALUE_CHANGED, NULL);

    s_label_bright_val = lv_label_create(ui_ScreenPageSettings);
    lv_label_set_text_fmt(s_label_bright_val, "%d%%", cfg->brightness_day);
    lv_obj_set_style_text_font(s_label_bright_val, ui_font_typoder(24), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_bright_val, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(s_label_bright_val, LV_ALIGN_CENTER, 0, layout.brightness_value_y);

    // ====== Hint ======
    lv_obj_t *hint = lv_label_create(ui_ScreenPageSettings);
    lv_label_set_text(hint, "Swipe left to go back\nVehicle applies after reconnect");
    lv_obj_set_style_text_font(hint, ui_font_hint(12), LV_PART_MAIN);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x555555), LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, layout.hint_y);

    // Events - swipe to go back
    lv_obj_add_event_cb(ui_ScreenPageSettings, on_settings_background, LV_EVENT_GESTURE, NULL);
}
