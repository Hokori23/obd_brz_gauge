#include "ui_dashboard_config.h"

#include "ui.h"
#include "ui_font_profile.h"
#include "ui_helpers.h"
#include "ui_layout.h"
#include "ui_round_shell.h"
#include "ui_home_runtime.h"
#include "ui_runtime_common.h"

#include <stdio.h>
#include <string.h>

#include "app_obd_dsp/aux_sensor_demand.h"
#include "bsp_obd_dsp/nvs_storage.h"

static lv_obj_t *s_dashboard_config_screen = NULL;
static uint8_t s_dashboard_config_gauge_index = 0;
static lv_obj_t *s_dashboard_cfg_type_roller = NULL;
static lv_obj_t *s_dashboard_cfg_type_row = NULL;
static lv_obj_t *s_dashboard_cfg_slot_count_roller = NULL;
static lv_obj_t *s_dashboard_cfg_slot_count_row = NULL;
static lv_obj_t *s_dashboard_cfg_slot_count_label = NULL;
static lv_obj_t *s_dashboard_cfg_slot_edit_roller = NULL;
static lv_obj_t *s_dashboard_cfg_slot_edit_row = NULL;
static lv_obj_t *s_dashboard_cfg_slot_edit_label = NULL;
static lv_obj_t *s_dashboard_cfg_slot_item_roller = NULL;
static lv_obj_t *s_dashboard_cfg_slot_item_row = NULL;
static lv_obj_t *s_dashboard_cfg_slot_item_label = NULL;
static lv_obj_t *s_dashboard_cfg_gear_gap_roller = NULL;
static lv_obj_t *s_dashboard_cfg_gear_gap_row = NULL;
static lv_obj_t *s_dashboard_cfg_gear_gap_label = NULL;
static lv_obj_t *s_dashboard_cfg_rpm_toggle_wrap = NULL;
static lv_obj_t *s_dashboard_cfg_rpm_toggle_off_btn = NULL;
static lv_obj_t *s_dashboard_cfg_rpm_toggle_on_btn = NULL;
static lv_obj_t *s_dashboard_cfg_body = NULL;
static uint8_t s_dashboard_cfg_supported_items[DISP_ITEM_COUNT] = {0};
static uint8_t s_dashboard_cfg_supported_item_count = 0;
static uint8_t s_dashboard_cfg_active_slot_index = 0;
static bool s_dashboard_cfg_refreshing = false;

/** 重建当前车型可用的仪表项列表。 */
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

/** 返回可用仪表项里的默认兜底项。 */
static uint8_t ui_dashboard_config_default_supported_item(void)
{
    for (uint8_t i = 0; i < s_dashboard_cfg_supported_item_count; ++i) {
        if (s_dashboard_cfg_supported_items[i] == DISP_ITEM_RPM) {
            return DISP_ITEM_RPM;
        }
    }

    return s_dashboard_cfg_supported_items[0];
}

/** 把仪表项映射成当前滚轮里的选中索引。 */
static uint16_t ui_dashboard_config_selected_index_for_item(uint8_t item)
{
    for (uint16_t i = 0; i < s_dashboard_cfg_supported_item_count; ++i) {
        if (s_dashboard_cfg_supported_items[i] == item) {
            return i;
        }
    }

    return 0;
}

/** 统一处理仪表页配置界面的页面切换动画。 */
static void ui_dashboard_config_screen_change_with_anim(lv_obj_t **target_scr,
                                                        lv_scr_load_anim_t anim,
                                                        void (*target_init)(void))
{
    _ui_screen_change(target_scr, anim, 200, 0, target_init);
}

/** 计算关闭配置页后应该返回的首页页面 ID。 */
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

/** 读取指定仪表页对应的配置。 */
static const ui_dashboard_page_cfg_t *ui_dashboard_config_get_page_cfg(uint8_t gauge_index)
{
    const ui_dashboard_cfg_t *dashboard = &nvs_cfg_get()->dashboard_cfg;
    if (gauge_index >= dashboard->gauge_page_count || gauge_index >= UI_DASHBOARD_MAX_PAGES) {
        return NULL;
    }
    return &dashboard->pages[gauge_index];
}

/** 读取当前界面上选中的仪表页类型。 */
static ui_dashboard_page_type_t ui_dashboard_config_selected_page_type(void)
{
    ui_dashboard_page_type_t page_type = UI_DASHBOARD_PAGE_TYPE_METRIC;

    if (s_dashboard_cfg_type_roller != NULL) {
        page_type = (ui_dashboard_page_type_t)lv_roller_get_selected(s_dashboard_cfg_type_roller);
        if (page_type >= UI_DASHBOARD_PAGE_TYPE_COUNT) {
            page_type = UI_DASHBOARD_PAGE_TYPE_METRIC;
        }
    }

    return page_type;
}

/** 生成档位页红线或最大转速滚轮的选项字符串。 */
static void ui_dashboard_config_build_gear_rpm_options(char *buf,
                                                       size_t buf_len,
                                                       uint16_t start_rpm)
{
    if (buf == NULL || buf_len == 0u) {
        return;
    }

    buf[0] = '\0';
    for (uint16_t rpm = start_rpm; rpm <= UI_DASHBOARD_GEAR_REDLINE_RPM_MAX; rpm += 100u) {
        char option[8];
        if (buf[0] != '\0') {
            strlcat(buf, "\n", buf_len);
        }
        snprintf(option, sizeof(option), "%u", (unsigned)rpm);
        strlcat(buf, option, buf_len);
    }
}

