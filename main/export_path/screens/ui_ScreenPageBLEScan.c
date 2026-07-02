// BLE Scan & Select Page
// Shows saved device and nearby scan results on a round-screen-safe layout.

#include "../ui.h"
#include "../ui_font_profile.h"
#include "../ui_home_runtime.h"
#include "../ui_layout.h"
#include "../ui_round_shell.h"
#include "app_obd_dsp/aux_sensor_demand.h"
#include "bsp_obd_dsp/elm327_ble_client.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG_BLE_UI = "ble_scan_ui";

static lv_obj_t *s_list = NULL;
static lv_obj_t *s_label_nearby = NULL;
static lv_obj_t *s_spinner = NULL;
static lv_obj_t *s_saved_panel = NULL;
static lv_obj_t *s_label_saved_hdr = NULL;
static lv_obj_t *s_saved_name_lbl = NULL;
static bool s_scanning = false;

static void start_scan(void);
static void on_device_selected(lv_event_t *e);
static void on_saved_device_delete(lv_event_t *e);
static void on_ble_scan_background(lv_event_t *e);
static void ui_ble_scan_screen_reset_state(void);
static void ui_ble_scan_screen_deleted(lv_event_t *e);

extern bool app_lvgl_lock(int timeout_ms);
extern void app_lvgl_unlock(void);

static inline bool lvgl_lock_ui(int timeout_ms)
{
    return app_lvgl_lock(timeout_ms);
}

static inline void lvgl_unlock_ui(void)
{
    app_lvgl_unlock();
}

static void ui_ble_scan_update_nearby_count(int total_count)
{
    if (s_label_nearby == NULL) {
        return;
    }

    if (total_count > 0) {
        lv_label_set_text_fmt(s_label_nearby, "NEARBY (%d)", total_count);
    } else {
        lv_label_set_text(s_label_nearby, "NEARBY");
    }
}

static void scan_result_cb(const ble_scan_result_t *dev, int total_count)
{
    if (s_list == NULL) {
        return;
    }

    if (!lvgl_lock_ui(100)) {
        return;
    }

    uint32_t child_cnt = lv_obj_get_child_cnt(s_list);
    for (uint32_t i = 0; i < child_cnt; ++i) {
        lv_obj_t *btn = lv_obj_get_child(s_list, i);
        lv_obj_t *lbl = lv_obj_get_child(btn, 0);
        if (lbl != NULL && strcmp(lv_label_get_text(lbl), dev->name) == 0) {
            lvgl_unlock_ui();
            return;
        }
    }

    lv_obj_t *btn = lv_list_add_btn(s_list, NULL, dev->name);
    ui_round_shell_apply_list_button_theme(btn, 20);
    lv_obj_add_event_cb(btn, on_device_selected, LV_EVENT_CLICKED, NULL);

    ui_ble_scan_update_nearby_count(total_count);
    lvgl_unlock_ui();
}

