#include "ui_home_runtime.h"

#include "ui_dashboard_config.h"
#include "ui.h"
#include "ui_font_profile.h"
#include "ui_helpers.h"
#include "ui_layout.h"
#include "ui_home_runtime_widgets.h"
#include "ui_home_pager.h"
#include "ui_round_shell.h"
#include "ui_runtime_common.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "app_obd_dsp/aux_sensor_demand.h"
#include "app_obd_dsp/lvgl_perf_trace.h"
#include "app_obd_dsp/obd_data_cache.h"
#include "app_obd_dsp/default_page_ids.h"
#include "app_obd_dsp/vehicle_profiles.h"
#include "bsp_obd_dsp/elm327_ble_client.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "esp_log.h"
#include "esp_timer.h"

#define UI_NAV_ANIM_MS 0
#define UI_HOME_GFORCE_HISTORY_BINS 48
#define UI_HOME_GFORCE_HISTORY_POINTS (UI_HOME_GFORCE_HISTORY_BINS + 1)
#define UI_HOME_PI_F 3.14159265f
#define UI_HOME_GFORCE_MAX_G 1.20f
#define UI_HOME_REFRESH_PERIOD_MENU_MS 400u
#define UI_HOME_REFRESH_PERIOD_ADD_MS 400u
#define UI_HOME_REFRESH_PERIOD_METRIC_MS 200u
#define UI_HOME_REFRESH_PERIOD_GEAR_MS 120u
#define UI_HOME_REFRESH_PERIOD_GFORCE_MS 120u
#define UI_HOME_GEAR_RING_MIN_SEGMENT_RPM 100u
#define UI_HOME_GEAR_RING_MAX_SEGMENTS 99u
#define UI_HOME_GEAR_RING_GAP_DEG 3.0f
#define UI_HOME_GEAR_BLINK_PERIOD_US 250000LL

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
    lv_obj_t *custom_status_label;
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
    lv_obj_t *gear_ring_draw_obj;
    lv_coord_t gear_ring_diameter;
    lv_coord_t gear_ring_width;
    uint16_t gear_ring_rpm;
    uint16_t gear_ring_redline_rpm;
    uint16_t gear_ring_max_rpm;
    uint16_t gear_ring_segment_rpm;
    bool gear_ring_enabled;
    bool gear_ring_blink_red;
    int64_t gear_status_wait_start_us;
    uint8_t item_cache[UI_DASHBOARD_MAX_SLOTS];
    uint8_t slot_count;
    lv_obj_t *menu_vehicle_label;
    lv_obj_t *menu_ble_label;
    lv_obj_t *menu_settings_btn;
} ui_home_tile_runtime_t;

typedef struct {
    bool active;
    bool target_committed;
    uint8_t from_page;
    uint8_t to_page;
    int64_t start_us;
    int64_t commit_us;
    int64_t mount_us;
    int64_t refresh_us;
    uint32_t settle_ms;
    lv_timer_t *settle_timer;
} ui_home_nav_perf_trace_t;

lv_obj_t *ui_ScreenPageHome = NULL;

static ui_home_pager_t s_home_pager;
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
static ui_home_nav_perf_trace_t s_home_nav_perf = {0};

static void ui_home_load(lv_scr_load_anim_t anim, uint8_t page_id);
static void ui_home_add_page(void);
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
static uint8_t ui_home_collect_visible_items(const ui_dashboard_page_cfg_t *page,
                                             disp_item_t items[UI_DASHBOARD_MAX_SLOTS]);
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
static uint32_t ui_home_refresh_period_ms_for_page(uint8_t page_id);
static void ui_home_refresh_timer_apply_profile(uint8_t page_id);
static void ui_home_runtime_refresh_tile(uint8_t tile_id);
static void ui_home_runtime_refresh_visible_tiles(uint8_t active_page);
static void ui_home_pager_drag_begin_handler(uint8_t from_page, void *user);
static void ui_home_pager_commit_handler(const ui_home_pager_commit_t *commit, void *user);
static void ui_home_pager_vertical_handler(lv_dir_t dir, uint8_t active_page, void *user);
static void ui_home_page_switch_perf_finish(lv_timer_t *timer);
static void ui_home_page_switch_perf_schedule_finish(uint32_t delay_ms);
static void ui_home_page_switch_perf_begin(uint8_t from_page);
static void ui_home_page_switch_perf_commit(uint8_t to_page);
static void ui_home_page_switch_perf_cancel(void);
static void ui_home_refresh_timer_suspend(void);
static void ui_home_refresh_timer_resume(void);
static ui_dashboard_page_type_t ui_home_page_type_for_gauge(uint8_t gauge_index);
static bool ui_home_read_disp_item_value(disp_item_t item, int32_t *out);
static void ui_home_refresh_menu_tile(ui_home_tile_runtime_t *rt,
                                      const char *vehicle_name,
                                      const char *ble_short);
static void ui_home_gear_ring_draw_event(lv_event_t *e);
static void ui_home_update_gear_ring(ui_home_tile_runtime_t *rt,
                                     uint16_t rpm,
                                     uint16_t redline_rpm,
                                     uint16_t max_rpm,
                                     uint16_t segment_rpm,
                                     bool enabled,
                                     bool blink_red);
static void ui_home_refresh_gear_tile(ui_home_tile_runtime_t *rt);
static void ui_home_refresh_gforce_tile(ui_home_tile_runtime_t *rt,
                                        ui_dashboard_page_type_t page_type);
static void ui_home_refresh_metric_tile(ui_home_tile_runtime_t *rt,
                                        const ui_dashboard_page_cfg_t *page,
                                        float sweep_ratio,
                                        int16_t brake_warn_x10,
                                        int16_t oil_warn_x10);
static float ui_home_clampf(float value, float min_value, float max_value);
static void ui_home_gforce_update_plot(ui_home_tile_runtime_t *rt, float lat_g, float lon_g);
static void ui_home_make_passive_obj(lv_obj_t *obj);

/** 统一处理首页相关页面的切换动画。 */
static void ui_home_screen_change_with_anim(lv_obj_t **target_scr,
                                            lv_scr_load_anim_t anim,
                                            void (*target_init)(void))
{
    _ui_screen_change(target_scr, anim, UI_NAV_ANIM_MS, 0, target_init);
}

/** 从首页沿纵向导航到其他整页界面。 */
static void ui_home_nav_vertical(lv_scr_load_anim_t anim, lv_obj_t **target_scr, void (*target_init)(void))
{
    ui_home_screen_change_with_anim(target_scr, anim, target_init);
}

/** 把配置里的默认页索引转换成首页运行时页面 ID。 */
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

/** 为新建仪表页填充默认配置。 */
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
    ui_dashboard_page_set_gear_redline_rpm(page, UI_DASHBOARD_GEAR_REDLINE_RPM_DEFAULT);
    ui_dashboard_page_set_gear_max_rpm(page, UI_DASHBOARD_GEAR_MAX_RPM_DEFAULT);
    ui_dashboard_page_set_gear_rpm_ring_enabled(page, true);
}

/** 读取指定仪表页的配置。 */
static const ui_dashboard_page_cfg_t *ui_home_get_gauge_cfg(uint8_t gauge_index)
{
    const ui_dashboard_cfg_t *dashboard = &nvs_cfg_get()->dashboard_cfg;
    if (gauge_index >= dashboard->gauge_page_count || gauge_index >= UI_DASHBOARD_MAX_PAGES) {
        return NULL;
    }
    return &dashboard->pages[gauge_index];
}

/** 返回指定仪表页当前采用的页面类型。 */
static ui_dashboard_page_type_t ui_home_page_type_for_gauge(uint8_t gauge_index)
{
    const ui_dashboard_page_cfg_t *page = ui_home_get_gauge_cfg(gauge_index);

    return ui_dashboard_page_get_type(page);
}

/**
 * 读取某个仪表项的当前值
 *
 * 同时在这里统一处理无效值过滤，避免界面层到处重复判空。
 */
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

