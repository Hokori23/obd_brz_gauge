#include "ui_dashboard_config.h"

#include "ui.h"
#include "ui_font_profile.h"
#include "ui_helpers.h"
#include "ui_layout.h"
#include "ui_round_shell.h"
#include "ui_home_runtime.h"
#include "ui_runtime_common.h"

#include <string.h>

#include "bsp_obd_dsp/nvs_storage.h"

static lv_obj_t *s_dashboard_config_screen = NULL;
static uint8_t s_dashboard_config_gauge_index = 0;
static lv_obj_t *s_dashboard_cfg_slot_count_roller = NULL;
static lv_obj_t *s_dashboard_cfg_slot_item_rollers[UI_DASHBOARD_MAX_SLOTS] = {0};
static lv_obj_t *s_dashboard_cfg_slot_rows[UI_DASHBOARD_MAX_SLOTS] = {0};

static void ui_dashboard_config_screen_change_with_anim(lv_obj_t **target_scr,
                                                        lv_scr_load_anim_t anim,
                                                        void (*target_init)(void))
{
    _ui_screen_change(target_scr, anim, 200, 0, target_init);
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

static void ui_dashboard_config_close_to_home(lv_scr_load_anim_t anim)
{
    ui_home_runtime_rebuild_and_load((uint8_t)(s_dashboard_config_gauge_index + 1u), anim);
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

    page = ui_dashboard_config_get_page_cfg(s_dashboard_config_gauge_index);

    s_dashboard_config_screen = lv_obj_create(NULL);

    if (page == NULL) {
        ui_round_screen_apply_base(s_dashboard_config_screen, lv_color_hex(0x000000));
        lv_obj_t *err_label = lv_label_create(s_dashboard_config_screen);
        lv_label_set_text(err_label, "No page config");
        lv_obj_set_style_text_color(err_label, lv_color_hex(0xFF4444), LV_PART_MAIN);
        lv_obj_center(err_label);
        return;
    }
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
    s_dashboard_cfg_slot_count_roller = NULL;
    memset(s_dashboard_cfg_slot_item_rollers, 0, sizeof(s_dashboard_cfg_slot_item_rollers));
    memset(s_dashboard_cfg_slot_rows, 0, sizeof(s_dashboard_cfg_slot_rows));
}