static void on_device_selected(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    if (lbl == NULL) {
        return;
    }

    const char *name = lv_label_get_text(lbl);
    ESP_LOGI(TAG_BLE_UI, "Selected BLE device: %s", name);

    elm327_ble_scan_only_stop();
    s_scanning = false;

    nvs_user_cfg_t cfg = *nvs_cfg_get();
    strncpy(cfg.ble_device_name, name, sizeof(cfg.ble_device_name) - 1u);
    cfg.ble_device_name[sizeof(cfg.ble_device_name) - 1u] = '\0';
    nvs_cfg_set(&cfg);

    if (s_saved_name_lbl != NULL) {
        lv_label_set_text(s_saved_name_lbl, name);
    }
    if (s_saved_panel != NULL) {
        lv_obj_clear_flag(s_saved_panel, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_label_saved_hdr != NULL) {
        lv_obj_clear_flag(s_label_saved_hdr, LV_OBJ_FLAG_HIDDEN);
    }

    if (s_spinner != NULL) {
        lv_obj_clear_flag(s_spinner, LV_OBJ_FLAG_HIDDEN);
    }

    elm327_ble_connect_by_name(name);
    ui_home_runtime_show_page(UI_HOME_PAGE_MENU_ID, LV_SCR_LOAD_ANIM_FADE_ON);
}

static void on_saved_device_delete(lv_event_t *e)
{
    LV_UNUSED(e);

    if (elm327_ble_is_connected()) {
        elm327_ble_disconnect();
    }

    nvs_user_cfg_t cfg = *nvs_cfg_get();
    cfg.ble_device_name[0] = '\0';
    nvs_cfg_set(&cfg);
    ESP_LOGI(TAG_BLE_UI, "Saved BLE device cleared");

    if (s_saved_panel != NULL) {
        lv_obj_add_flag(s_saved_panel, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_label_saved_hdr != NULL) {
        lv_obj_add_flag(s_label_saved_hdr, LV_OBJ_FLAG_HIDDEN);
    }
}

static void start_scan(void)
{
    if (s_scanning) {
        return;
    }

    s_scanning = true;

    if (s_list != NULL) {
        lv_obj_clean(s_list);
    }
    ui_ble_scan_update_nearby_count(0);
    if (s_spinner != NULL) {
        lv_obj_clear_flag(s_spinner, LV_OBJ_FLAG_HIDDEN);
    }

    elm327_ble_scan_only_start(15, scan_result_cb);
}

static void on_ble_scan_background(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
        return;
    }

    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir != LV_DIR_TOP) {
        return;
    }

    elm327_ble_scan_only_stop();
    lv_indev_wait_release(lv_indev_get_act());
    ui_home_runtime_show_page(UI_HOME_PAGE_MENU_ID, LV_SCR_LOAD_ANIM_FADE_ON);
}

static void ui_ble_scan_screen_reset_state(void)
{
    ui_ScreenPageBLEScan = NULL;
    s_list = NULL;
    s_label_nearby = NULL;
    s_spinner = NULL;
    s_saved_panel = NULL;
    s_label_saved_hdr = NULL;
    s_saved_name_lbl = NULL;
    s_scanning = false;
}

static void ui_ble_scan_screen_deleted(lv_event_t *e)
{
    LV_UNUSED(e);

    if (s_scanning) {
        elm327_ble_scan_only_stop();
    }
    ui_ble_scan_screen_reset_state();
}

