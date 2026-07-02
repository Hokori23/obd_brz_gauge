#include "ui_dashboard_config.h"

#include "ui.h"
#include "ui_font_profile.h"
#include "ui_helpers.h"
#include "ui_layout.h"
#include "ui_round_shell.h"
#include "ui_home_runtime.h"
#include "ui_runtime_common.h"

#include <math.h>
#include <string.h>

#include "app_obd_dsp/aux_sensor_demand.h"
#include "bsp_obd_dsp/nvs_storage.h"

static lv_obj_t *s_dashboard_config_screen = NULL;
static uint8_t s_dashboard_config_gauge_index = 0;
static lv_obj_t *s_dashboard_cfg_type_roller = NULL;
static lv_obj_t *s_dashboard_cfg_type_row = NULL;
static lv_obj_t *s_dashboard_cfg_slot_count_roller = NULL;
static lv_obj_t *s_dashboard_cfg_slot_count_row = NULL;
static lv_obj_t *s_dashboard_cfg_slot_item_rollers[UI_DASHBOARD_MAX_SLOTS] = {0};
static lv_obj_t *s_dashboard_cfg_slot_rows[UI_DASHBOARD_MAX_SLOTS] = {0};
static lv_obj_t *s_dashboard_cfg_body = NULL;
static uint8_t s_dashboard_cfg_supported_items[DISP_ITEM_COUNT] = {0};
static uint8_t s_dashboard_cfg_supported_item_count = 0;

static void ui_dashboard_config_rebuild_supported_items(void)
{
    uint8_t vehicle_profile_idx = nvs_cfg_get()->vehicle_profile_idx;

    s_dashboard_cfg_supported_item_count = 0;
    for (uint8_t i = 0; i < DISP_ITEM_COUNT; ++i) {
        if (!ui_dashboard_item_supported_for_vehicle(vehicle_profile_idx, i)) {
            continue;
        }
        s_dashboard_cfg_supported_items[s_dashboard_cfg_supported_item_count++] = i;
    }

    if (s_dashboard_cfg_supported_item_count == 0u) {
        s_dashboard_cfg_supported_items[0] = DISP_ITEM_RPM;
        s_dashboard_cfg_supported_item_count = 1u;
    }
}

static uint8_t ui_dashboard_config_default_supported_item(void)
{
    for (uint8_t i = 0; i < s_dashboard_cfg_supported_item_count; ++i) {
        if (s_dashboard_cfg_supported_items[i] == DISP_ITEM_RPM) {
            return DISP_ITEM_RPM;
        }
    }

    return s_dashboard_cfg_supported_items[0];
}

static uint16_t ui_dashboard_config_selected_index_for_item(uint8_t item)
{
    for (uint16_t i = 0; i < s_dashboard_cfg_supported_item_count; ++i) {
        if (s_dashboard_cfg_supported_items[i] == item) {
            return i;
        }
    }

    return 0;
}

static void ui_dashboard_config_screen_change_with_anim(lv_obj_t **target_scr,
                                                        lv_scr_load_anim_t anim,
                                                        void (*target_init)(void))
{
    _ui_screen_change(target_scr, anim, 200, 0, target_init);
}

static lv_coord_t ui_dashboard_config_circle_left_at_y(lv_coord_t y)
{
    int32_t r = (int32_t)ui_round_radius();
    int32_t dy = (int32_t)y - r;
    int32_t r2_dy2 = r * r - dy * dy;
    if (r2_dy2 <= 0) {
        return 0;
    }
    return (lv_coord_t)(r - (int32_t)sqrtf((float)r2_dy2));
}

static lv_coord_t ui_dashboard_config_circle_right_at_y(lv_coord_t y)
{
    return (lv_coord_t)(ui_screen_width() - ui_dashboard_config_circle_left_at_y(y));
}