/** 按当前配置重建首页分页描述表。 */
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

/** 取消当前页面切换性能采样。 */
static void ui_home_page_switch_perf_cancel(void)
{
    if (s_home_nav_perf.settle_timer != NULL) {
        lv_timer_del(s_home_nav_perf.settle_timer);
    }
    memset(&s_home_nav_perf, 0, sizeof(s_home_nav_perf));
    ui_home_refresh_timer_resume();
    app_lvgl_perf_trace_cancel();
}

/** 从拖拽开始时启动一次页面切换性能采样。 */
static void ui_home_page_switch_perf_begin(uint8_t from_page)
{
    ui_home_page_switch_perf_cancel();
    ui_home_refresh_timer_suspend();
    s_home_nav_perf.active = true;
    s_home_nav_perf.from_page = from_page;
    s_home_nav_perf.to_page = from_page;
    s_home_nav_perf.start_us = esp_timer_get_time();
    app_lvgl_perf_trace_begin("home_swipe", from_page, from_page);
}

/** 为性能采样安排一次延迟收尾。 */
static void ui_home_page_switch_perf_schedule_finish(uint32_t delay_ms)
{
    if (delay_ms == 0u) {
        delay_ms = 1u;
    }

    if (s_home_nav_perf.settle_timer != NULL) {
        lv_timer_del(s_home_nav_perf.settle_timer);
        s_home_nav_perf.settle_timer = NULL;
    }

    s_home_nav_perf.settle_timer = lv_timer_create(ui_home_page_switch_perf_finish, delay_ms, NULL);
    if (s_home_nav_perf.settle_timer == NULL) {
        ui_home_page_switch_perf_finish(NULL);
        return;
    }

    lv_timer_set_repeat_count(s_home_nav_perf.settle_timer, 1);
}

/** 在目标页确定后提交页面切换性能采样。 */
static void ui_home_page_switch_perf_commit(uint8_t to_page)
{
    uint32_t refresh_period_ms;

    if (!s_home_nav_perf.active) {
        ui_home_page_switch_perf_begin(s_home_active_page);
    }

    s_home_nav_perf.target_committed = true;
    s_home_nav_perf.to_page = to_page;
    s_home_nav_perf.commit_us = esp_timer_get_time() - s_home_nav_perf.start_us;
    refresh_period_ms = ui_home_refresh_period_ms_for_page(to_page);
    if (refresh_period_ms < 120u) {
        refresh_period_ms = 120u;
    } else if (refresh_period_ms > 320u) {
        refresh_period_ms = 320u;
    }
    s_home_nav_perf.settle_ms = (uint32_t)(refresh_period_ms + 80u);
    app_lvgl_perf_trace_update_target(to_page);
    ui_home_page_switch_perf_schedule_finish(s_home_nav_perf.settle_ms);
}

/** 结束一次页面切换性能采样并输出统计日志。 */
static void ui_home_page_switch_perf_finish(lv_timer_t *timer)
{
    app_lvgl_perf_trace_stats_t stats = {0};

    LV_UNUSED(timer);
    if (!s_home_nav_perf.active || !s_home_nav_perf.target_committed) {
        ui_home_page_switch_perf_cancel();
        return;
    }

    if (app_lvgl_perf_trace_end(&stats)) {
        ESP_LOGI("ui_home_perf",
                 "nav %" PRIu8 "->%" PRIu8 " total=%" PRIu32 "ms commit=%" PRIi64
                 "ms mount=%" PRIi64 "ms refresh=%" PRIi64 "ms settle=%" PRIu32
                 "ms flushes=%" PRIu32 " px=%" PRIu32 " avg_px=%" PRIu32
                 " max_rect=%" PRIu32 "x%" PRIu32
                 " fw=%" PRIu32 " line=%" PRIu32 " tail=%" PRIu32 " rounds=%" PRIu32
                 " submit_us(avg/max)=%" PRIi64
                 "/%" PRIi64 " done_us(avg/max)=%" PRIi64 "/%" PRIi64
                 " rot=%u lines=%" PRIu32,
                 stats.from_page,
                 stats.to_page,
                 stats.duration_ms,
                 s_home_nav_perf.commit_us / 1000LL,
                 s_home_nav_perf.mount_us / 1000LL,
                 s_home_nav_perf.refresh_us / 1000LL,
                 s_home_nav_perf.settle_ms,
                 stats.flushes,
                 stats.pixels,
                 stats.avg_pixels,
                 stats.max_width,
                 stats.max_height,
                 stats.full_width_flushes,
                 stats.line_band_flushes,
                 stats.tail_band_flushes,
                 stats.rounds_started,
                 stats.avg_submit_us,
                 stats.max_submit_us,
                 stats.avg_done_us,
                 stats.max_done_us,
                 stats.rotation,
                 stats.buffer_lines);
    }

    ui_home_refresh_timer_resume();
    memset(&s_home_nav_perf, 0, sizeof(s_home_nav_perf));
}

/** 重置某个首页 tile 的运行时缓存。 */
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

/** 清理某个 tile 的视觉状态缓存。 */
static void ui_home_reset_tile_effect_cache(uint8_t page_id)
{
    if (page_id >= UI_HOME_MAX_TILE_COUNT) {
        return;
    }

    if (s_home_content_roots[page_id] != NULL) {
        lv_obj_clear_flag(s_home_content_roots[page_id], LV_OBJ_FLAG_HIDDEN);
    }
}

/** 重置首页界面的整体运行时状态。 */
static void ui_home_reset_screen_state(void)
{
    ui_home_page_switch_perf_cancel();
    memset(s_home_tiles, 0, sizeof(s_home_tiles));
    memset(s_home_content_roots, 0, sizeof(s_home_content_roots));
    memset(s_home_tile_mounted, 0, sizeof(s_home_tile_mounted));
    for (uint8_t i = 0; i < UI_HOME_MAX_TILE_COUNT; ++i) {
        ui_home_reset_runtime(i);
        ui_home_reset_tile_effect_cache(i);
    }
}

/** 把首页页面 ID 映射回仪表页索引。 */
static int8_t ui_home_page_to_gauge_index(uint8_t page_id)
{
    if (page_id >= s_home_tile_count || s_home_tile_descs[page_id].kind != UI_HOME_TILE_GAUGE) {
        return -1;
    }

    return s_home_tile_descs[page_id].gauge_index;
}

/**
 * 切换当前激活的首页页面
 *
 * 同步完成挂载、刷新频率和传感器需求的切换。
 */
static void ui_home_set_active_page(uint8_t page_id, lv_anim_enable_t anim_en)
{
    if (page_id >= s_home_tile_count) {
        page_id = UI_HOME_PAGE_MENU_ID;
    }

    if (ui_home_pager_root(&s_home_pager) != NULL &&
        s_home_active_page == page_id &&
        ui_home_pager_active_page(&s_home_pager) == page_id) {
        return;
    }

    s_home_active_page = page_id;
    ui_home_sync_tile_mounts(page_id);
    ui_home_refresh_timer_apply_profile(page_id);
    if (ui_home_pager_root(&s_home_pager) != NULL) {
        ui_home_pager_set_active_page(&s_home_pager, page_id, anim_en != LV_ANIM_OFF);
    }
    aux_sensor_demand_refresh();
}