void ui_ScreenPageBLEScan_screen_init(void)
{
    ui_ble_scan_layout_t layout;
    ui_ble_scan_layout_get(&layout);

    ui_ScreenPageBLEScan = lv_obj_create(NULL);
    ui_round_screen_apply_base(ui_ScreenPageBLEScan, lv_color_hex(0x000000));
    ui_round_shell_create_ring(ui_ScreenPageBLEScan, &layout.shell);
    lv_obj_add_event_cb(ui_ScreenPageBLEScan,
                        scr_unloaded_delete_cb,
                        LV_EVENT_SCREEN_UNLOADED,
                        &ui_ScreenPageBLEScan);
    lv_obj_add_event_cb(ui_ScreenPageBLEScan, ui_ble_scan_screen_deleted, LV_EVENT_DELETE, NULL);

    ui_round_shell_create_title_block(ui_ScreenPageBLEScan,
                                      "BLE SCAN",
                                      NULL,
                                      ui_safe_margin() + ui_layout_px(12),
                                      20,
                                      ui_layout_px(4),
                                      11,
                                      NULL,
                                      NULL);

    s_spinner = lv_spinner_create(ui_ScreenPageBLEScan, 1000, 60);
    lv_obj_set_size(s_spinner, ui_layout_px(18), ui_layout_px(18));
    lv_obj_align(s_spinner, LV_ALIGN_TOP_RIGHT, -(ui_safe_margin() + ui_layout_px(38)), layout.nearby_y - ui_layout_px(118));
    lv_obj_set_style_arc_color(s_spinner, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_spinner, ui_layout_px(2), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_spinner, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_spinner, ui_layout_px(2), LV_PART_MAIN);

    const nvs_user_cfg_t *saved_cfg = nvs_cfg_get();
    bool has_saved = (saved_cfg->ble_device_name[0] != '\0');

    s_label_saved_hdr = lv_label_create(ui_ScreenPageBLEScan);
    lv_label_set_text(s_label_saved_hdr, "SAVED DEVICE");
    lv_obj_set_style_text_font(s_label_saved_hdr, ui_font_hint(12), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_saved_hdr, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(s_label_saved_hdr, LV_ALIGN_TOP_MID, 0, layout.saved_header_y);
    if (!has_saved) {
        lv_obj_add_flag(s_label_saved_hdr, LV_OBJ_FLAG_HIDDEN);
    }

    s_saved_panel = lv_obj_create(ui_ScreenPageBLEScan);
    lv_obj_remove_style_all(s_saved_panel);
    lv_obj_set_size(s_saved_panel, layout.saved_panel_width, layout.saved_panel_height);
    lv_obj_align(s_saved_panel, LV_ALIGN_TOP_MID, 0, layout.saved_panel_y);
    ui_round_shell_apply_dark_card_theme(s_saved_panel, layout.saved_panel_radius, layout.saved_panel_pad);
    lv_obj_clear_flag(s_saved_panel, LV_OBJ_FLAG_SCROLLABLE);
    if (!has_saved) {
        lv_obj_add_flag(s_saved_panel, LV_OBJ_FLAG_HIDDEN);
    }

    s_saved_name_lbl = lv_label_create(s_saved_panel);
    lv_label_set_text(s_saved_name_lbl, has_saved ? saved_cfg->ble_device_name : "");
    lv_label_set_long_mode(s_saved_name_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(s_saved_name_lbl, layout.saved_panel_width - layout.delete_button_width - ui_layout_px(28));
    lv_obj_set_style_text_font(s_saved_name_lbl, ui_font_typoder(18), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_saved_name_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(s_saved_name_lbl, LV_ALIGN_LEFT_MID, layout.saved_name_x, 0);

    lv_obj_t *del_btn = lv_btn_create(s_saved_panel);
    lv_obj_set_size(del_btn, layout.delete_button_width, layout.delete_button_height);
    lv_obj_align(del_btn, LV_ALIGN_RIGHT_MID, layout.delete_button_x, 0);
    ui_round_shell_apply_danger_button_theme(del_btn, layout.delete_button_radius, layout.delete_button_pad);
    lv_obj_t *del_lbl = lv_label_create(del_btn);
    lv_label_set_text(del_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(del_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(del_lbl);
    lv_obj_add_event_cb(del_btn, on_saved_device_delete, LV_EVENT_CLICKED, NULL);

    lv_obj_t *divider = lv_obj_create(ui_ScreenPageBLEScan);
    lv_obj_remove_style_all(divider);
    lv_obj_set_size(divider, layout.divider_width, 1);
    lv_obj_align(divider, LV_ALIGN_TOP_MID, 0, layout.divider_y);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(divider, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    s_label_nearby = lv_label_create(ui_ScreenPageBLEScan);
    lv_label_set_text(s_label_nearby, "NEARBY");
    lv_obj_set_style_text_font(s_label_nearby, ui_font_hint(12), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_nearby, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(s_label_nearby, LV_ALIGN_TOP_MID, 0, layout.nearby_y);

    s_list = lv_list_create(ui_ScreenPageBLEScan);
    lv_obj_set_size(s_list, layout.list_width, layout.list_height);
    lv_obj_align(s_list, LV_ALIGN_TOP_MID, 0, layout.list_y);
    lv_obj_add_flag(s_list, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_bg_color(s_list, lv_color_hex(0x111111), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_list, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_list, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_list, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_list, layout.list_pad, LV_PART_MAIN);
    lv_obj_set_style_radius(s_list, layout.list_radius, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(s_list, true, LV_PART_MAIN);

    lv_obj_add_event_cb(ui_ScreenPageBLEScan, on_ble_scan_background, LV_EVENT_GESTURE, NULL);
    aux_sensor_demand_refresh();
    start_scan();
}