/** 同步切换按钮的选中状态。 */
static void ui_dashboard_config_set_toggle_checked(lv_obj_t *btn, bool checked)
{
    if (btn == NULL) {
        return;
    }

    if (checked) {
        lv_obj_add_state(btn, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(btn, LV_STATE_CHECKED);
    }
}

/** 按当前开关值同步档位页 RPM ring 的双按钮状态。 */
static void ui_dashboard_config_sync_gear_toggle(bool enabled)
{
    ui_dashboard_config_set_toggle_checked(s_dashboard_cfg_rpm_toggle_off_btn, !enabled);
    ui_dashboard_config_set_toggle_checked(s_dashboard_cfg_rpm_toggle_on_btn, enabled);
}

/** 读取当前界面上选中的档位分段转速步进。 */
static uint16_t ui_dashboard_config_selected_gear_gap_rpm(void)
{
    static const uint16_t s_gap_options[] = {100u, 200u, 500u, 800u, 1000u, 2000u};
    uint16_t idx = 2u;

    if (s_dashboard_cfg_gear_gap_roller != NULL) {
        idx = lv_roller_get_selected(s_dashboard_cfg_gear_gap_roller);
    }
    if (idx >= (sizeof(s_gap_options) / sizeof(s_gap_options[0]))) {
        idx = 2u;
    }

    return s_gap_options[idx];
}

/**
 * 刷新配置页的行布局和当前值
 *
 * 根据页面类型切换不同控件的显隐、文案和滚轮选项。
 */
static void ui_dashboard_config_refresh_rows(void)
{
    char slot_options[32] = {0};
    char redline_options[512] = {0};
    char max_options[512] = {0};
    nvs_user_cfg_t cfg;
    const ui_dashboard_page_cfg_t *page_cfg;
    uint8_t slot_count = 1u;
    ui_dashboard_page_type_t page_type;

    cfg = *nvs_cfg_get();
    if (s_dashboard_config_gauge_index >= cfg.dashboard_cfg.gauge_page_count) {
        return;
    }
    page_cfg = &cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index];
    page_type = ui_dashboard_config_selected_page_type();

    s_dashboard_cfg_refreshing = true;
    if (s_dashboard_cfg_slot_count_roller != NULL &&
        page_type == UI_DASHBOARD_PAGE_TYPE_METRIC) {
        slot_count = (uint8_t)(lv_roller_get_selected(s_dashboard_cfg_slot_count_roller) + 1u);
    }

    if (page_type == UI_DASHBOARD_PAGE_TYPE_METRIC) {
        if (s_dashboard_cfg_slot_count_row != NULL) {
            lv_obj_clear_flag(s_dashboard_cfg_slot_count_row, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_dashboard_cfg_slot_edit_row != NULL) {
            lv_obj_clear_flag(s_dashboard_cfg_slot_edit_row, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_dashboard_cfg_slot_item_row != NULL) {
            lv_obj_clear_flag(s_dashboard_cfg_slot_item_row, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_dashboard_cfg_gear_gap_row != NULL) {
            lv_obj_add_flag(s_dashboard_cfg_gear_gap_row, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_dashboard_cfg_slot_count_label != NULL) {
            lv_label_set_text(s_dashboard_cfg_slot_count_label, "SLOTS");
        }
        if (s_dashboard_cfg_slot_edit_label != NULL) {
            lv_label_set_text(s_dashboard_cfg_slot_edit_label, "EDIT SLOT");
        }
        if (s_dashboard_cfg_slot_item_label != NULL) {
            lv_label_set_text(s_dashboard_cfg_slot_item_label, "METRIC");
        }
        if (s_dashboard_cfg_slot_item_roller != NULL) {
            lv_obj_clear_flag(s_dashboard_cfg_slot_item_roller, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_dashboard_cfg_rpm_toggle_wrap != NULL) {
            lv_obj_add_flag(s_dashboard_cfg_rpm_toggle_wrap, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_dashboard_cfg_slot_count_roller != NULL) {
            lv_obj_set_width(s_dashboard_cfg_slot_count_roller, ui_layout_px(104));
            lv_roller_set_options(s_dashboard_cfg_slot_count_roller, "1\n2\n3\n4\n5\n6", LV_ROLLER_MODE_NORMAL);
            lv_roller_set_selected(s_dashboard_cfg_slot_count_roller,
                                   (uint16_t)(page_cfg->slot_count - 1u),
                                   LV_ANIM_OFF);
        }

        if (slot_count == 0u) {
            slot_count = 1u;
        }
        if (s_dashboard_cfg_active_slot_index >= slot_count) {
            s_dashboard_cfg_active_slot_index = (uint8_t)(slot_count - 1u);
        }

        for (uint8_t i = 0; i < slot_count; ++i) {
            char option[8];
            if (i > 0u) {
                strlcat(slot_options, "\n", sizeof(slot_options));
            }
            snprintf(option, sizeof(option), "%u", (unsigned)(i + 1u));
            strlcat(slot_options, option, sizeof(slot_options));
        }

        if (s_dashboard_cfg_slot_edit_roller != NULL) {
            lv_obj_set_width(s_dashboard_cfg_slot_edit_roller, ui_layout_px(104));
            lv_roller_set_options(s_dashboard_cfg_slot_edit_roller, slot_options, LV_ROLLER_MODE_NORMAL);
            lv_roller_set_selected(s_dashboard_cfg_slot_edit_roller, s_dashboard_cfg_active_slot_index, LV_ANIM_OFF);
        }
        if (s_dashboard_cfg_slot_item_roller != NULL) {
            lv_obj_set_width(s_dashboard_cfg_slot_item_roller, ui_layout_px(152));
            lv_roller_set_selected(s_dashboard_cfg_slot_item_roller,
                                   ui_dashboard_config_selected_index_for_item(
                                       ui_dashboard_item_supported_for_vehicle(cfg.vehicle_profile_idx,
                                                                               page_cfg->slot_items[s_dashboard_cfg_active_slot_index] % DISP_ITEM_COUNT)
                                           ? (uint8_t)(page_cfg->slot_items[s_dashboard_cfg_active_slot_index] % DISP_ITEM_COUNT)
                                           : ui_dashboard_config_default_supported_item()),
                                   LV_ANIM_OFF);
        }
        s_dashboard_cfg_refreshing = false;
        return;
    }

    if (page_type == UI_DASHBOARD_PAGE_TYPE_GEAR) {
        uint16_t redline_rpm = ui_dashboard_page_get_gear_redline_rpm(page_cfg);
        uint16_t max_rpm = ui_dashboard_page_get_gear_max_rpm(page_cfg);
        uint16_t gap_rpm = ui_dashboard_page_get_gear_segment_rpm_step(page_cfg);

        if (s_dashboard_cfg_slot_count_row != NULL) {
            lv_obj_clear_flag(s_dashboard_cfg_slot_count_row, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_dashboard_cfg_slot_edit_row != NULL) {
            lv_obj_clear_flag(s_dashboard_cfg_slot_edit_row, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_dashboard_cfg_slot_item_row != NULL) {
            lv_obj_clear_flag(s_dashboard_cfg_slot_item_row, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_dashboard_cfg_gear_gap_row != NULL) {
            lv_obj_clear_flag(s_dashboard_cfg_gear_gap_row, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_dashboard_cfg_slot_count_label != NULL) {
            lv_label_set_text(s_dashboard_cfg_slot_count_label, "REDLINE");
        }
        if (s_dashboard_cfg_slot_edit_label != NULL) {
            lv_label_set_text(s_dashboard_cfg_slot_edit_label, "MAX RPM");
        }
        if (s_dashboard_cfg_slot_item_label != NULL) {
            lv_label_set_text(s_dashboard_cfg_slot_item_label, "RPM RING");
        }
        if (s_dashboard_cfg_gear_gap_label != NULL) {
            lv_label_set_text(s_dashboard_cfg_gear_gap_label, "GAP");
        }
        if (s_dashboard_cfg_slot_item_roller != NULL) {
            lv_obj_add_flag(s_dashboard_cfg_slot_item_roller, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_dashboard_cfg_rpm_toggle_wrap != NULL) {
            lv_obj_clear_flag(s_dashboard_cfg_rpm_toggle_wrap, LV_OBJ_FLAG_HIDDEN);
        }

        ui_dashboard_config_build_gear_rpm_options(redline_options,
                                                   sizeof(redline_options),
                                                   UI_DASHBOARD_GEAR_REDLINE_RPM_MIN);
        ui_dashboard_config_build_gear_rpm_options(max_options, sizeof(max_options), redline_rpm);
        if (s_dashboard_cfg_slot_count_roller != NULL) {
            lv_obj_set_width(s_dashboard_cfg_slot_count_roller, ui_layout_px(124));
            lv_roller_set_options(s_dashboard_cfg_slot_count_roller, redline_options, LV_ROLLER_MODE_NORMAL);
            lv_roller_set_selected(s_dashboard_cfg_slot_count_roller,
                                   (uint16_t)((redline_rpm - UI_DASHBOARD_GEAR_REDLINE_RPM_MIN) / 100u),
                                   LV_ANIM_OFF);
        }
        if (s_dashboard_cfg_slot_edit_roller != NULL) {
            lv_obj_set_width(s_dashboard_cfg_slot_edit_roller, ui_layout_px(124));
            lv_roller_set_options(s_dashboard_cfg_slot_edit_roller, max_options, LV_ROLLER_MODE_NORMAL);
            lv_roller_set_selected(s_dashboard_cfg_slot_edit_roller,
                                   (uint16_t)((max_rpm - redline_rpm) / 100u),
                                   LV_ANIM_OFF);
        }
        if (s_dashboard_cfg_gear_gap_roller != NULL) {
            lv_obj_set_width(s_dashboard_cfg_gear_gap_roller, ui_layout_px(124));
            lv_roller_set_options(s_dashboard_cfg_gear_gap_roller, "100\n200\n500\n800\n1000\n2000", LV_ROLLER_MODE_NORMAL);
            switch (gap_rpm) {
            case 100u: lv_roller_set_selected(s_dashboard_cfg_gear_gap_roller, 0u, LV_ANIM_OFF); break;
            case 200u: lv_roller_set_selected(s_dashboard_cfg_gear_gap_roller, 1u, LV_ANIM_OFF); break;
            case 500u: lv_roller_set_selected(s_dashboard_cfg_gear_gap_roller, 2u, LV_ANIM_OFF); break;
            case 800u: lv_roller_set_selected(s_dashboard_cfg_gear_gap_roller, 3u, LV_ANIM_OFF); break;
            case 1000u: lv_roller_set_selected(s_dashboard_cfg_gear_gap_roller, 4u, LV_ANIM_OFF); break;
            case 2000u:
            default:
                lv_roller_set_selected(s_dashboard_cfg_gear_gap_roller, 5u, LV_ANIM_OFF);
                break;
            }
        }
        ui_dashboard_config_sync_gear_toggle(ui_dashboard_page_is_gear_rpm_ring_enabled(page_cfg));
        s_dashboard_cfg_refreshing = false;
        return;
    }

    if (s_dashboard_cfg_slot_count_row != NULL) {
        lv_obj_add_flag(s_dashboard_cfg_slot_count_row, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_dashboard_cfg_slot_edit_row != NULL) {
        lv_obj_add_flag(s_dashboard_cfg_slot_edit_row, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_dashboard_cfg_slot_item_row != NULL) {
        lv_obj_add_flag(s_dashboard_cfg_slot_item_row, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_dashboard_cfg_gear_gap_row != NULL) {
        lv_obj_add_flag(s_dashboard_cfg_gear_gap_row, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_dashboard_cfg_rpm_toggle_wrap != NULL) {
        lv_obj_add_flag(s_dashboard_cfg_rpm_toggle_wrap, LV_OBJ_FLAG_HIDDEN);
    }
    s_dashboard_cfg_refreshing = false;
}

/** 处理页面类型滚轮变化。 */
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
    aux_sensor_demand_refresh();
}

/** 处理槽位数量或档位页红线滚轮变化。 */
static void ui_dashboard_config_slot_count_changed(lv_event_t *e)
{
    nvs_user_cfg_t cfg;
    uint8_t slot_count;
    const ui_dashboard_page_cfg_t *current_page;

    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    if (s_dashboard_cfg_refreshing) {
        return;
    }

    cfg = *nvs_cfg_get();
    if (s_dashboard_config_gauge_index >= cfg.dashboard_cfg.gauge_page_count) {
        return;
    }

    if (ui_dashboard_config_selected_page_type() == UI_DASHBOARD_PAGE_TYPE_GEAR) {
        uint16_t redline_rpm =
            (uint16_t)(UI_DASHBOARD_GEAR_REDLINE_RPM_MIN +
                       (lv_roller_get_selected(s_dashboard_cfg_slot_count_roller) * 100u));
        ui_dashboard_page_set_gear_redline_rpm(&cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index],
                                               redline_rpm);
        nvs_cfg_set(&cfg);
        ui_dashboard_config_refresh_rows();
        aux_sensor_demand_refresh();
        return;
    }

    slot_count = (uint8_t)(lv_roller_get_selected(s_dashboard_cfg_slot_count_roller) + 1u);
    current_page = &cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index];
    if (slot_count > current_page->slot_count) {
        uint8_t default_item = ui_dashboard_config_default_supported_item();
        for (uint8_t i = current_page->slot_count; i < slot_count; ++i) {
            cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index].slot_items[i] = default_item;
        }
    }

    cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index].slot_count = slot_count;
    nvs_cfg_set(&cfg);
    ui_dashboard_config_refresh_rows();
    aux_sensor_demand_refresh();
}

/** 处理编辑槽位索引或档位页最大转速滚轮变化。 */
static void ui_dashboard_config_slot_edit_changed(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    if (s_dashboard_cfg_refreshing) {
        return;
    }

    if (ui_dashboard_config_selected_page_type() == UI_DASHBOARD_PAGE_TYPE_GEAR) {
        nvs_user_cfg_t cfg = *nvs_cfg_get();
        ui_dashboard_page_cfg_t *page;
        uint16_t redline_rpm;
        uint16_t max_rpm;

        if (s_dashboard_config_gauge_index >= cfg.dashboard_cfg.gauge_page_count) {
            return;
        }

        page = &cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index];
        redline_rpm = ui_dashboard_page_get_gear_redline_rpm(page);
        max_rpm = (uint16_t)(redline_rpm + (lv_roller_get_selected(s_dashboard_cfg_slot_edit_roller) * 100u));
        ui_dashboard_page_set_gear_max_rpm(page, max_rpm);
        nvs_cfg_set(&cfg);
        ui_dashboard_config_refresh_rows();
        aux_sensor_demand_refresh();
        return;
    }

    s_dashboard_cfg_active_slot_index = (uint8_t)lv_roller_get_selected(s_dashboard_cfg_slot_edit_roller);
    ui_dashboard_config_refresh_rows();
}

/** 处理某个槽位的仪表项选择变化。 */
static void ui_dashboard_config_slot_item_changed(lv_event_t *e)
{
    nvs_user_cfg_t cfg;

    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    if (s_dashboard_cfg_refreshing) {
        return;
    }

    cfg = *nvs_cfg_get();
    if (s_dashboard_config_gauge_index >= cfg.dashboard_cfg.gauge_page_count) {
        return;
    }

    if (s_dashboard_cfg_active_slot_index >= UI_DASHBOARD_MAX_SLOTS) {
        return;
    }

    cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index].slot_items[s_dashboard_cfg_active_slot_index] =
        s_dashboard_cfg_supported_items[lv_roller_get_selected(s_dashboard_cfg_slot_item_roller)];
    nvs_cfg_set(&cfg);
    aux_sensor_demand_refresh();
}

/** 处理档位页分段转速步进变化。 */
static void ui_dashboard_config_gear_gap_changed(lv_event_t *e)
{
    nvs_user_cfg_t cfg;

    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || s_dashboard_cfg_refreshing) {
        return;
    }

    cfg = *nvs_cfg_get();
    if (s_dashboard_config_gauge_index >= cfg.dashboard_cfg.gauge_page_count) {
        return;
    }

    ui_dashboard_page_set_gear_segment_rpm_step(&cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index],
                                                ui_dashboard_config_selected_gear_gap_rpm());
    nvs_cfg_set(&cfg);
    ui_dashboard_config_refresh_rows();
}

/** 处理档位页 RPM ring 开关点击。 */
static void ui_dashboard_config_rpm_toggle_clicked(lv_event_t *e)
{
    nvs_user_cfg_t cfg;
    ui_dashboard_page_cfg_t *page;
    bool enable;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED || s_dashboard_cfg_refreshing) {
        return;
    }

    cfg = *nvs_cfg_get();
    if (s_dashboard_config_gauge_index >= cfg.dashboard_cfg.gauge_page_count) {
        return;
    }

    page = &cfg.dashboard_cfg.pages[s_dashboard_config_gauge_index];
    enable = lv_event_get_user_data(e) == s_dashboard_cfg_rpm_toggle_on_btn;
    ui_dashboard_page_set_gear_rpm_ring_enabled(page, enable);
    nvs_cfg_set(&cfg);
    ui_dashboard_config_refresh_rows();
    aux_sensor_demand_refresh();
}

/** 关闭配置页并回到对应首页页面。 */
static void ui_dashboard_config_close_to_home(lv_scr_load_anim_t anim)
{
    ui_home_runtime_rebuild_and_load(ui_dashboard_config_get_return_page_id(), anim);
}

/** 处理错误兜底页面里的返回按钮。 */
static void ui_dashboard_config_error_back(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    ui_dashboard_config_close_to_home(LV_SCR_LOAD_ANIM_NONE);
}

/** 创建一行统一样式的配置容器。 */
static lv_obj_t *ui_dashboard_config_create_row(lv_obj_t *parent, lv_coord_t height)
{
    lv_obj_t *row = lv_obj_create(parent);

    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, height);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(row, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(row, ui_layout_px(6), LV_PART_MAIN);
    lv_obj_set_style_pad_right(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(row, 0, LV_PART_MAIN);
    return row;
}

/** 处理配置页里的手势返回。 */
static void ui_dashboard_config_gesture_event(lv_event_t *e)
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
    ui_dashboard_config_close_to_home(LV_SCR_LOAD_ANIM_NONE);
}

/** 在配置页销毁时清理全局引用。 */
static void ui_dashboard_config_deleted(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_dashboard_config_reset();
}

/** 为配置页滚轮应用统一主题样式。 */
static void ui_dashboard_config_apply_roller_style(lv_obj_t *roller,
                                                   lv_coord_t radius,
                                                   lv_coord_t min_height,
                                                   lv_coord_t font_size)
{
    ui_round_shell_apply_dark_roller_preset(roller, radius, min_height, font_size);
}

/**
 * 初始化仪表页配置界面
 *
 * 创建完整的配置页面，并按当前仪表页类型装配对应控件。
 */
static void ui_dashboard_config_screen_init(void)
{
    const char *type_options = "METRIC\nGEAR\nG-OBD\nG-ESP32";
    const char *slot_count_options = "1\n2\n3\n4\n5\n6";
    char item_options[128] = {0};
    const ui_dashboard_page_cfg_t *page;
    lv_coord_t title_y;
    lv_coord_t row_h;
    lv_coord_t roller_font;
    lv_coord_t label_font;
    lv_coord_t count_roller_w;
    lv_coord_t type_roller_w;
    lv_coord_t item_roller_w;
    lv_coord_t visible_row_radius;
    lv_coord_t row_w;
    lv_coord_t row_centers[4];
    lv_coord_t gear_gap_center_y;
    ui_settings_layout_t layout;

    if (s_dashboard_config_screen != NULL) {
        return;
    }

    ui_settings_layout_get(&layout);
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
    ui_round_shell_create_ring(s_dashboard_config_screen, &layout.shell);
    lv_obj_add_event_cb(s_dashboard_config_screen,
                        scr_unloaded_delete_cb,
                        LV_EVENT_SCREEN_UNLOADED,
                        &s_dashboard_config_screen);
    lv_obj_add_event_cb(s_dashboard_config_screen, ui_dashboard_config_deleted, LV_EVENT_DELETE, NULL);
    lv_obj_add_event_cb(s_dashboard_config_screen, ui_dashboard_config_gesture_event, LV_EVENT_GESTURE, NULL);

    if (page == NULL) {
        lv_obj_t *title = NULL;
        lv_obj_t *subtitle = NULL;
        lv_obj_t *panel;
        lv_coord_t btn_w;
        lv_coord_t btn_h;
        lv_coord_t panel_w = ui_screen_width() - ((ui_safe_margin() + ui_layout_px(18)) * 2);
        lv_coord_t panel_h = ui_layout_px(124);

        ui_round_shell_create_title_block(s_dashboard_config_screen,
                                          "DASHBOARD",
                                          "Page config is unavailable",
                                          ui_safe_margin() + ui_layout_px(8),
                                          20,
                                          ui_layout_px(4),
                                          11,
                                          &title,
                                          &subtitle);

        if (panel_w < ui_layout_px(220)) {
            panel_w = ui_layout_px(220);
        }

        panel = lv_obj_create(s_dashboard_config_screen);
        lv_obj_set_size(panel, panel_w, panel_h);
        lv_obj_align_to(panel, subtitle, LV_ALIGN_OUT_BOTTOM_MID, 0, ui_layout_px(14));
        ui_round_shell_apply_dark_card_theme(panel, ui_layout_px(20), ui_layout_px(16));
        lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(panel, lv_color_hex(0x3A2323), LV_PART_MAIN);
        lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(panel, ui_layout_px(10), LV_PART_MAIN);

        lv_obj_t *err_label = lv_label_create(panel);
        lv_label_set_text(err_label, "No page config");
        lv_obj_set_style_text_font(err_label, ui_font_typoder(20), LV_PART_MAIN);
        lv_obj_set_style_text_color(err_label, lv_color_hex(0xFF6B6B), LV_PART_MAIN);
        lv_obj_set_style_text_align(err_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

        lv_obj_t *status_label = lv_label_create(panel);
        lv_label_set_text(status_label, "Open from a valid gauge page.");
        lv_obj_set_style_text_font(status_label, ui_font_hint(12), LV_PART_MAIN);
        lv_obj_set_style_text_color(status_label, lv_color_hex(0xBDBDBD), LV_PART_MAIN);
        lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

        btn_w = (lv_coord_t)(ui_screen_width() - ((ui_safe_margin() + ui_layout_px(20)) * 2));
        btn_h = ui_layout_px(38);
        if (btn_w < ui_layout_px(96)) {
            btn_w = ui_layout_px(96);
        }

        lv_obj_t *back_btn = lv_btn_create(s_dashboard_config_screen);
        lv_obj_set_size(back_btn, btn_w, btn_h);
        lv_obj_align_to(back_btn, panel, LV_ALIGN_OUT_BOTTOM_MID, 0, ui_layout_px(16));
        ui_round_shell_apply_toggle_button_theme(back_btn, lv_color_hex(0x2F80ED));
        lv_obj_set_style_radius(back_btn, btn_h / 2, LV_PART_MAIN);
        lv_obj_add_event_cb(back_btn, ui_dashboard_config_error_back, LV_EVENT_CLICKED, NULL);

        lv_obj_t *back_label = lv_label_create(back_btn);
        lv_label_set_text(back_label, "BACK");
        lv_obj_set_style_text_font(back_label, ui_font_typoder(18), LV_PART_MAIN);
        lv_obj_center(back_label);
        aux_sensor_demand_refresh();
        return;
    }

    title_y = ui_safe_margin() + ui_layout_px(10);
    row_h = ui_layout_px(44);
    row_w = ui_layout_px(248);
    count_roller_w = ui_layout_px(104);
    type_roller_w = ui_layout_px(134);
    item_roller_w = ui_layout_px(152);
    roller_font = 15;
    label_font = 12;
    visible_row_radius = ui_layout_px(18);
    row_centers[0] = ui_layout_px(-76);
    row_centers[1] = ui_layout_px(-18);
    row_centers[2] = ui_layout_px(40);
    row_centers[3] = ui_layout_px(98);
    gear_gap_center_y = ui_layout_px(146);

    lv_obj_t *title = ui_round_shell_create_page_title(s_dashboard_config_screen,
                                                       "DASHBOARD",
                                                       title_y,
                                                       22,
                                                       lv_color_hex(0xFFFFFF));
    LV_UNUSED(title);

    s_dashboard_cfg_body = NULL;

    s_dashboard_cfg_type_row = ui_dashboard_config_create_row(s_dashboard_config_screen, row_h);
    lv_obj_set_width(s_dashboard_cfg_type_row, row_w);
    lv_obj_align(s_dashboard_cfg_type_row, LV_ALIGN_CENTER, 0, row_centers[0]);
    lv_obj_t *type_label = lv_label_create(s_dashboard_cfg_type_row);
    lv_label_set_text(type_label, "TYPE");
    lv_obj_set_style_text_font(type_label, ui_font_hint(label_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(type_label, lv_color_hex(0x7A7A7A), LV_PART_MAIN);
    lv_obj_align(type_label, LV_ALIGN_LEFT_MID, ui_layout_px(6), 0);

    s_dashboard_cfg_type_roller = lv_roller_create(s_dashboard_cfg_type_row);
    lv_roller_set_options(s_dashboard_cfg_type_roller, type_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_dashboard_cfg_type_roller, 1);
    lv_roller_set_selected(s_dashboard_cfg_type_roller, (uint16_t)ui_dashboard_page_get_type(page), LV_ANIM_OFF);
    lv_obj_set_size(s_dashboard_cfg_type_roller, type_roller_w, row_h);
    lv_obj_align(s_dashboard_cfg_type_roller, LV_ALIGN_RIGHT_MID, 0, 0);
    ui_dashboard_config_apply_roller_style(s_dashboard_cfg_type_roller,
                                           visible_row_radius,
                                           row_h,
                                           roller_font);
    lv_obj_add_event_cb(s_dashboard_cfg_type_roller,
                        ui_dashboard_config_type_changed,
                        LV_EVENT_VALUE_CHANGED,
                        NULL);

    s_dashboard_cfg_slot_count_row = ui_dashboard_config_create_row(s_dashboard_config_screen, row_h);
    lv_obj_set_width(s_dashboard_cfg_slot_count_row, row_w);
    lv_obj_align(s_dashboard_cfg_slot_count_row, LV_ALIGN_CENTER, 0, row_centers[1]);
    s_dashboard_cfg_slot_count_label = lv_label_create(s_dashboard_cfg_slot_count_row);
    lv_obj_t *count_label = s_dashboard_cfg_slot_count_label;
    lv_label_set_text(count_label, "SLOTS");
    lv_obj_set_style_text_font(count_label, ui_font_hint(label_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(count_label, lv_color_hex(0x7A7A7A), LV_PART_MAIN);
    lv_obj_align(count_label, LV_ALIGN_LEFT_MID, ui_layout_px(6), 0);

    s_dashboard_cfg_slot_count_roller = lv_roller_create(s_dashboard_cfg_slot_count_row);
    lv_roller_set_options(s_dashboard_cfg_slot_count_roller, slot_count_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_dashboard_cfg_slot_count_roller, 1);
    lv_roller_set_selected(s_dashboard_cfg_slot_count_roller, (uint16_t)(page->slot_count - 1u), LV_ANIM_OFF);
    lv_obj_set_size(s_dashboard_cfg_slot_count_roller, count_roller_w, row_h);
    lv_obj_align(s_dashboard_cfg_slot_count_roller, LV_ALIGN_RIGHT_MID, 0, 0);
    ui_dashboard_config_apply_roller_style(s_dashboard_cfg_slot_count_roller,
                                           visible_row_radius,
                                           row_h,
                                           roller_font);
    lv_obj_add_event_cb(s_dashboard_cfg_slot_count_roller,
                        ui_dashboard_config_slot_count_changed,
                        LV_EVENT_VALUE_CHANGED,
                        NULL);

    s_dashboard_cfg_slot_edit_row = ui_dashboard_config_create_row(s_dashboard_config_screen, row_h);
    lv_obj_set_width(s_dashboard_cfg_slot_edit_row, row_w);
    lv_obj_align(s_dashboard_cfg_slot_edit_row, LV_ALIGN_CENTER, 0, row_centers[2]);
    s_dashboard_cfg_slot_edit_label = lv_label_create(s_dashboard_cfg_slot_edit_row);
    lv_label_set_text(s_dashboard_cfg_slot_edit_label, "EDIT SLOT");
    lv_obj_set_style_text_font(s_dashboard_cfg_slot_edit_label, ui_font_hint(label_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_dashboard_cfg_slot_edit_label, lv_color_hex(0x7A7A7A), LV_PART_MAIN);
    lv_obj_align(s_dashboard_cfg_slot_edit_label, LV_ALIGN_LEFT_MID, ui_layout_px(6), 0);

    s_dashboard_cfg_slot_edit_roller = lv_roller_create(s_dashboard_cfg_slot_edit_row);
    lv_roller_set_options(s_dashboard_cfg_slot_edit_roller, "1", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_dashboard_cfg_slot_edit_roller, 1);
    lv_obj_set_size(s_dashboard_cfg_slot_edit_roller, count_roller_w, row_h);
    lv_obj_align(s_dashboard_cfg_slot_edit_roller, LV_ALIGN_RIGHT_MID, 0, 0);
    ui_dashboard_config_apply_roller_style(s_dashboard_cfg_slot_edit_roller,
                                           visible_row_radius,
                                           row_h,
                                           roller_font);
    lv_obj_add_event_cb(s_dashboard_cfg_slot_edit_roller,
                        ui_dashboard_config_slot_edit_changed,
                        LV_EVENT_VALUE_CHANGED,
                        NULL);

    s_dashboard_cfg_slot_item_row = ui_dashboard_config_create_row(s_dashboard_config_screen, row_h);
    lv_obj_set_width(s_dashboard_cfg_slot_item_row, row_w);
    lv_obj_align(s_dashboard_cfg_slot_item_row, LV_ALIGN_CENTER, 0, row_centers[3]);
    s_dashboard_cfg_slot_item_label = lv_label_create(s_dashboard_cfg_slot_item_row);
    lv_label_set_text(s_dashboard_cfg_slot_item_label, "METRIC");
    lv_obj_set_style_text_font(s_dashboard_cfg_slot_item_label, ui_font_hint(label_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_dashboard_cfg_slot_item_label, lv_color_hex(0x7A7A7A), LV_PART_MAIN);
    lv_obj_align(s_dashboard_cfg_slot_item_label, LV_ALIGN_LEFT_MID, ui_layout_px(6), 0);

    s_dashboard_cfg_slot_item_roller = lv_roller_create(s_dashboard_cfg_slot_item_row);
    lv_roller_set_options(s_dashboard_cfg_slot_item_roller, item_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_dashboard_cfg_slot_item_roller, 1);
    lv_obj_set_size(s_dashboard_cfg_slot_item_roller, item_roller_w, row_h);
    lv_obj_align(s_dashboard_cfg_slot_item_roller, LV_ALIGN_RIGHT_MID, 0, 0);
    ui_dashboard_config_apply_roller_style(s_dashboard_cfg_slot_item_roller,
                                           visible_row_radius,
                                           row_h,
                                           (roller_font > 14) ? (roller_font - 1) : roller_font);
    lv_obj_add_event_cb(s_dashboard_cfg_slot_item_roller,
                        ui_dashboard_config_slot_item_changed,
                        LV_EVENT_VALUE_CHANGED,
                        NULL);

    s_dashboard_cfg_rpm_toggle_wrap = lv_obj_create(s_dashboard_cfg_slot_item_row);
    lv_obj_set_size(s_dashboard_cfg_rpm_toggle_wrap, ui_layout_px(152), row_h);
    lv_obj_align(s_dashboard_cfg_rpm_toggle_wrap, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(s_dashboard_cfg_rpm_toggle_wrap, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_dashboard_cfg_rpm_toggle_wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_dashboard_cfg_rpm_toggle_wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_dashboard_cfg_rpm_toggle_wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(s_dashboard_cfg_rpm_toggle_wrap, ui_layout_px(8), LV_PART_MAIN);
    lv_obj_set_layout(s_dashboard_cfg_rpm_toggle_wrap, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_dashboard_cfg_rpm_toggle_wrap, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_dashboard_cfg_rpm_toggle_wrap,
                          LV_FLEX_ALIGN_END,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(s_dashboard_cfg_rpm_toggle_wrap, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_dashboard_cfg_rpm_toggle_wrap, LV_OBJ_FLAG_SCROLLABLE);

    s_dashboard_cfg_rpm_toggle_off_btn = lv_btn_create(s_dashboard_cfg_rpm_toggle_wrap);
    lv_obj_set_size(s_dashboard_cfg_rpm_toggle_off_btn, ui_layout_px(72), row_h);
    lv_obj_add_flag(s_dashboard_cfg_rpm_toggle_off_btn, LV_OBJ_FLAG_CHECKABLE);
    ui_round_shell_apply_toggle_button_theme(s_dashboard_cfg_rpm_toggle_off_btn, lv_color_hex(0x7A7A7A));
    lv_obj_add_event_cb(s_dashboard_cfg_rpm_toggle_off_btn,
                        ui_dashboard_config_rpm_toggle_clicked,
                        LV_EVENT_CLICKED,
                        s_dashboard_cfg_rpm_toggle_off_btn);
    lv_obj_t *rpm_off_label = lv_label_create(s_dashboard_cfg_rpm_toggle_off_btn);
    lv_label_set_text(rpm_off_label, "OFF");
    lv_obj_center(rpm_off_label);

    s_dashboard_cfg_rpm_toggle_on_btn = lv_btn_create(s_dashboard_cfg_rpm_toggle_wrap);
    lv_obj_set_size(s_dashboard_cfg_rpm_toggle_on_btn, ui_layout_px(72), row_h);
    lv_obj_add_flag(s_dashboard_cfg_rpm_toggle_on_btn, LV_OBJ_FLAG_CHECKABLE);
    ui_round_shell_apply_toggle_button_theme(s_dashboard_cfg_rpm_toggle_on_btn, lv_color_hex(0x2F80ED));
    lv_obj_add_event_cb(s_dashboard_cfg_rpm_toggle_on_btn,
                        ui_dashboard_config_rpm_toggle_clicked,
                        LV_EVENT_CLICKED,
                        s_dashboard_cfg_rpm_toggle_on_btn);
    lv_obj_t *rpm_on_label = lv_label_create(s_dashboard_cfg_rpm_toggle_on_btn);
    lv_label_set_text(rpm_on_label, "ON");
    lv_obj_center(rpm_on_label);

    s_dashboard_cfg_gear_gap_row = ui_dashboard_config_create_row(s_dashboard_config_screen, row_h);
    lv_obj_set_width(s_dashboard_cfg_gear_gap_row, row_w);
    lv_obj_align(s_dashboard_cfg_gear_gap_row, LV_ALIGN_CENTER, 0, gear_gap_center_y);
    lv_obj_add_flag(s_dashboard_cfg_gear_gap_row, LV_OBJ_FLAG_HIDDEN);

    s_dashboard_cfg_gear_gap_label = lv_label_create(s_dashboard_cfg_gear_gap_row);
    lv_label_set_text(s_dashboard_cfg_gear_gap_label, "GAP");
    lv_obj_set_style_text_font(s_dashboard_cfg_gear_gap_label, ui_font_hint(label_font), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_dashboard_cfg_gear_gap_label, lv_color_hex(0x7A7A7A), LV_PART_MAIN);
    lv_obj_align(s_dashboard_cfg_gear_gap_label, LV_ALIGN_LEFT_MID, ui_layout_px(6), 0);

    s_dashboard_cfg_gear_gap_roller = lv_roller_create(s_dashboard_cfg_gear_gap_row);
    lv_roller_set_options(s_dashboard_cfg_gear_gap_roller, "100\n200\n500\n800\n1000\n2000", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_dashboard_cfg_gear_gap_roller, 1);
    lv_obj_set_size(s_dashboard_cfg_gear_gap_roller, ui_layout_px(124), row_h);
    lv_obj_align(s_dashboard_cfg_gear_gap_roller, LV_ALIGN_RIGHT_MID, 0, 0);
    ui_dashboard_config_apply_roller_style(s_dashboard_cfg_gear_gap_roller,
                                           visible_row_radius,
                                           row_h,
                                           roller_font);
    lv_obj_add_event_cb(s_dashboard_cfg_gear_gap_roller,
                        ui_dashboard_config_gear_gap_changed,
                        LV_EVENT_VALUE_CHANGED,
                        NULL);

    s_dashboard_cfg_active_slot_index = 0u;
    ui_dashboard_config_refresh_rows();
    aux_sensor_demand_refresh();
}

/** 打开指定仪表页的配置界面。 */
void ui_dashboard_config_open(uint8_t gauge_index)
{
    s_dashboard_config_gauge_index = gauge_index;
    ui_dashboard_config_screen_change_with_anim(&s_dashboard_config_screen,
                                                LV_SCR_LOAD_ANIM_NONE,
                                                ui_dashboard_config_screen_init);
}

/** 重置仪表页配置界面的静态状态缓存。 */
void ui_dashboard_config_reset(void)
{
    s_dashboard_config_screen = NULL;
    s_dashboard_config_gauge_index = 0;
    s_dashboard_cfg_body = NULL;
    s_dashboard_cfg_type_row = NULL;
    s_dashboard_cfg_type_roller = NULL;
    s_dashboard_cfg_slot_count_row = NULL;
    s_dashboard_cfg_slot_count_roller = NULL;
    s_dashboard_cfg_slot_count_label = NULL;
    s_dashboard_cfg_slot_edit_row = NULL;
    s_dashboard_cfg_slot_edit_roller = NULL;
    s_dashboard_cfg_slot_edit_label = NULL;
    s_dashboard_cfg_slot_item_row = NULL;
    s_dashboard_cfg_slot_item_roller = NULL;
    s_dashboard_cfg_slot_item_label = NULL;
    s_dashboard_cfg_gear_gap_roller = NULL;
    s_dashboard_cfg_gear_gap_row = NULL;
    s_dashboard_cfg_gear_gap_label = NULL;
    s_dashboard_cfg_rpm_toggle_wrap = NULL;
    s_dashboard_cfg_rpm_toggle_off_btn = NULL;
    s_dashboard_cfg_rpm_toggle_on_btn = NULL;
    s_dashboard_cfg_supported_item_count = 0;
    s_dashboard_cfg_active_slot_index = 0;
    s_dashboard_cfg_refreshing = false;
}