/** 为单个首页 tile 创建内容根容器。 */
static lv_obj_t *ui_home_create_content_root(lv_obj_t *tile)
{
    lv_obj_t *root = lv_obj_create(tile);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_center(root);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(root, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_update_layout(root);
    lv_obj_set_style_transform_pivot_x(root, lv_obj_get_width(root) / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_y(root, lv_obj_get_height(root) / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_translate_x(root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_translate_y(root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_opa(root, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_zoom(root, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    return root;
}

/** 收集某个仪表页当前真正可显示的仪表项。 */
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

/** 处理首页菜单里的设置入口点击。 */
static void ui_home_menu_open_settings(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ui_home_nav_vertical(LV_SCR_LOAD_ANIM_NONE, &ui_ScreenPageSettings, &ui_ScreenPageSettings_screen_init);
}

/** 处理首页菜单里的蓝牙扫描入口点击。 */
static void ui_home_menu_open_ble_scan(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ui_home_nav_vertical(LV_SCR_LOAD_ANIM_NONE, &ui_ScreenPageBLEScan, &ui_ScreenPageBLEScan_screen_init);
}

/** 处理新增仪表页入口点击。 */
static void ui_home_add_page_click(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ui_home_add_page();
}

/**
 * 重建首页结构并加载目标页面
 *
 * 用于配置变化后整页重建，确保 tile 结构和运行时缓存同步刷新。
 */
void ui_home_runtime_rebuild_and_load(uint8_t page_id, lv_scr_load_anim_t anim)
{
    if (ui_ScreenPageHome != NULL) {
        ui_home_pager_deinit(&s_home_pager);
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

/** 新增一个默认仪表页。 */
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
            ui_round_shell_apply_modal_theme(s_home_notice_msgbox, lv_color_hex(0x2F80ED), 0);
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

/** 处理首页提示弹窗的关闭事件。 */
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

/** 首页刷新定时器回调。 */
static void ui_home_refresh_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    ui_home_runtime_refresh_visible_tiles(s_home_active_page);
}

/** 根据页面类型选择合适的刷新周期。 */
static uint32_t ui_home_refresh_period_ms_for_page(uint8_t page_id)
{
    if (page_id >= s_home_tile_count) {
        return UI_HOME_REFRESH_PERIOD_METRIC_MS;
    }

    switch (s_home_tile_descs[page_id].kind) {
    case UI_HOME_TILE_MENU:
        return UI_HOME_REFRESH_PERIOD_MENU_MS;
    case UI_HOME_TILE_ADD:
        return UI_HOME_REFRESH_PERIOD_ADD_MS;
    case UI_HOME_TILE_GAUGE: {
        ui_dashboard_page_type_t page_type =
            ui_home_page_type_for_gauge((uint8_t)s_home_tile_descs[page_id].gauge_index);
        switch (page_type) {
        case UI_DASHBOARD_PAGE_TYPE_GEAR:
            return UI_HOME_REFRESH_PERIOD_GEAR_MS;
        case UI_DASHBOARD_PAGE_TYPE_G_FORCE_OBD:
        case UI_DASHBOARD_PAGE_TYPE_G_FORCE_ESP32:
            return UI_HOME_REFRESH_PERIOD_GFORCE_MS;
        case UI_DASHBOARD_PAGE_TYPE_METRIC:
        default:
            return UI_HOME_REFRESH_PERIOD_METRIC_MS;
        }
    }
    default:
        return UI_HOME_REFRESH_PERIOD_METRIC_MS;
    }
}

/** 把对象设置成被动展示态，避免抢占手势和点击。 */
static void ui_home_make_passive_obj(lv_obj_t *obj)
{
    if (obj == NULL) {
        return;
    }

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

/** 按页面类型更新首页刷新定时器的周期。 */
static void ui_home_refresh_timer_apply_profile(uint8_t page_id)
{
    if (s_home_refresh_timer == NULL) {
        return;
    }

    lv_timer_set_period(s_home_refresh_timer, ui_home_refresh_period_ms_for_page(page_id));
}

/** 暂停首页刷新定时器。 */
static void ui_home_refresh_timer_suspend(void)
{
    if (s_home_refresh_timer == NULL) {
        return;
    }

    lv_timer_pause(s_home_refresh_timer);
}

/** 恢复首页刷新定时器。 */
static void ui_home_refresh_timer_resume(void)
{
    if (s_home_refresh_timer == NULL) {
        return;
    }

    lv_timer_resume(s_home_refresh_timer);
    lv_timer_reset(s_home_refresh_timer);
}

/** 把蓝牙设备名格式化成首页显示用的短文本。 */
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

/** 构建首页菜单页内容。 */
static void ui_home_create_menu_content(uint8_t tile_id, lv_obj_t *parent)
{
    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];
    lv_obj_t *vehicle;
    lv_obj_t *ble_btn;
    lv_obj_t *ble;
    lv_obj_t *btn;
    lv_obj_t *btn_label;
    lv_coord_t vehicle_y = ui_layout_px(-78);
    lv_coord_t ble_y = ui_layout_px(0);
    lv_coord_t settings_y = ui_layout_px(66);

    vehicle = lv_label_create(parent);
    lv_label_set_text(vehicle, "--");
    lv_obj_set_style_text_font(vehicle, ui_font_typoder(36), LV_PART_MAIN);
    lv_obj_set_style_text_color(vehicle, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_label_set_long_mode(vehicle, LV_LABEL_LONG_DOT);
    lv_obj_set_width(vehicle, ui_layout_px(236));
    lv_obj_set_style_text_align(vehicle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(vehicle, LV_ALIGN_CENTER, 0, vehicle_y);

    ble_btn = lv_btn_create(parent);
    lv_obj_set_size(ble_btn, ui_layout_px(236), ui_layout_px(52));
    lv_obj_align(ble_btn, LV_ALIGN_CENTER, 0, ble_y);
    ui_round_shell_apply_action_button_theme(ble_btn, lv_color_hex(0x2F80ED), false, ui_layout_px(22), 18);
    lv_obj_add_flag(ble_btn, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(ble_btn, ui_home_menu_open_ble_scan, LV_EVENT_CLICKED, NULL);

    ble = lv_label_create(ble_btn);
    lv_label_set_text(ble, "BLE: Not set");
    lv_obj_set_style_text_font(ble, ui_font_typoder(18), LV_PART_MAIN);
    lv_obj_set_style_text_color(ble, lv_color_hex(0xDCEBFF), LV_PART_MAIN);
    lv_obj_center(ble);

    btn = lv_btn_create(parent);
    lv_obj_set_size(btn, ui_layout_px(236), ui_layout_px(50));
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, settings_y);
    ui_round_shell_apply_action_button_theme(btn, lv_color_hex(0x2F80ED), true, ui_layout_px(20), 19);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(btn, ui_home_menu_open_settings, LV_EVENT_CLICKED, NULL);

    btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "SETTINGS");
    lv_obj_set_style_text_font(btn_label, ui_font_typoder(19), LV_PART_MAIN);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(btn_label);

    rt->root = parent;
    rt->menu_vehicle_label = vehicle;
    rt->menu_ble_label = ble;
    rt->menu_settings_btn = btn;
}

static void ui_home_build_gauge_layout(lv_obj_t *parent,
                                       uint8_t slot_count,
                                       ui_home_slot_layout_t layouts[UI_DASHBOARD_MAX_SLOTS])
{
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
    h = lv_obj_get_content_height(parent);
    if (h <= 0) {
        h = lv_obj_get_height(parent);
    }
    if (line_thickness < 1) {
        line_thickness = 1;
    }

    memset(layouts, 0, sizeof(ui_home_slot_layout_t) * UI_DASHBOARD_MAX_SLOTS);

    if (slot_count == 1u) {
        content_inset = ui_safe_margin() + ui_layout_px(1);
    } else if (slot_count <= 2u) {
        content_inset = ui_safe_margin() + ui_layout_px(3);
    } else if (slot_count <= 4u) {
        content_inset = ui_safe_margin() + ui_layout_px(3);
    } else {
        content_inset = ui_safe_margin() + ui_layout_px(2);
    }
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

    row_gap = ui_layout_px((row_count >= 3u) ? 4 : 6);
    col_gap = ui_layout_px((slot_count <= 2u) ? 0 : 3);
    if (row_gap < 4) {
        row_gap = 4;
    }
    if (col_gap < 2 && slot_count > 2u) {
        col_gap = 2;
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

        ui_round_shell_safe_span_for_band(card_y,
                                          card_h,
                                          ui_safe_margin() + ui_layout_px(1),
                                          &span_left,
                                          &span_right);
        layouts[0] = (ui_home_slot_layout_t){span_left, card_y, (lv_coord_t)(span_right - span_left), card_h};
        return;
    }

    for (uint8_t row = 0; row < row_count; ++row) {
        lv_coord_t span_left;
        lv_coord_t span_right;
        lv_coord_t span_w;

        ui_round_shell_safe_span_for_band(current_y, row_h, content_inset, &span_left, &span_right);
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

            ui_round_shell_create_divider(parent,
                                          divider_x,
                                          current_y,
                                          line_thickness,
                                          row_h,
                                          lv_color_hex(0x353535),
                                          168);
        }

        if ((uint8_t)(row + 1u) < row_count) {
            lv_coord_t divider_y = current_y + row_h + (row_gap / 2);
            lv_coord_t divider_left;
            lv_coord_t divider_right;
            ui_round_shell_safe_span_for_band(divider_y, line_thickness, content_inset, &divider_left, &divider_right);
            ui_round_shell_create_divider(parent,
                                          divider_left,
                                          divider_y,
                                          (lv_coord_t)(divider_right - divider_left),
                                          line_thickness,
                                          lv_color_hex(0x353535),
                                          168);
        }

        current_y = (lv_coord_t)(current_y + row_h + row_gap + line_thickness);
    }
}

/** 构建普通仪表页内容。 */
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
        lv_obj_t *card = ui_home_runtime_widgets_create_menu_card(parent,
                                                                  ui_layout_px(248),
                                                                  ui_layout_px(116),
                                                                  0);
        lv_obj_t *title = lv_label_create(card);
        lv_obj_t *hint = lv_label_create(card);

        lv_label_set_text(title, "NO SUPPORTED ITEM");
        lv_obj_set_style_text_font(title, ui_font_typoder(20), LV_PART_MAIN);
        lv_obj_set_style_text_color(title, lv_color_hex(0xD0D0D0), LV_PART_MAIN);
        lv_obj_set_width(title, LV_PCT(100));
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

        lv_label_set_text(hint, "This page has no valid metric for the current vehicle profile.");
        lv_obj_set_style_text_font(hint, ui_font_hint(11), LV_PART_MAIN);
        lv_obj_set_style_text_color(hint, lv_color_hex(0x8D8D8D), LV_PART_MAIN);
        lv_obj_set_width(hint, LV_PCT(100));
        lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        return;
    }

    ui_home_build_gauge_layout(parent, visible_count, layouts);

    for (uint8_t i = 0; i < visible_count; ++i) {
        ui_home_runtime_widgets_create_slot_card(parent,
                                                 layouts[i].x,
                                                 layouts[i].y,
                                                 layouts[i].w,
                                                 layouts[i].h,
                                                 visible_count,
                                                 &rt->name_labels[i],
                                                 &rt->value_labels[i],
                                                 &rt->unit_labels[i]);
        ui_home_runtime_widgets_apply_slot_typography(rt->name_labels[i],
                                                      rt->value_labels[i],
                                                      rt->unit_labels[i],
                                                      visible_items[i],
                                                      visible_count);
    }
}

/** 构建档位页内容。 */
static void ui_home_create_gear_content(uint8_t tile_id, lv_obj_t *parent)
{
    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];
    lv_coord_t top_pad = ui_safe_margin() + ui_layout_px(12);
    lv_coord_t side_pad = ui_safe_margin() + ui_layout_px(14);
    lv_coord_t bottom_pad = ui_safe_margin() + ui_layout_px(52);
    rt->root = parent;
    rt->slot_count = 0u;
    rt->custom_title_label = NULL;

    LV_UNUSED(top_pad);
    LV_UNUSED(bottom_pad);
    rt->gear_ring_diameter = LV_MIN((lv_coord_t)ui_screen_width(), (lv_coord_t)ui_screen_height());
    rt->gear_ring_diameter = LV_MAX(rt->gear_ring_diameter, ui_layout_px(220));
    rt->gear_ring_width = LV_MAX(ui_layout_px(10),
                                 (lv_coord_t)((rt->gear_ring_diameter - ui_layout_px(4)) / 16));
    rt->gear_ring_draw_obj = lv_obj_create(parent);
    lv_obj_remove_style_all(rt->gear_ring_draw_obj);
    lv_obj_set_size(rt->gear_ring_draw_obj, rt->gear_ring_diameter, rt->gear_ring_diameter);
    lv_obj_align(rt->gear_ring_draw_obj, LV_ALIGN_CENTER, 0, 0);
    ui_home_make_passive_obj(rt->gear_ring_draw_obj);
    lv_obj_add_event_cb(rt->gear_ring_draw_obj, ui_home_gear_ring_draw_event, LV_EVENT_DRAW_MAIN, rt);

    rt->custom_value_label = lv_label_create(parent);
    lv_label_set_text(rt->custom_value_label, "N");
    lv_obj_set_style_text_font(rt->custom_value_label, ui_font_typoder(156), LV_PART_MAIN);
    lv_obj_set_style_text_color(rt->custom_value_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(rt->custom_value_label, LV_ALIGN_CENTER, 0, ui_layout_px(-4));

    rt->custom_status_label = lv_label_create(parent);
    lv_label_set_text(rt->custom_status_label, "");
    lv_obj_set_width(rt->custom_status_label, ui_screen_width() - (side_pad * 2));
    lv_obj_set_style_text_font(rt->custom_status_label, ui_font_hint(10), LV_PART_MAIN);
    lv_obj_set_style_text_color(rt->custom_status_label, lv_color_hex(0x6E6E6E), LV_PART_MAIN);
    lv_obj_set_style_text_align(rt->custom_status_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(rt->custom_status_label,
                 LV_ALIGN_BOTTOM_MID,
                 0,
                 -(ui_safe_margin() + ui_layout_px(26)));
}

static void ui_home_create_gforce_content(uint8_t tile_id,
                                          lv_obj_t *parent,
                                          ui_dashboard_page_type_t page_type)
{
    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];
    lv_coord_t top_pad = ui_safe_margin() + ui_layout_px(6);
    lv_coord_t side_pad = ui_safe_margin() + ui_layout_px(6);
    lv_coord_t bottom_pad = ui_safe_margin() + ui_layout_px(6);
    lv_coord_t title_h = 0;
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

    rt->root = parent;
    rt->slot_count = 0u;

    lv_obj_update_layout(parent);
    w = lv_obj_get_width(parent);
    h = lv_obj_get_height(parent);
    LV_UNUSED(page_type);
    plot_size = LV_MIN(w - (side_pad * 2), h - top_pad - bottom_pad - title_h);
    plot_size = LV_MAX(plot_size, ui_layout_px(180));
    plot_x = (w - plot_size) / 2;
    plot_y = top_pad + LV_MAX(0, (h - top_pad - bottom_pad - title_h - plot_size) / 2);
    radius = plot_size / 2;
    axis_thickness = LV_MAX(1, plot_size / 110);
    outer_border = LV_MAX(2, plot_size / 48);
    mid_size = (plot_size * 68) / 100;
    inner_size = (plot_size * 38) / 100;
    rt->gforce_plot_size = plot_size;
    rt->gforce_plot_radius = radius - LV_MAX(ui_layout_px(10), plot_size / 18);

    rt->custom_title_label = NULL;

    rt->gforce_plot = lv_obj_create(parent);
    lv_obj_remove_style_all(rt->gforce_plot);
    lv_obj_set_size(rt->gforce_plot, plot_size, plot_size);
    lv_obj_set_pos(rt->gforce_plot, plot_x, plot_y);
    ui_home_make_passive_obj(rt->gforce_plot);
    lv_obj_set_style_radius(rt->gforce_plot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_plot, LV_OPA_TRANSP, LV_PART_MAIN);

    rt->gforce_outer_ring = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_outer_ring);
    ui_home_make_passive_obj(rt->gforce_outer_ring);
    lv_obj_set_size(rt->gforce_outer_ring, plot_size, plot_size);
    lv_obj_center(rt->gforce_outer_ring);
    lv_obj_set_style_radius(rt->gforce_outer_ring, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(rt->gforce_outer_ring, outer_border, LV_PART_MAIN);
    lv_obj_set_style_border_color(rt->gforce_outer_ring, lv_color_hex(0xF1F1F1), LV_PART_MAIN);
    lv_obj_set_style_border_opa(rt->gforce_outer_ring, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_outer_ring, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_outer_ring, 0, LV_PART_MAIN);

    rt->gforce_mid_ring = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_mid_ring);
    ui_home_make_passive_obj(rt->gforce_mid_ring);
    lv_obj_set_size(rt->gforce_mid_ring, mid_size, mid_size);
    lv_obj_center(rt->gforce_mid_ring);
    lv_obj_set_style_radius(rt->gforce_mid_ring, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(rt->gforce_mid_ring, LV_MAX(1, outer_border - 1), LV_PART_MAIN);
    lv_obj_set_style_border_color(rt->gforce_mid_ring, lv_color_hex(0x8C8C8C), LV_PART_MAIN);
    lv_obj_set_style_border_opa(rt->gforce_mid_ring, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_mid_ring, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_mid_ring, 0, LV_PART_MAIN);

    rt->gforce_inner_ring = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_inner_ring);
    ui_home_make_passive_obj(rt->gforce_inner_ring);
    lv_obj_set_size(rt->gforce_inner_ring, inner_size, inner_size);
    lv_obj_center(rt->gforce_inner_ring);
    lv_obj_set_style_radius(rt->gforce_inner_ring, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(rt->gforce_inner_ring, LV_MAX(1, outer_border - 1), LV_PART_MAIN);
    lv_obj_set_style_border_color(rt->gforce_inner_ring, lv_color_hex(0x5E5E5E), LV_PART_MAIN);
    lv_obj_set_style_border_opa(rt->gforce_inner_ring, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_inner_ring, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_inner_ring, 0, LV_PART_MAIN);

    rt->gforce_axis_h = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_axis_h);
    ui_home_make_passive_obj(rt->gforce_axis_h);
    lv_obj_set_size(rt->gforce_axis_h, plot_size - (plot_size / 7), axis_thickness);
    lv_obj_center(rt->gforce_axis_h);
    lv_obj_set_style_bg_color(rt->gforce_axis_h, lv_color_hex(0xBDBDBD), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_axis_h, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_axis_h, 0, LV_PART_MAIN);

    rt->gforce_axis_v = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_axis_v);
    ui_home_make_passive_obj(rt->gforce_axis_v);
    lv_obj_set_size(rt->gforce_axis_v, axis_thickness, plot_size - (plot_size / 7));
    lv_obj_center(rt->gforce_axis_v);
    lv_obj_set_style_bg_color(rt->gforce_axis_v, lv_color_hex(0xBDBDBD), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_axis_v, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_axis_v, 0, LV_PART_MAIN);

    rt->gforce_vector_line = lv_line_create(rt->gforce_plot);
    ui_home_make_passive_obj(rt->gforce_vector_line);
    lv_obj_set_size(rt->gforce_vector_line, plot_size, plot_size);
    lv_obj_center(rt->gforce_vector_line);
    lv_obj_set_style_line_width(rt->gforce_vector_line, LV_MAX(1, plot_size / 110), LV_PART_MAIN);
    lv_obj_set_style_line_color(rt->gforce_vector_line, lv_color_hex(0xFF8E76), LV_PART_MAIN);
    lv_obj_set_style_line_opa(rt->gforce_vector_line, 40, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(rt->gforce_vector_line, true, LV_PART_MAIN);

    rt->gforce_dot_glow = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_dot_glow);
    ui_home_make_passive_obj(rt->gforce_dot_glow);
    lv_obj_set_size(rt->gforce_dot_glow,
                    LV_MAX(ui_layout_px(20), plot_size / 10),
                    LV_MAX(ui_layout_px(20), plot_size / 10));
    lv_obj_align(rt->gforce_dot_glow, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_radius(rt->gforce_dot_glow, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(rt->gforce_dot_glow, lv_color_hex(0xFF5A42), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_dot_glow, 40, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_dot_glow, 0, LV_PART_MAIN);

    rt->gforce_dot = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_dot);
    ui_home_make_passive_obj(rt->gforce_dot);
    lv_obj_set_size(rt->gforce_dot,
                    LV_MAX(ui_layout_px(12), plot_size / 18),
                    LV_MAX(ui_layout_px(12), plot_size / 18));
    lv_obj_align(rt->gforce_dot, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_radius(rt->gforce_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(rt->gforce_dot, lv_color_hex(0xFF4D3A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(rt->gforce_dot, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_dot, 0, LV_PART_MAIN);

    rt->gforce_center_dot = lv_obj_create(rt->gforce_plot);
    lv_obj_remove_style_all(rt->gforce_center_dot);
    ui_home_make_passive_obj(rt->gforce_center_dot);
    lv_obj_set_size(rt->gforce_center_dot,
                    LV_MAX(ui_layout_px(8), plot_size / 34),
                    LV_MAX(ui_layout_px(8), plot_size / 34));
    lv_obj_center(rt->gforce_center_dot);
    lv_obj_set_style_radius(rt->gforce_center_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(rt->gforce_center_dot, lv_color_hex(0xFFF6D0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rt->gforce_center_dot, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(rt->gforce_center_dot, 0, LV_PART_MAIN);
    lv_obj_move_foreground(rt->gforce_vector_line);
    lv_obj_move_foreground(rt->gforce_dot_glow);
    lv_obj_move_foreground(rt->gforce_center_dot);
    lv_obj_move_foreground(rt->gforce_dot);

    ui_home_gforce_update_plot(rt, 0.0f, 0.0f);
}

/** 对浮点数做区间钳制。 */
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

/** 更新 G-force 页面上的轨迹点和读数。 */
static void ui_home_gforce_update_plot(ui_home_tile_runtime_t *rt, float lat_g, float lon_g)
{
    float lat_target;
    float lon_target;
    float plot_scale;
    lv_coord_t center;
    lv_coord_t dot_x;
    lv_coord_t dot_y;
    lv_coord_t dot_size;
    bool has_force;
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

    if ((lat_target == 0.0f) && (lon_target == 0.0f)) {
        rt->gforce_display_lat_g = 0.0f;
        rt->gforce_display_lon_g = 0.0f;
    } else {
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

    center = rt->gforce_plot_size / 2;
    plot_scale = (float)rt->gforce_plot_radius / UI_HOME_GFORCE_MAX_G;
    dot_x = (lv_coord_t)lroundf((float)center + (rt->gforce_display_lat_g * plot_scale));
    dot_y = (lv_coord_t)lroundf((float)center - (rt->gforce_display_lon_g * plot_scale));
    dot_size = lv_obj_get_width(rt->gforce_dot);
    has_force = (rt->gforce_display_lat_g != 0.0f) || (rt->gforce_display_lon_g != 0.0f);

    rt->gforce_vector_points[0].x = center;
    rt->gforce_vector_points[0].y = center;
    rt->gforce_vector_points[1].x = dot_x;
    rt->gforce_vector_points[1].y = dot_y;
    lv_line_set_points(rt->gforce_vector_line, rt->gforce_vector_points, 2);
    lv_obj_set_style_line_opa(rt->gforce_vector_line, has_force ? 40 : 0, LV_PART_MAIN);

    if (rt->gforce_dot_glow != NULL) {
        lv_coord_t glow_size = lv_obj_get_width(rt->gforce_dot_glow);
        lv_obj_set_style_bg_opa(rt->gforce_dot_glow, has_force ? 40 : 0, LV_PART_MAIN);
        lv_obj_align(rt->gforce_dot_glow,
                     LV_ALIGN_TOP_LEFT,
                     dot_x - (glow_size / 2),
                     dot_y - (glow_size / 2));
    }
    lv_obj_align(rt->gforce_dot,
                 LV_ALIGN_TOP_LEFT,
                 dot_x - (dot_size / 2),
                 dot_y - (dot_size / 2));
}

/** 构建“新增页面”入口页内容。 */
static void ui_home_create_add_content(uint8_t tile_id, lv_obj_t *parent)
{
    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];
    lv_obj_t *title = NULL;
    lv_obj_t *plus_wrap;
    lv_obj_t *plus_h;
    lv_obj_t *plus_v;
    lv_coord_t btn_size;
    lv_coord_t plus_size;
    lv_coord_t bar_long;
    lv_coord_t bar_thick;

    ui_round_shell_create_title_block(parent,
                                      "ADD PAGE",
                                      NULL,
                                      ui_layout_px(26),
                                      16,
                                      ui_layout_px(6),
                                      11,
                                      &title,
                                      NULL);

    lv_obj_t *btn = lv_btn_create(parent);
    btn_size = ui_layout_px(240);
    lv_obj_set_size(btn, btn_size, btn_size);
    lv_obj_center(btn);
    ui_home_runtime_widgets_apply_add_button_theme(btn, btn_size);
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

/** 处理仪表页长按进入编辑模式。 */
static void ui_home_gauge_long_pressed(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_LONG_PRESSED) {
        return;
    }
    ui_home_enter_edit_mode((uint8_t)(uintptr_t)lv_event_get_user_data(e));
}

/** 处理编辑模式里的返回操作。 */
static void ui_home_edit_back(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ui_home_exit_edit_mode();
}

/** 处理编辑模式里的删除操作。 */
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
    ui_round_shell_apply_modal_theme(s_home_delete_msgbox, lv_color_hex(0xEB5757), 1);

    lv_obj_add_event_cb(s_home_delete_msgbox, ui_home_delete_confirm_result, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_home_delete_msgbox, ui_home_delete_confirm_result, LV_EVENT_DELETE, NULL);
}

/** 处理编辑模式里的配置入口。 */
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

/**
 * 进入首页编辑模式
 *
 * 为当前页面挂上编辑遮罩和入口按钮。
 */
static void ui_home_enter_edit_mode(uint8_t page_id)
{
    ui_home_tile_runtime_t *rt;
    lv_obj_t *zone_edit;
    lv_obj_t *zone_del;
    lv_obj_t *zone_back;
    lv_obj_t *label;

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

    ui_home_pager_set_locked(&s_home_pager, true);

    lv_obj_set_style_transform_zoom(rt->root, (lv_coord_t)(256 * 12 / 10), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_translate_x(rt->root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_translate_y(rt->root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_opa(rt->root, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_home_edit_overlay = lv_obj_create(s_home_tiles[page_id]);
    lv_obj_remove_style_all(s_home_edit_overlay);
    lv_obj_set_size(s_home_edit_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_home_edit_overlay);
    lv_obj_set_style_bg_color(s_home_edit_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_home_edit_overlay, 118, LV_PART_MAIN);
    lv_obj_clear_flag(s_home_edit_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_home_edit_overlay, LV_OBJ_FLAG_CLICKABLE);

    zone_edit = lv_obj_create(s_home_edit_overlay);
    lv_obj_remove_style_all(zone_edit);
    lv_obj_set_size(zone_edit, LV_PCT(50), LV_PCT(50));
    lv_obj_align(zone_edit, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(zone_edit, lv_color_hex(0x0078D4), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(zone_edit, 148, LV_PART_MAIN);
    lv_obj_clear_flag(zone_edit, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(zone_edit, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(zone_edit, ui_home_edit_config, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(zone_edit);
    lv_label_set_text(label, "EDIT");
    lv_obj_set_style_text_font(label, ui_font_typoder(24), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, ui_layout_px(16), ui_layout_px(20));

    zone_del = lv_obj_create(s_home_edit_overlay);
    lv_obj_remove_style_all(zone_del);
    lv_obj_set_size(zone_del, LV_PCT(50), LV_PCT(50));
    lv_obj_align(zone_del, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(zone_del, lv_color_hex(0xFF5252), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(zone_del, 148, LV_PART_MAIN);
    lv_obj_clear_flag(zone_del, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(zone_del, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(zone_del, ui_home_edit_delete, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(zone_del);
    lv_label_set_text(label, "DELETE");
    lv_obj_set_style_text_font(label, ui_font_typoder(22), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, ui_layout_px(-12), ui_layout_px(20));

    zone_back = lv_obj_create(s_home_edit_overlay);
    lv_obj_remove_style_all(zone_back);
    lv_obj_set_size(zone_back, LV_PCT(100), LV_PCT(50));
    lv_obj_align(zone_back, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(zone_back, lv_color_hex(0x0DBC79), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(zone_back, 148, LV_PART_MAIN);
    lv_obj_clear_flag(zone_back, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(zone_back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(zone_back, ui_home_edit_back, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(zone_back);
    lv_label_set_text(label, "BACK");
    lv_obj_set_style_text_font(label, ui_font_typoder(28), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, ui_layout_px(-16));
}

/** 退出首页编辑模式并清理遮罩。 */
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

    ui_home_pager_set_locked(&s_home_pager, false);

    s_home_edit_mode = false;
    s_home_edit_page = UI_HOME_PAGE_MENU_ID;
    s_home_edit_target_root = NULL;

    ui_home_reset_tile_effect_cache(page_id);
}

/** 处理删除仪表页确认弹窗的结果。 */
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

/** 挂载某个首页页面的实际内容。 */
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

/** 卸载某个首页页面的实际内容。 */
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

/** 让首页只保留当前页附近几个 tile 处于挂载状态。 */
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

/** 自绘档位页的 RPM ring。 */
static void ui_home_gear_ring_draw_event(lv_event_t *e)
{
    ui_home_tile_runtime_t *rt = (ui_home_tile_runtime_t *)lv_event_get_user_data(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
    lv_area_t coords;
    lv_point_t center;
    lv_draw_arc_dsc_t arc_dsc;
    float usable_deg;
    float accum_deg = 0.0f;
    uint16_t rpm;
    uint16_t redline_rpm;
    uint16_t max_rpm;
    uint16_t segment_rpm;
    uint16_t segment_count;
    lv_coord_t radius;

    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN || rt == NULL || obj == NULL || draw_ctx == NULL) {
        return;
    }
    if (!rt->gear_ring_enabled || rt->gear_ring_rpm == 0u || rt->gear_ring_max_rpm == 0u) {
        return;
    }

    rpm = rt->gear_ring_rpm;
    redline_rpm = rt->gear_ring_redline_rpm;
    max_rpm = rt->gear_ring_max_rpm;
    segment_rpm = rt->gear_ring_segment_rpm;

    if (rpm > max_rpm) {
        rpm = max_rpm;
    }
    if (redline_rpm > max_rpm) {
        redline_rpm = max_rpm;
    }
    if (segment_rpm < UI_HOME_GEAR_RING_MIN_SEGMENT_RPM) {
        segment_rpm = UI_HOME_GEAR_RING_MIN_SEGMENT_RPM;
    }

    segment_count = (uint16_t)((max_rpm + (segment_rpm - 1u)) / segment_rpm);
    if (segment_count == 0u) {
        segment_count = 1u;
    }
    if (segment_count > UI_HOME_GEAR_RING_MAX_SEGMENTS) {
        segment_count = UI_HOME_GEAR_RING_MAX_SEGMENTS;
    }

    usable_deg = 360.0f - (((float)segment_count) * UI_HOME_GEAR_RING_GAP_DEG);
    if (usable_deg < 1.0f) {
        usable_deg = 1.0f;
    }

    coords = obj->coords;
    center.x = (lv_coord_t)((coords.x1 + coords.x2) / 2);
    center.y = (lv_coord_t)((coords.y1 + coords.y2) / 2);
    radius = (lv_coord_t)(LV_MIN(lv_area_get_width(&coords), lv_area_get_height(&coords)) / 2);
    if (radius <= 0) {
        return;
    }

    lv_draw_arc_dsc_init(&arc_dsc);
    arc_dsc.width = rt->gear_ring_width;
    arc_dsc.rounded = 0;
    arc_dsc.opa = LV_OPA_COVER;

    for (uint16_t i = 0; i < segment_count; ++i) {
        uint16_t seg_start_rpm = (uint16_t)(i * segment_rpm);
        uint16_t seg_end_rpm = (uint16_t)(seg_start_rpm + segment_rpm);
        uint16_t seg_span_rpm;
        float seg_deg_full;
        float seg_deg_fill;
        float fill_ratio;
        uint16_t start_angle;
        uint16_t end_angle;

        if (seg_end_rpm > max_rpm) {
            seg_end_rpm = max_rpm;
        }
        seg_span_rpm = (uint16_t)(seg_end_rpm - seg_start_rpm);
        if (seg_span_rpm == 0u) {
            continue;
        }

        seg_deg_full = usable_deg * ((float)seg_span_rpm / (float)max_rpm);
        fill_ratio = ui_home_clampf(((float)rpm - (float)seg_start_rpm) / (float)seg_span_rpm,
                                    0.0f,
                                    1.0f);
        seg_deg_fill = seg_deg_full * fill_ratio;
        if (seg_deg_fill <= 0.1f) {
            accum_deg += seg_deg_full + UI_HOME_GEAR_RING_GAP_DEG;
            continue;
        }

        start_angle = (uint16_t)(((int32_t)(90.0f + accum_deg)) % 360);
        end_angle = (uint16_t)(((int32_t)(90.0f + accum_deg + seg_deg_fill)) % 360);
        arc_dsc.color = (seg_end_rpm > redline_rpm)
                            ? (rt->gear_ring_blink_red ? lv_color_hex(0xFF3B30)
                                                       : lv_color_hex(0xF5F5F5))
                            : lv_color_hex(0xF5F5F5);
        lv_draw_arc(draw_ctx, &arc_dsc, &center, (uint16_t)radius, start_angle, end_angle);
        accum_deg += seg_deg_full + UI_HOME_GEAR_RING_GAP_DEG;
    }
}

static void ui_home_update_gear_ring(ui_home_tile_runtime_t *rt,
                                     uint16_t rpm,
                                     uint16_t redline_rpm,
                                     uint16_t max_rpm,
                                     uint16_t segment_rpm,
                                     bool enabled,
                                     bool blink_red)
{
    if (rt == NULL) {
        return;
    }
    rt->gear_ring_rpm = rpm;
    rt->gear_ring_redline_rpm = redline_rpm;
    rt->gear_ring_max_rpm = max_rpm;
    rt->gear_ring_segment_rpm = segment_rpm;
    rt->gear_ring_enabled = enabled;
    rt->gear_ring_blink_red = blink_red;
    if (rt->gear_ring_draw_obj != NULL) {
        lv_obj_invalidate(rt->gear_ring_draw_obj);
    }
}

/**
 * 刷新档位页
 *
 * 负责同步档位文本、状态提示和 RPM ring 显示。
 */
static void ui_home_refresh_gear_tile(ui_home_tile_runtime_t *rt)
{
    uint16_t rpm = obd_data_get_rpm();
    // Debug mock: force gear page RPM to 9000 for ring performance validation.
    // uint16_t rpm = 9000u;
    uint16_t speed = obd_data_get_speed();
    const ui_dashboard_page_cfg_t *page_cfg = NULL;
    uint16_t redline_rpm = UI_DASHBOARD_GEAR_REDLINE_RPM_DEFAULT;
    uint16_t max_rpm = UI_DASHBOARD_GEAR_MAX_RPM_DEFAULT;
    uint16_t segment_rpm = UI_DASHBOARD_GEAR_SEGMENT_RPM_DEFAULT;
    bool rpm_ring_enabled = true;
    enGear gear = GEAR_NEUTRAL;
    bool valid_gear = false;
    const char *gear_text = "N";
    const char *status_text = "";
    int64_t now_us = esp_timer_get_time();
    bool blink_red = false;

    if (rt == NULL || rt->root == NULL || rt->custom_value_label == NULL ||
        rt->custom_status_label == NULL) {
        return;
    }

    if (rt->root != NULL) {
        uint8_t tile_id = (uint8_t)(rt - s_home_tile_runtime);
        int8_t gauge_index = s_home_tile_descs[tile_id].gauge_index;
        if (gauge_index >= 0) {
            page_cfg = ui_home_get_gauge_cfg((uint8_t)gauge_index);
        }
    }
    if (page_cfg != NULL) {
        redline_rpm = ui_dashboard_page_get_gear_redline_rpm(page_cfg);
        max_rpm = ui_dashboard_page_get_gear_max_rpm(page_cfg);
        segment_rpm = ui_dashboard_page_get_gear_segment_rpm_step(page_cfg);
        rpm_ring_enabled = ui_dashboard_page_is_gear_rpm_ring_enabled(page_cfg);
    }
    blink_red = (rpm > redline_rpm) && (((now_us / UI_HOME_GEAR_BLINK_PERIOD_US) & 0x1LL) != 0LL);
    lv_obj_set_style_text_color(rt->custom_value_label,
                                blink_red ? lv_color_hex(0xFF3B30) : lv_color_hex(0xFFFFFF),
                                LV_PART_MAIN);

    if (vehicle_profile_is_active_zc6()) {
        valid_gear = obd_data_get_actual_gear(&gear);
        if (!valid_gear) {
            gear_text = "-";
            if (!elm327_ble_is_connected()) {
                status_text = "Connect OBD to read gear CAN";
                rt->gear_status_wait_start_us = 0;
            } else {
                if (rt->gear_status_wait_start_us == 0) {
                    rt->gear_status_wait_start_us = now_us;
                }
                if ((now_us - rt->gear_status_wait_start_us) >= 2500000LL) {
                    status_text = "No 0x141 gear CAN, adapter may not support monitor mode";
                } else {
                    status_text = "Waiting for ZC6 gear CAN 0x141";
                }
            }
        } else {
            rt->gear_status_wait_start_us = 0;
            status_text = "";
        }
    } else {
        gear = calculate_gear((float)rpm, (float)speed);
        valid_gear = true;
        rt->gear_status_wait_start_us = 0;
        status_text = "";
    }

    if (!valid_gear) {
        ui_home_update_gear_ring(rt,
                                 rpm,
                                 redline_rpm,
                                 max_rpm,
                                 segment_rpm,
                                 rpm_ring_enabled,
                                 blink_red);
        ui_label_set_text_if_changed(rt->custom_value_label, gear_text);
        ui_label_set_text_if_changed(rt->custom_status_label, status_text);
        return;
    }

    switch (gear) {
    case GEAR_1: gear_text = "1"; break;
    case GEAR_2: gear_text = "2"; break;
    case GEAR_3: gear_text = "3"; break;
    case GEAR_4: gear_text = "4"; break;
    case GEAR_5: gear_text = "5"; break;
    case GEAR_6: gear_text = "6"; break;
    case GEAR_REVERSE: gear_text = "R"; break;
    case GEAR_NEUTRAL:
    default:
        gear_text = "N";
        break;
    }

    ui_home_update_gear_ring(rt,
                             rpm,
                             redline_rpm,
                             max_rpm,
                             segment_rpm,
                             rpm_ring_enabled,
                             blink_red);
    ui_label_set_text_if_changed(rt->custom_value_label, gear_text);
    ui_label_set_text_if_changed(rt->custom_status_label, status_text);
}

/** 刷新 G-force 页面数据。 */
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
    ui_home_gforce_update_plot(rt,
                               (lat_x100 > -32768) ? ((float)lat_x100 / 100.0f) : 0.0f,
                               (lon_x100 > -32768) ? ((float)lon_x100 / 100.0f) : 0.0f);
}

/** 刷新普通仪表页里的各个指标卡片。 */
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

        ui_home_runtime_widgets_apply_slot_typography(rt->name_labels[i],
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

/** 刷新单个首页 tile 的显示内容。 */
static void ui_home_runtime_refresh_tile(uint8_t tile_id)
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

    if (tile_id >= s_home_tile_count || !s_home_tile_mounted[tile_id]) {
        return;
    }

    ui_home_tile_runtime_t *rt = &s_home_tile_runtime[tile_id];
    if (rt->root == NULL) {
        return;
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

/** 刷新当前页及其相邻页，兼顾滑动时的预热显示。 */
static void ui_home_runtime_refresh_visible_tiles(uint8_t active_page)
{
    if (active_page >= s_home_tile_count) {
        return;
    }

    if (active_page > 0u) {
        ui_home_runtime_refresh_tile((uint8_t)(active_page - 1u));
    }
    ui_home_runtime_refresh_tile(active_page);
    if ((uint8_t)(active_page + 1u) < s_home_tile_count) {
        ui_home_runtime_refresh_tile((uint8_t)(active_page + 1u));
    }
}

/** 立即刷新当前激活页。 */
void ui_home_runtime_refresh_active_tile(void)
{
    ui_home_runtime_refresh_tile(s_home_active_page);
}

/** 处理分页器完成页面提交后的同步逻辑。 */
static void ui_home_pager_commit_handler(const ui_home_pager_commit_t *commit, void *user)
{
    int64_t mount_start_us;
    int64_t refresh_begin_us;

    LV_UNUSED(user);
    if (commit == NULL) {
        return;
    }

    if (!commit->changed) {
        ui_home_page_switch_perf_cancel();
        return;
    }

    if (!s_home_nav_perf.active) {
        ui_home_page_switch_perf_begin(commit->from_page);
    }

    ui_home_page_switch_perf_commit(commit->to_page);
    s_home_active_page = commit->to_page;
    mount_start_us = esp_timer_get_time();
    ui_home_sync_tile_mounts(commit->to_page);
    s_home_nav_perf.mount_us = esp_timer_get_time() - mount_start_us;

    refresh_begin_us = esp_timer_get_time();
    ui_home_refresh_timer_apply_profile(commit->to_page);
    aux_sensor_demand_refresh();
    s_home_nav_perf.refresh_us = esp_timer_get_time() - refresh_begin_us;
}

/** 在分页器确认横向拖拽后启动配套状态切换。 */
static void ui_home_pager_drag_begin_handler(uint8_t from_page, void *user)
{
    LV_UNUSED(user);
    if (s_home_edit_mode || s_home_nav_perf.active) {
        return;
    }

    /*
     * Start perf tracing and suspend the refresh timer as soon as the gesture
     * is confirmed horizontal, so drag-time work is captured instead of only
     * the release animation tail.
     */
    ui_home_page_switch_perf_begin(from_page);
}

/** 处理首页菜单页的纵向手势入口。 */
static void ui_home_pager_vertical_handler(lv_dir_t dir, uint8_t active_page, void *user)
{
    LV_UNUSED(user);
    if (s_home_edit_mode || active_page != UI_HOME_PAGE_MENU_ID) {
        return;
    }

    ui_home_open_menu_overlay(dir);
}

/** 根据纵向手势方向打开菜单页对应的外层界面。 */
static void ui_home_open_menu_overlay(lv_dir_t dir)
{
    if (dir == LV_DIR_TOP) {
        ui_home_nav_vertical(LV_SCR_LOAD_ANIM_NONE, &ui_ScreenPageBLEScan, &ui_ScreenPageBLEScan_screen_init);
    } else if (dir == LV_DIR_BOTTOM) {
        ui_home_nav_vertical(LV_SCR_LOAD_ANIM_NONE, &ui_ScreenPageSettings, &ui_ScreenPageSettings_screen_init);
    }
}

/**
 * 初始化首页运行时界面
 *
 * 创建分页器、挂载页面并启动刷新定时器。
 */
void ui_home_runtime_screen_init(void)
{
    ui_home_pager_config_t pager_cfg;
    lv_coord_t screen_width;
    lv_coord_t screen_height;

    if (ui_ScreenPageHome != NULL) {
        return;
    }

    ui_home_rebuild_tile_descriptors();
    ui_home_reset_screen_state();

    ui_ScreenPageHome = lv_obj_create(NULL);
    ui_round_screen_apply_base(ui_ScreenPageHome, lv_color_hex(0x000000));
    lv_obj_set_style_border_width(ui_ScreenPageHome, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui_ScreenPageHome, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_update_layout(ui_ScreenPageHome);

    screen_width = lv_obj_get_width(ui_ScreenPageHome);
    screen_height = lv_obj_get_height(ui_ScreenPageHome);
    pager_cfg.parent = ui_ScreenPageHome;
    pager_cfg.width = screen_width;
    pager_cfg.height = screen_height;
    pager_cfg.page_count = s_home_tile_count;
    pager_cfg.initial_page = s_home_active_page;
    pager_cfg.drag_begin_cb = ui_home_pager_drag_begin_handler;
    pager_cfg.commit_cb = ui_home_pager_commit_handler;
    pager_cfg.vertical_cb = ui_home_pager_vertical_handler;
    pager_cfg.user = NULL;
    ui_home_pager_init(&s_home_pager, &pager_cfg);

    for (uint8_t i = 0; i < s_home_tile_count; ++i) {
        s_home_tiles[i] = ui_home_pager_page(&s_home_pager, i);
        if (s_home_tiles[i] != NULL) {
            lv_obj_add_flag(s_home_tiles[i], LV_OBJ_FLAG_EVENT_BUBBLE);
        }
    }

    ui_home_set_active_page(s_home_active_page, LV_ANIM_OFF);
    ui_home_runtime_refresh_visible_tiles(s_home_active_page);
    if (s_home_refresh_timer == NULL) {
        s_home_refresh_timer = lv_timer_create(ui_home_refresh_timer_cb,
                                               ui_home_refresh_period_ms_for_page(s_home_active_page),
                                               NULL);
    }
}

/** 加载首页并切换到指定页面。 */
static void ui_home_load(lv_scr_load_anim_t anim, uint8_t page_id)
{
    ui_home_runtime_screen_init();
    ui_home_set_active_page(page_id, LV_ANIM_OFF);
    ui_home_runtime_refresh_visible_tiles(page_id);
    lv_scr_load_anim(ui_ScreenPageHome, anim, UI_NAV_ANIM_MS, 0, false);
}

/** 对外暴露的首页切页入口。 */
void ui_home_runtime_show_page(uint8_t page_id, lv_scr_load_anim_t anim)
{
    ui_home_load(anim, page_id);
    aux_sensor_demand_refresh();
}

/** 对外暴露的默认页转换入口。 */
uint8_t ui_home_runtime_page_from_default(uint8_t default_page)
{
    return ui_home_page_from_default_page(default_page);
}

/** 判断当前激活页是否依赖某个仪表项。 */
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

/** 判断当前激活页是否属于某种页面类型。 */
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

/** 判断当前激活的档位页是否启用了 RPM ring。 */
bool ui_home_runtime_active_page_gear_rpm_ring_enabled(void)
{
    const ui_dashboard_page_cfg_t *page;

    if (!ui_home_runtime_active_page_uses_type(UI_DASHBOARD_PAGE_TYPE_GEAR)) {
        return false;
    }

    page = ui_home_get_gauge_cfg((uint8_t)s_home_tile_descs[s_home_active_page].gauge_index);
    if (page == NULL) {
        return false;
    }

    return ui_dashboard_page_is_gear_rpm_ring_enabled(page);
}

/** 重置首页运行时状态，并准备下次按指定初始页启动。 */
void ui_home_runtime_reset(uint8_t initial_page_id)
{
    if (s_home_refresh_timer != NULL) {
        lv_timer_del(s_home_refresh_timer);
        s_home_refresh_timer = NULL;
    }
    ui_home_pager_deinit(&s_home_pager);
    ui_ScreenPageHome = NULL;
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