static void ui_dashboard_config_safe_span_for_band(lv_coord_t y,
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
        lv_coord_t sample_left = ui_dashboard_config_circle_left_at_y(samples[i]) + inset;
        lv_coord_t sample_right = ui_dashboard_config_circle_right_at_y(samples[i]) - inset;
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

static uint8_t ui_dashboard_config_get_return_page_id(void)
{
    const ui_dashboard_cfg_t *dashboard = &nvs_cfg_get()->dashboard_cfg;

    if (dashboard->gauge_page_count == 0u) {
        return UI_HOME_PAGE_MENU_ID;
    }
    if (s_dashboard_config_gauge_index < dashboard->gauge_page_count) {
        return (uint8_t)(s_dashboard_config_gauge_index + 1u);
    }

    return UI_HOME_PAGE_MENU_ID;
}

static const ui_dashboard_page_cfg_t *ui_dashboard_config_get_page_cfg(uint8_t gauge_index)
{
    const ui_dashboard_cfg_t *dashboard = &nvs_cfg_get()->dashboard_cfg;
    if (gauge_index >= dashboard->gauge_page_count || gauge_index >= UI_DASHBOARD_MAX_PAGES) {
        return NULL;
    }
    return &dashboard->pages[gauge_index];
}

static void ui_dashboard_config_refresh_rows(void)
{
    uint8_t slot_count = 1u;
    ui_dashboard_page_type_t page_type = UI_DASHBOARD_PAGE_TYPE_METRIC;

    if (s_dashboard_cfg_slot_count_roller != NULL) {
        slot_count = (uint8_t)(lv_roller_get_selected(s_dashboard_cfg_slot_count_roller) + 1u);
    }
    if (s_dashboard_cfg_type_roller != NULL) {
        page_type = (ui_dashboard_page_type_t)lv_roller_get_selected(s_dashboard_cfg_type_roller);
        if (page_type >= UI_DASHBOARD_PAGE_TYPE_COUNT) {
            page_type = UI_DASHBOARD_PAGE_TYPE_METRIC;
        }
    }

    if (s_dashboard_cfg_slot_count_row != NULL) {
        if (page_type == UI_DASHBOARD_PAGE_TYPE_METRIC) {
            lv_obj_clear_flag(s_dashboard_cfg_slot_count_row, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_dashboard_cfg_slot_count_row, LV_OBJ_FLAG_HIDDEN);
        }
    }

    for (uint8_t i = 0; i < UI_DASHBOARD_MAX_SLOTS; ++i) {
        if (s_dashboard_cfg_slot_rows[i] == NULL) {
            continue;
        }
        if (page_type == UI_DASHBOARD_PAGE_TYPE_METRIC && i < slot_count) {
            lv_obj_clear_flag(s_dashboard_cfg_slot_rows[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_dashboard_cfg_slot_rows[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void ui_dashboard_config_type_changed(lv_event_t *e)
{
    nvs_user_cfg_t cfg;
    ui_dashboard_page_type_t page_type;

    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    cfg = *nvs_cfg_get();
    if (s_dashboard_config_gauge_index >= cfg.dashboard_cfg.gauge_page_count) {
        return;
    }

    page_type = (ui_dashboard_page_type_t)lv_roller_get_selected(s_dashboard_cfg_type_roller);
    ui_dashboard_page_set_type(&cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index], page_type);
    nvs_cfg_set(&cfg);
    ui_dashboard_config_refresh_rows();
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
        uint8_t default_item = ui_dashboard_config_default_supported_item();
        for (uint8_t i = current_page->slot_count; i < slot_count; ++i) {
            cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index].slot_items[i] = default_item;
            if (s_dashboard_cfg_slot_item_rollers[i] != NULL) {
                lv_roller_set_selected(s_dashboard_cfg_slot_item_rollers[i],
                                       ui_dashboard_config_selected_index_for_item(default_item),
                                       LV_ANIM_OFF);
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
        s_dashboard_cfg_supported_items[lv_roller_get_selected(s_dashboard_cfg_slot_item_rollers[slot_index])];
    nvs_cfg_set(&cfg);
}

static void ui_dashboard_config_close_to_home(lv_scr_load_anim_t anim)
{
    ui_home_runtime_rebuild_and_load(ui_dashboard_config_get_return_page_id(), anim);
}

static void ui_dashboard_config_error_back(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    ui_dashboard_config_close_to_home(LV_SCR_LOAD_ANIM_NONE);
}

static lv_obj_t *ui_dashboard_config_create_row(lv_obj_t *parent, lv_coord_t height)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, height);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

static void ui_dashboard_config_background(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
        return;
    }

    if (lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_TOP) {
        lv_indev_wait_release(lv_indev_get_act());
        ui_dashboard_config_close_to_home(LV_SCR_LOAD_ANIM_NONE);
    }
}

static void ui_dashboard_config_deleted(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_dashboard_config_reset();
}

static void ui_dashboard_config_screen_init(void)
{
    const char *type_options = "METRIC\nGEAR\nG-OBD\nG-ESP32";
    const char *slot_count_options = "1\n2\n3\n4\n5\n6";
    char item_options[128] = {0};
    const ui_dashboard_page_cfg_t *page;
    lv_coord_t safe_margin;
    lv_coord_t top_pad;
    lv_coord_t bottom_pad;
    lv_coord_t title_h;
    lv_coord_t title_gap;
    lv_coord_t title_y;
    lv_coord_t body_top;
    lv_coord_t body_h;
    lv_coord_t body_used_h;
    lv_coord_t body_left;
    lv_coord_t body_right;
    lv_coord_t body_w;
    lv_coord_t body_inset;
    lv_coord_t body_row_gap;
    lv_coord_t row_h;
    lv_coord_t roller_font;
    lv_coord_t label_font;
    lv_coord_t count_row_h;
    lv_coord_t slot_row_h;
    lv_coord_t type_row_h;
    lv_coord_t count_roller_w;
    lv_coord_t type_roller_w;
    lv_coord_t item_roller_w;
    lv_coord_t visible_row_radius;
    uint8_t total_row_count = (uint8_t)(2u + UI_DASHBOARD_MAX_SLOTS);

    if (s_dashboard_config_screen != NULL) {
        return;
    }

    ui_dashboard_config_rebuild_supported_items();
    for (uint8_t i = 0; i < s_dashboard_cfg_supported_item_count; ++i) {
        if (i > 0u) {
            strlcat(item_options, "\n", sizeof(item_options));
        }
        strlcat(item_options, ui_disp_item_name(s_dashboard_cfg_supported_items[i]), sizeof(item_options));
    }

    page = ui_dashboard_config_get_page_cfg(s_dashboard_config_gauge_index);

    s_dashboard_config_screen = lv_obj_create(NULL);
    ui_round_screen_apply_base(s_dashboard_config_screen, lv_color_hex(0x000000));
    lv_obj_add_event_cb(s_dashboard_config_screen, ui_dashboard_config_background, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(s_dashboard_config_screen,
                        scr_unloaded_delete_cb,
                        LV_EVENT_SCREEN_UNLOADED,
                        &s_dashboard_config_screen);
    lv_obj_add_event_cb(s_dashboard_config_screen, ui_dashboard_config_deleted, LV_EVENT_DELETE, NULL);

    if (page == NULL) {
        lv_coord_t err_title_font = ui_layout_px(22);
        lv_coord_t err_hint_font = ui_layout_px(12);
        lv_coord_t btn_w;
        lv_coord_t btn_h;
        lv_coord_t btn_y;
        lv_obj_t *err_label = lv_label_create(s_dashboard_config_screen);
        lv_label_set_text(err_label, "No page config");
        lv_obj_set_style_text_font(err_label, ui_font_typoder(err_title_font), LV_PART_MAIN);
        lv_obj_set_style_text_color(err_label, lv_color_hex(0xFF4444), LV_PART_MAIN);
        lv_obj_align(err_label, LV_ALIGN_CENTER, 0, ui_layout_px(-28));

        lv_obj_t *hint_label = lv_label_create(s_dashboard_config_screen);
        lv_label_set_text(hint_label, "Tap back to return");
        lv_obj_set_style_text_font(hint_label, ui_font_hint(err_hint_font), LV_PART_MAIN);
        lv_obj_set_style_text_color(hint_label, lv_color_hex(0xA0A0A0), LV_PART_MAIN);
        lv_obj_align_to(hint_label, err_label, LV_ALIGN_OUT_BOTTOM_MID, 0, ui_layout_px(8));

        btn_w = (lv_coord_t)(ui_screen_width() - ((ui_safe_margin() + ui_layout_px(20)) * 2));
        btn_h = ui_layout_px(38);
        if (btn_w < ui_layout_px(96)) {
            btn_w = ui_layout_px(96);
        }
        btn_y = ui_layout_px(46);

        lv_obj_t *back_btn = lv_btn_create(s_dashboard_config_screen);
        lv_obj_set_size(back_btn, btn_w, btn_h);
        lv_obj_align(back_btn, LV_ALIGN_CENTER, 0, btn_y);
        lv_obj_set_style_radius(back_btn, btn_h / 2, LV_PART_MAIN);
        lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(back_btn, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(back_btn, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(back_btn, 0, LV_PART_MAIN);
        lv_obj_add_event_cb(back_btn, ui_dashboard_config_error_back, LV_EVENT_CLICKED, NULL);

        lv_obj_t *back_label = lv_label_create(back_btn);
        lv_label_set_text(back_label, "BACK");
        lv_obj_set_style_text_font(back_label, ui_font_typoder(ui_layout_px(18)), LV_PART_MAIN);
        lv_obj_set_style_text_color(back_label, lv_color_hex(0x000000), LV_PART_MAIN);
        lv_obj_center(back_label);
        aux_sensor_demand_refresh();
        return;
    }

    safe_margin = ui_safe_margin();
    top_pad = safe_margin + ui_layout_px(2);
    bottom_pad = safe_margin + ui_layout_px(4);
    title_h = ui_layout_px(20);
    title_gap = ui_layout_px(6);
    title_y = top_pad;
    body_top = title_y + title_h + title_gap;
    body_h = (lv_coord_t)(ui_screen_height() - bottom_pad - body_top);
    if (body_h < 1) {
        body_h = 1;
    }

    body_row_gap = ui_layout_px(2);
    if (body_row_gap < 1) {
        body_row_gap = 1;
    }

    row_h = (lv_coord_t)((body_h - ((total_row_count - 1u) * body_row_gap)) / total_row_count);
    if (row_h < ui_layout_px(18)) {
        row_h = ui_layout_px(18);
    }
    body_used_h = (lv_coord_t)(row_h * total_row_count + ((total_row_count - 1u) * body_row_gap));
    if (body_used_h > body_h) {
        row_h = (lv_coord_t)((body_h - ((total_row_count - 1u) * body_row_gap)) / total_row_count);
        if (row_h < 1) {
            row_h = 1;
        }
        body_used_h = (lv_coord_t)(row_h * total_row_count + ((total_row_count - 1u) * body_row_gap));
    }
    body_top = (lv_coord_t)(body_top + ((body_h - body_used_h) / 2));
    body_h = body_used_h;

    body_inset = safe_margin + ui_layout_px(6);
    ui_dashboard_config_safe_span_for_band(body_top, body_h, body_inset, &body_left, &body_right);
    body_w = (lv_coord_t)(body_right - body_left);
    type_row_h = row_h;
    count_row_h = row_h;
    slot_row_h = row_h;
    count_roller_w = (lv_coord_t)(body_w * 42 / 100);
    type_roller_w = (lv_coord_t)(body_w * 52 / 100);
    item_roller_w = (lv_coord_t)(body_w * 52 / 100);
    if (count_roller_w < 1) {
        count_roller_w = 1;
    }
    if (type_roller_w < 1) {
        type_roller_w = 1;
    }
    if (item_roller_w < 1) {
        item_roller_w = 1;
    }
    roller_font = (row_h >= ui_layout_px(28)) ? 16 : 14;
    label_font = (row_h >= ui_layout_px(26)) ? 12 : 10;
    visible_row_radius = row_h / 2;
    if (visible_row_radius < ui_layout_px(8)) {
        visible_row_radius = ui_layout_px(8);
    }

    lv_obj_t *title = lv_label_create(s_dashboard_config_screen);
    lv_label_set_text(title, "DASHBOARD");
    lv_obj_set_style_text_font(title, ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, title_y);

    s_dashboard_cfg_body = lv_obj_create(s_dashboard_config_screen);
    lv_obj_remove_style_all(s_dashboard_cfg_body);
    lv_obj_set_size(s_dashboard_cfg_body, body_w, body_h);
    lv_obj_align(s_dashboard_cfg_body, LV_ALIGN_TOP_LEFT, body_left, body_top);
    lv_obj_set_layout(s_dashboard_cfg_body, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_dashboard_cfg_body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_dashboard_cfg_body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_dashboard_cfg_body, body_row_gap, LV_PART_MAIN);
    lv_obj_set_style_pad_top(s_dashboard_cfg_body, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(s_dashboard_cfg_body, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(s_dashboard_cfg_body, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_right(s_dashboard_cfg_body, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(s_dashboard_cfg_body, LV_SCROLLBAR_MODE_OFF);

    s_dashboard_cfg_type_row = ui_dashboard_config_create_row(s_dashboard_cfg_body, type_row_h);
    lv_obj_t *type_label = lv_label_create(s_dashboard_cfg_type_row);
    lv_label_set_text(type_label, "TYPE");
    lv_obj_set_style_text_font(type_label, ui_font_hint(label_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(type_label, lv_color_hex(0x7A7A7A), LV_PART_MAIN);
    lv_obj_align(type_label, LV_ALIGN_LEFT_MID, ui_layout_px(6), 0);

    s_dashboard_cfg_type_roller = lv_roller_create(s_dashboard_cfg_type_row);
    lv_roller_set_options(s_dashboard_cfg_type_roller, type_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_dashboard_cfg_type_roller, 1);
    lv_roller_set_selected(s_dashboard_cfg_type_roller, (uint16_t)ui_dashboard_page_get_type(page), LV_ANIM_OFF);
    lv_obj_set_size(s_dashboard_cfg_type_roller, type_roller_w, type_row_h);
    lv_obj_align(s_dashboard_cfg_type_roller, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_clear_flag(s_dashboard_cfg_type_roller, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_text_font(s_dashboard_cfg_type_roller, ui_font_typoder(roller_font), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_dashboard_cfg_type_roller, ui_font_typoder(roller_font), LV_PART_SELECTED);
    ui_round_shell_apply_roller_theme(s_dashboard_cfg_type_roller,
                                      visible_row_radius,
                                      lv_color_hex(0x202020),
                                      lv_color_hex(0x444444),
                                      lv_color_hex(0xCFCFCF),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0x000000));
    lv_obj_add_event_cb(s_dashboard_cfg_type_roller,
                        ui_dashboard_config_type_changed,
                        LV_EVENT_VALUE_CHANGED,
                        NULL);

    s_dashboard_cfg_slot_count_row = ui_dashboard_config_create_row(s_dashboard_cfg_body, count_row_h);
    lv_obj_t *count_label = lv_label_create(s_dashboard_cfg_slot_count_row);
    lv_label_set_text(count_label, "SLOTS");
    lv_obj_set_style_text_font(count_label, ui_font_hint(label_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(count_label, lv_color_hex(0x7A7A7A), LV_PART_MAIN);
    lv_obj_align(count_label, LV_ALIGN_LEFT_MID, ui_layout_px(6), 0);

    s_dashboard_cfg_slot_count_roller = lv_roller_create(s_dashboard_cfg_slot_count_row);
    lv_roller_set_options(s_dashboard_cfg_slot_count_roller, slot_count_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_dashboard_cfg_slot_count_roller, 1);
    lv_roller_set_selected(s_dashboard_cfg_slot_count_roller, (uint16_t)(page->slot_count - 1u), LV_ANIM_OFF);
    lv_obj_set_size(s_dashboard_cfg_slot_count_roller, count_roller_w, count_row_h);
    lv_obj_align(s_dashboard_cfg_slot_count_roller, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_clear_flag(s_dashboard_cfg_slot_count_roller, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_text_font(s_dashboard_cfg_slot_count_roller, ui_font_typoder(roller_font), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_dashboard_cfg_slot_count_roller, ui_font_typoder(roller_font), LV_PART_SELECTED);
    ui_round_shell_apply_roller_theme(s_dashboard_cfg_slot_count_roller,
                                      visible_row_radius,
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
        lv_obj_t *row = ui_dashboard_config_create_row(s_dashboard_cfg_body, slot_row_h);
        s_dashboard_cfg_slot_rows[i] = row;

        lv_obj_t *label = lv_label_create(row);
        lv_label_set_text_fmt(label, "SLOT %u", (unsigned)(i + 1u));
        lv_obj_set_style_text_font(label, ui_font_hint(label_font), LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(0x7A7A7A), LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, ui_layout_px(6), 0);

        s_dashboard_cfg_slot_item_rollers[i] = lv_roller_create(row);
        lv_roller_set_options(s_dashboard_cfg_slot_item_rollers[i], item_options, LV_ROLLER_MODE_NORMAL);
        lv_roller_set_visible_row_count(s_dashboard_cfg_slot_item_rollers[i], 1);
        lv_roller_set_selected(s_dashboard_cfg_slot_item_rollers[i],
                               ui_dashboard_config_selected_index_for_item(
                                   ui_dashboard_item_supported_for_vehicle(nvs_cfg_get()->vehicle_profile_idx,
                                                                           page->slot_items[i] % DISP_ITEM_COUNT)
                                       ? (uint8_t)(page->slot_items[i] % DISP_ITEM_COUNT)
                                       : ui_dashboard_config_default_supported_item()),
                               LV_ANIM_OFF);
        lv_obj_set_size(s_dashboard_cfg_slot_item_rollers[i], item_roller_w, slot_row_h);
        lv_obj_align(s_dashboard_cfg_slot_item_rollers[i], LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_clear_flag(s_dashboard_cfg_slot_item_rollers[i], LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_set_style_text_font(s_dashboard_cfg_slot_item_rollers[i], ui_font_typoder(roller_font), LV_PART_MAIN);
        lv_obj_set_style_text_font(s_dashboard_cfg_slot_item_rollers[i], ui_font_typoder(roller_font), LV_PART_SELECTED);
        ui_round_shell_apply_roller_theme(s_dashboard_cfg_slot_item_rollers[i],
                                          visible_row_radius,
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

    ui_dashboard_config_refresh_rows();
    aux_sensor_demand_refresh();
}

void ui_dashboard_config_open(uint8_t gauge_index)
{
    s_dashboard_config_gauge_index = gauge_index;
    ui_dashboard_config_screen_change_with_anim(&s_dashboard_config_screen,
                                                LV_SCR_LOAD_ANIM_NONE,
                                                ui_dashboard_config_screen_init);
}

void ui_dashboard_config_reset(void)
{
    s_dashboard_config_screen = NULL;
    s_dashboard_config_gauge_index = 0;
    s_dashboard_cfg_body = NULL;
    s_dashboard_cfg_type_row = NULL;
    s_dashboard_cfg_type_roller = NULL;
    s_dashboard_cfg_slot_count_row = NULL;
    s_dashboard_cfg_slot_count_roller = NULL;
    s_dashboard_cfg_supported_item_count = 0;
    memset(s_dashboard_cfg_slot_item_rollers, 0, sizeof(s_dashboard_cfg_slot_item_rollers));
    memset(s_dashboard_cfg_slot_rows, 0, sizeof(s_dashboard_cfg_slot_rows));
}
