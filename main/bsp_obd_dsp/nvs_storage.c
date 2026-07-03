#include "nvs_storage.h"

#include <stdio.h>
#include <string.h>

#include "bsp_obd_dsp/nvs_error_log_logic.h"
#include "app_obd_dsp/vehicle_profiles.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "export_path/ui.h"
#include "export_path/ui_runtime_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#define TAG "nvs_storage"
#define NS_CFG "cfg"
#define KEY_CFG "settings"
#define NS_STAT "stat"
#define KEY_STAT "runtime"
#define NS_DIAG "diag"
#define KEY_ERR_LOG "errors"
#define STAT_FLUSH_PERIOD_MS 30000
#define ERROR_FLUSH_PERIOD_MS 5000
#define NVS_FLUSH_TASK_STACK_BYTES 4096

typedef struct {
    uint8_t protocol;
    theme_cfg_t theme_cfg;
    char ble_device_name[32];
    uint8_t default_page;
    uint8_t brightness_day;
    uint8_t vehicle_profile_idx;
    uint16_t brake_temp_warn_c;
    uint16_t oil_pressure_warn_x10;
    uint8_t temp_display_map[3];
    uint8_t info_display_map[5];
    uint8_t needle_source_idx;
    uint8_t rsv[2];
} legacy_nvs_user_cfg_v0_t;

typedef struct {
    uint8_t slot_count;
    uint8_t slot_items[UI_DASHBOARD_MAX_SLOTS];
    uint8_t rsv;
} legacy_ui_dashboard_page_cfg_v1_t;

typedef struct {
    uint8_t version;
    uint8_t gauge_page_count;
    uint8_t rsv[2];
    legacy_ui_dashboard_page_cfg_v1_t pages[UI_DASHBOARD_MAX_PAGES];
} legacy_ui_dashboard_cfg_v1_t;

typedef struct {
    uint8_t protocol;
    theme_cfg_t theme_cfg;
    char ble_device_name[32];
    uint8_t default_page;
    uint8_t brightness_day;
    uint8_t vehicle_profile_idx;
    uint16_t brake_temp_warn_c;
    uint16_t oil_pressure_warn_x10;
    uint8_t temp_display_map[3];
    uint8_t info_display_map[5];
    uint8_t needle_source_idx;
    uint8_t rsv[2];
    legacy_ui_dashboard_cfg_v1_t dashboard_cfg;
} legacy_nvs_user_cfg_v1_t;

static nvs_user_cfg_t s_cfg = {
    .protocol = 0,
    .theme_cfg.theme = 1,
    .theme_cfg.user_theme_domiant_color = COLOR_DOMIANT_PINK,
    .theme_cfg.user_theme_secondary_color = COLOR_SECONDARY_PINK,
    .ble_device_name = "",
    .temp_display_map = {0, 1, 2},
    .info_display_map = {0, 2, 3, 4, 1},
    .brake_temp_warn_c = 600,
    .oil_pressure_warn_x10 = 80,
};
static nvs_stat_t s_stat = {0};
static nvs_error_log_t s_error_log = {
    .version = NVS_ERROR_LOG_VERSION,
};
static bool s_stat_dirty = false;
static bool s_error_log_dirty = false;
static uint32_t s_stat_dirty_seq = 0u;
static uint32_t s_error_log_dirty_seq = 0u;
static SemaphoreHandle_t s_mux;

typedef struct {
    bool flush_error_log;
    bool flush_stat;
    uint32_t error_log_dirty_seq;
    uint32_t stat_dirty_seq;
    nvs_error_log_t error_log;
    nvs_stat_t stat;
} nvs_flush_snapshot_t;

static nvs_flush_snapshot_t s_flush_snapshot;

static esp_err_t load_cfg_blob(void);
static void dashboard_cfg_set_defaults(ui_dashboard_cfg_t *cfg);
static void dashboard_cfg_sanitize(ui_dashboard_cfg_t *cfg);
static void dashboard_cfg_sanitize_default_page(nvs_user_cfg_t *cfg);
static void cfg_migrate_from_legacy(const legacy_nvs_user_cfg_v0_t *legacy);
static void cfg_migrate_from_v1(const legacy_nvs_user_cfg_v1_t *legacy);
static void cfg_sanitize(nvs_user_cfg_t *cfg);
static esp_err_t load_blob(const char *ns, const char *key, void *out, size_t len);
static esp_err_t load_blob_or_create(const char *ns, const char *key, void *out, size_t len);
static esp_err_t save_blob(const char *ns, const char *key, const void *data, size_t len);
static void stat_flush_task(void *arg);
static void mark_stat_dirty_locked(void);
static void mark_error_log_dirty_locked(void);
static void nvs_storage_collect_flush_snapshot_locked(nvs_flush_snapshot_t *snapshot, bool include_stat);
static void nvs_storage_commit_flush_snapshot(const nvs_flush_snapshot_t *snapshot,
                                              esp_err_t error_log_err,
                                              esp_err_t stat_err);
static void error_log_append_locked(const char *tag, esp_err_t err, const char *message);
static uint16_t ui_dashboard_gear_clamp_rpm(uint16_t rpm);
static void ui_dashboard_page_apply_gear_defaults(ui_dashboard_page_cfg_t *page);
static uint8_t ui_dashboard_gear_segment_step_index_for_rpm(uint16_t rpm_step);

#define UI_DASHBOARD_PAGE_UNSUPPORTED_MASK ((uint8_t)((1u << UI_DASHBOARD_MAX_SLOTS) - 1u))
#define UI_DASHBOARD_PAGE_TYPE_SHIFT       6u
#define UI_DASHBOARD_PAGE_TYPE_MASK        ((uint8_t)(0x3u << UI_DASHBOARD_PAGE_TYPE_SHIFT))
#define UI_DASHBOARD_GEAR_FLAG_RPM_RING    0x01u

static const uint16_t s_ui_dashboard_gear_segment_rpm_options[] = {100u, 200u, 500u, 800u, 1000u, 2000u};

/**
 * 读取 OBD 轮询模式
 *
 * 核心职责：把存储配置映射成受支持的轮询档位。
 */
uint8_t nvs_cfg_get_obd_poll_mode(const nvs_user_cfg_t *cfg)
{
    uint8_t mode = NVS_OBD_POLL_MODE_NORMAL;

    if (cfg != NULL) {
        mode = cfg->rsv[0];
    }

    if (mode >= NVS_OBD_POLL_MODE_COUNT) {
        mode = NVS_OBD_POLL_MODE_NORMAL;
    }

    return mode;
}

/** 根据轮询模式返回单个 OBD 轮询槽位的延时。 */
uint16_t nvs_cfg_get_obd_poll_slot_delay_ms(const nvs_user_cfg_t *cfg)
{
    switch (nvs_cfg_get_obd_poll_mode(cfg)) {
    case NVS_OBD_POLL_MODE_FAST:
        return 60u;
    case NVS_OBD_POLL_MODE_NORMAL:
    default:
        return 100u;
    }
}

/** 读取 RaceChrono BLE 功能模式。 */
uint8_t nvs_cfg_get_racechrono_ble_mode(const nvs_user_cfg_t *cfg)
{
    uint8_t mode = NVS_RACECHRONO_BLE_ENABLED;

    if (cfg != NULL) {
        mode = cfg->theme_cfg.rsv[0];
    }

    if (mode >= NVS_RACECHRONO_BLE_COUNT) {
        mode = NVS_RACECHRONO_BLE_ENABLED;
    }

    return mode;
}

/** 判断 RaceChrono BLE 是否启用。 */
bool nvs_cfg_is_racechrono_ble_enabled(const nvs_user_cfg_t *cfg)
{
    return nvs_cfg_get_racechrono_ble_mode(cfg) == NVS_RACECHRONO_BLE_ENABLED;
}

/** 读取油压数据工作模式。 */
uint8_t nvs_cfg_get_oil_pressure_mode(const nvs_user_cfg_t *cfg)
{
    uint8_t mode = NVS_OIL_PRESSURE_MODE_ALWAYS;

    if (cfg != NULL) {
        mode = cfg->theme_cfg.rsv[1];
    }

    if (mode >= NVS_OIL_PRESSURE_MODE_COUNT) {
        mode = NVS_OIL_PRESSURE_MODE_ALWAYS;
    }

    return mode;
}

/** 判断油压采集是否按需触发。 */
bool nvs_cfg_is_oil_pressure_demand_driven(const nvs_user_cfg_t *cfg)
{
    return nvs_cfg_get_oil_pressure_mode(cfg) == NVS_OIL_PRESSURE_MODE_DEMAND;
}

/** 读取显示旋转模式。 */
uint8_t nvs_cfg_get_display_rotation_mode(const nvs_user_cfg_t *cfg)
{
    uint8_t mode = NVS_DISPLAY_ROTATION_MODE_BOARD_DEFAULT;

    if (cfg != NULL) {
        mode = cfg->rsv[1];
    }

    if (mode >= NVS_DISPLAY_ROTATION_MODE_COUNT) {
        mode = NVS_DISPLAY_ROTATION_MODE_BOARD_DEFAULT;
    }

    return mode;
}

/** 把显示旋转模式转换成实际角度值。 */
uint16_t nvs_cfg_get_display_rotation_degrees(const nvs_user_cfg_t *cfg, uint16_t board_default_degrees)
{
    switch (nvs_cfg_get_display_rotation_mode(cfg)) {
    case NVS_DISPLAY_ROTATION_MODE_NORMAL:
        return 0u;
    case NVS_DISPLAY_ROTATION_MODE_180:
        return 180u;
    case NVS_DISPLAY_ROTATION_MODE_BOARD_DEFAULT:
    default:
        return board_default_degrees;
    }
}

/**
 * 判断某个仪表项是否适用于当前车型
 *
 * 避免把车型不支持的数据项放进仪表页，减少空值和误导展示。
 */
bool ui_dashboard_item_supported_for_vehicle(uint8_t vehicle_profile_idx, uint8_t item)
{
    const vehicle_profile_t *profile = vehicle_profile_get(vehicle_profile_idx);
    disp_item_t disp_item = (disp_item_t)item;

    if (item >= DISP_ITEM_COUNT || profile == NULL) {
        return false;
    }

    switch (disp_item) {
    case DISP_ITEM_OIL:
        return profile->oil_temp_strategy.primary != OIL_TEMP_MODE_NONE ||
               profile->oil_temp_strategy.secondary != OIL_TEMP_MODE_NONE ||
               profile->oil_temp_strategy.tertiary != OIL_TEMP_MODE_NONE;
    case DISP_ITEM_BOOST:
        return profile->has_boost;
    default:
        return true;
    }
}

/** 判断页面槽位是否被标记为车型不支持。 */
bool ui_dashboard_page_slot_is_unsupported(const ui_dashboard_page_cfg_t *page, uint8_t slot_index)
{
    if (page == NULL || slot_index >= UI_DASHBOARD_MAX_SLOTS) {
        return false;
    }

    return (page->rsv & (uint8_t)(1u << slot_index)) != 0u;
}

/** 读取仪表页类型。 */
ui_dashboard_page_type_t ui_dashboard_page_get_type(const ui_dashboard_page_cfg_t *page)
{
    uint8_t raw_type;

    if (page == NULL) {
        return UI_DASHBOARD_PAGE_TYPE_METRIC;
    }

    raw_type = (uint8_t)((page->rsv & UI_DASHBOARD_PAGE_TYPE_MASK) >> UI_DASHBOARD_PAGE_TYPE_SHIFT);
    if (raw_type >= (uint8_t)UI_DASHBOARD_PAGE_TYPE_COUNT) {
        raw_type = (uint8_t)UI_DASHBOARD_PAGE_TYPE_METRIC;
    }

    return (ui_dashboard_page_type_t)raw_type;
}

/** 设置仪表页类型并完成范围收敛。 */
void ui_dashboard_page_set_type(ui_dashboard_page_cfg_t *page, ui_dashboard_page_type_t type)
{
    if (page == NULL) {
        return;
    }

    if (type >= UI_DASHBOARD_PAGE_TYPE_COUNT) {
        type = UI_DASHBOARD_PAGE_TYPE_METRIC;
    }

    page->rsv &= (uint8_t)~UI_DASHBOARD_PAGE_TYPE_MASK;
    page->rsv |= (uint8_t)((uint8_t)type << UI_DASHBOARD_PAGE_TYPE_SHIFT);
}

/** 读取档位页红线转速。 */
uint16_t ui_dashboard_page_get_gear_redline_rpm(const ui_dashboard_page_cfg_t *page)
{
    if (page == NULL) {
        return UI_DASHBOARD_GEAR_REDLINE_RPM_DEFAULT;
    }

    return ui_dashboard_gear_clamp_rpm((uint16_t)page->gear_redline_rpm_100 * 100u);
}

/** 设置档位页红线转速，并同步修正最大转速下限。 */
void ui_dashboard_page_set_gear_redline_rpm(ui_dashboard_page_cfg_t *page, uint16_t rpm)
{
    uint16_t clamped_rpm;

    if (page == NULL) {
        return;
    }

    clamped_rpm = ui_dashboard_gear_clamp_rpm(rpm);
    page->gear_redline_rpm_100 = (uint8_t)(clamped_rpm / 100u);
    if (ui_dashboard_page_get_gear_max_rpm(page) < clamped_rpm) {
        page->gear_max_rpm_100 = page->gear_redline_rpm_100;
    }
}

/** 读取档位页最大转速。 */
uint16_t ui_dashboard_page_get_gear_max_rpm(const ui_dashboard_page_cfg_t *page)
{
    uint16_t redline;
    uint16_t max_rpm;

    if (page == NULL) {
        return UI_DASHBOARD_GEAR_MAX_RPM_DEFAULT;
    }

    redline = ui_dashboard_page_get_gear_redline_rpm(page);
    max_rpm = ui_dashboard_gear_clamp_rpm((uint16_t)page->gear_max_rpm_100 * 100u);
    if (max_rpm < redline) {
        max_rpm = redline;
    }
    return max_rpm;
}

/** 设置档位页最大转速，并确保不低于红线转速。 */
void ui_dashboard_page_set_gear_max_rpm(ui_dashboard_page_cfg_t *page, uint16_t rpm)
{
    uint16_t clamped_rpm;
    uint16_t redline;

    if (page == NULL) {
        return;
    }

    redline = ui_dashboard_page_get_gear_redline_rpm(page);
    clamped_rpm = ui_dashboard_gear_clamp_rpm(rpm);
    if (clamped_rpm < redline) {
        clamped_rpm = redline;
    }
    page->gear_max_rpm_100 = (uint8_t)(clamped_rpm / 100u);
}

/** 判断档位页是否启用转速环。 */
bool ui_dashboard_page_is_gear_rpm_ring_enabled(const ui_dashboard_page_cfg_t *page)
{
    if (page == NULL) {
        return true;
    }

    return (page->gear_flags & UI_DASHBOARD_GEAR_FLAG_RPM_RING) != 0u;
}

/** 设置档位页是否启用转速环。 */
void ui_dashboard_page_set_gear_rpm_ring_enabled(ui_dashboard_page_cfg_t *page, bool enabled)
{
    if (page == NULL) {
        return;
    }

    if (enabled) {
        page->gear_flags |= UI_DASHBOARD_GEAR_FLAG_RPM_RING;
    } else {
        page->gear_flags &= (uint8_t)~UI_DASHBOARD_GEAR_FLAG_RPM_RING;
    }
}

/** 读取档位页分段转速步进。 */
uint16_t ui_dashboard_page_get_gear_segment_rpm_step(const ui_dashboard_page_cfg_t *page)
{
    uint8_t idx = ui_dashboard_gear_segment_step_index_for_rpm(UI_DASHBOARD_GEAR_SEGMENT_RPM_DEFAULT);

    if (page != NULL) {
        idx = page->gear_segment_rpm_step_idx;
    }

    if (idx >= (sizeof(s_ui_dashboard_gear_segment_rpm_options) /
                sizeof(s_ui_dashboard_gear_segment_rpm_options[0]))) {
        idx = ui_dashboard_gear_segment_step_index_for_rpm(UI_DASHBOARD_GEAR_SEGMENT_RPM_DEFAULT);
    }

    return s_ui_dashboard_gear_segment_rpm_options[idx];
}

/** 设置档位页分段转速步进。 */
void ui_dashboard_page_set_gear_segment_rpm_step(ui_dashboard_page_cfg_t *page, uint16_t rpm_step)
{
    if (page == NULL) {
        return;
    }

    page->gear_segment_rpm_step_idx = ui_dashboard_gear_segment_step_index_for_rpm(rpm_step);
}

/**
 * 按车型整理仪表页配置
 *
 * 核心职责：标记不支持的槽位，并顺手收敛页面上的档位参数。
 */
void ui_dashboard_cfg_format_for_vehicle(ui_dashboard_cfg_t *cfg, uint8_t vehicle_profile_idx)
{
    if (cfg == NULL) {
        return;
    }

    for (uint8_t i = 0; i < UI_DASHBOARD_MAX_PAGES; ++i) {
        ui_dashboard_page_cfg_t *page = &cfg->pages[i];
        uint8_t unsupported_mask = 0u;
        ui_dashboard_page_type_t page_type = ui_dashboard_page_get_type(page);

        page->rsv &= UI_DASHBOARD_PAGE_UNSUPPORTED_MASK;
        if (i >= cfg->gauge_page_count) {
            page->rsv = 0u;
            continue;
        }

        for (uint8_t j = 0; j < page->slot_count && j < UI_DASHBOARD_MAX_SLOTS; ++j) {
            uint8_t item = (uint8_t)(page->slot_items[j] % DISP_ITEM_COUNT);
            if (!ui_dashboard_item_supported_for_vehicle(vehicle_profile_idx, item)) {
                unsupported_mask |= (uint8_t)(1u << j);
            }
        }

        page->rsv = unsupported_mask;
        ui_dashboard_page_set_type(page, page_type);
        ui_dashboard_page_set_gear_redline_rpm(page, ui_dashboard_page_get_gear_redline_rpm(page));
        ui_dashboard_page_set_gear_max_rpm(page, ui_dashboard_page_get_gear_max_rpm(page));
        ui_dashboard_page_set_gear_rpm_ring_enabled(page,
                                                    ui_dashboard_page_is_gear_rpm_ring_enabled(page));
        ui_dashboard_page_set_gear_segment_rpm_step(page,
                                                    ui_dashboard_page_get_gear_segment_rpm_step(page));
    }
}

/**
 * 初始化 NVS 存储子系统
 *
 * 负责加载配置、恢复统计数据，并启动后台刷盘任务。
 */
esp_err_t nvs_storage_init(void)
{
    BaseType_t task_created;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // ========== 数据恢复 ==========
    load_cfg_blob();
    if (load_blob_or_create(NS_STAT, KEY_STAT, &s_stat, sizeof(s_stat)) != ESP_OK) {
        ESP_LOGW(TAG, "Using in-memory default for %s/%s", NS_STAT, KEY_STAT);
    }
    if (load_blob_or_create(NS_DIAG, KEY_ERR_LOG, &s_error_log, sizeof(s_error_log)) != ESP_OK) {
        ESP_LOGW(TAG, "Using in-memory default for %s/%s", NS_DIAG, KEY_ERR_LOG);
    }
    cfg_sanitize(&s_cfg);
    nvs_error_log_logic_sanitize(&s_error_log);

    s_mux = xSemaphoreCreateMutex();
    if (s_mux == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // ========== 后台刷盘 ==========
    // 统计和错误日志走后台刷盘，避免高频更新把 flash 写放大。
    task_created = xTaskCreate(stat_flush_task,
                               "nvs_flush",
                               NVS_FLUSH_TASK_STACK_BYTES,
                               NULL,
                               4,
                               NULL);
    if (task_created != pdPASS) {
        vSemaphoreDelete(s_mux);
        s_mux = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

/** 返回当前生效的用户配置快照。 */
const nvs_user_cfg_t *nvs_cfg_get(void)
{
    return &s_cfg;
}

/**
 * 写入新的用户配置
 *
 * 仅在配置实际变化时落盘，避免无意义重复写入。
 */
esp_err_t nvs_cfg_set(const nvs_user_cfg_t *cfg)
{
    if (!cfg) {
        return ESP_ERR_INVALID_ARG;
    }
    if (memcmp(cfg, &s_cfg, sizeof(s_cfg)) == 0) {
        return ESP_OK;
    }
    s_cfg = *cfg;
    cfg_sanitize(&s_cfg);
    return save_blob(NS_CFG, KEY_CFG, &s_cfg, sizeof(s_cfg));
}

/** 返回当前运行统计结构。 */
const nvs_stat_t *nvs_stat_get(void)
{
    return &s_stat;
}

/** 增加总里程计数。 */
void nvs_stat_add_odometer(uint32_t delta_m)
{
    xSemaphoreTake(s_mux, portMAX_DELAY);
    s_stat.odometer_m += delta_m;
    mark_stat_dirty_locked();
    xSemaphoreGive(s_mux);
}

/** 增加累计运行时长。 */
void nvs_stat_add_runtime(uint32_t delta_s)
{
    xSemaphoreTake(s_mux, portMAX_DELAY);
    s_stat.run_time_s += delta_s;
    mark_stat_dirty_locked();
    xSemaphoreGive(s_mux);
}

/** 增加单次 trip 里程。 */
void nvs_stat_add_trip(uint32_t delta_m)
{
    xSemaphoreTake(s_mux, portMAX_DELAY);
    s_stat.trip_m += delta_m;
    mark_stat_dirty_locked();
    xSemaphoreGive(s_mux);
}

/**
 * 按速度和采样间隔更新里程统计
 *
 * 核心职责：把瞬时速度换算成距离、运行时长和平均速度。
 */
void nvs_stat_update_speed(uint8_t speed_kmh, uint32_t dt_ms)
{
    if (dt_ms < 1000 || speed_kmh == 0) {
        return;
    }

    xSemaphoreTake(s_mux, portMAX_DELAY);

    double dist_m = ((double)speed_kmh * (double)dt_ms) / 3.6e3;
    s_stat.odometer_m += (uint64_t)dist_m;
    s_stat.trip_m += (uint64_t)dist_m;
    s_stat.run_time_s += dt_ms / 1000;
    s_stat.trip_run_time_s += dt_ms / 1000;

    if (speed_kmh > s_stat.max_speed_kmh) {
        s_stat.max_speed_kmh = speed_kmh;
    }

    if (s_stat.trip_run_time_s) {
        double avg_ms = (double)s_stat.trip_m / (double)s_stat.trip_run_time_s;
        s_stat.avg_speed_kmh = (uint16_t)(avg_ms * 3.6 + 0.5);
        if (s_stat.avg_speed_kmh > s_stat.max_speed_kmh) {
            s_stat.avg_speed_kmh = s_stat.max_speed_kmh;
        }
    }

    mark_stat_dirty_locked();
    xSemaphoreGive(s_mux);
}

/** 重置 trip 统计。 */
void nvs_stat_reset_trip(void)
{
    xSemaphoreTake(s_mux, portMAX_DELAY);
    s_stat.trip_m = 0;
    s_stat.max_speed_kmh = 0;
    s_stat.avg_speed_kmh = 0;
    s_stat.trip_run_time_s = 0;
    mark_stat_dirty_locked();
    xSemaphoreGive(s_mux);
}

/** 复制一份当前里程统计，供日志或 UI 安全读取。 */
nvs_stat_t nvs_stat_get_mileage(void)
{
    xSemaphoreTake(s_mux, portMAX_DELAY);
    nvs_stat_t stat = s_stat;
    xSemaphoreGive(s_mux);
    return stat;
}

/**
 * 记录一条错误日志
 *
 * 即使互斥锁尚未初始化，也尽量把早期启动错误保留下来。
 */
void nvs_error_log_record(const char *tag, esp_err_t err, const char *message)
{
    if (s_mux == NULL) {
        error_log_append_locked(tag, err, message);
        return;
    }

    xSemaphoreTake(s_mux, portMAX_DELAY);
    error_log_append_locked(tag, err, message);
    xSemaphoreGive(s_mux);
}

/** 返回当前错误日志条目数。 */
uint8_t nvs_error_log_count(void)
{
    uint8_t count;

    if (s_mux == NULL) {
        return s_error_log.count;
    }

    xSemaphoreTake(s_mux, portMAX_DELAY);
    count = s_error_log.count;
    xSemaphoreGive(s_mux);
    return count;
}

/** 复制当前错误日志内容。 */
void nvs_error_log_copy(nvs_error_log_t *out)
{
    if (out == NULL) {
        return;
    }

    if (s_mux == NULL) {
        *out = s_error_log;
        return;
    }

    xSemaphoreTake(s_mux, portMAX_DELAY);
    *out = s_error_log;
    xSemaphoreGive(s_mux);
}

/**
 * NVS 后台刷盘任务
 *
 * 以较高频率处理错误日志、较低频率处理统计数据，平衡可靠性和 flash 寿命。
 */
static void stat_flush_task(void *arg)
{
    (void)arg;
    uint32_t stat_elapsed_ms = 0u;
    nvs_flush_snapshot_t *snapshot = &s_flush_snapshot;

    while (1) {
        esp_err_t error_log_err = ESP_OK;
        esp_err_t stat_err = ESP_OK;

        vTaskDelay(pdMS_TO_TICKS(ERROR_FLUSH_PERIOD_MS));
        stat_elapsed_ms += ERROR_FLUSH_PERIOD_MS;
        xSemaphoreTake(s_mux, portMAX_DELAY);
        // 先抓快照再解锁，避免实际写 flash 时长时间占住共享状态。
        nvs_storage_collect_flush_snapshot_locked(snapshot, stat_elapsed_ms >= STAT_FLUSH_PERIOD_MS);
        if (stat_elapsed_ms >= STAT_FLUSH_PERIOD_MS) {
            stat_elapsed_ms = 0u;
        }
        xSemaphoreGive(s_mux);

        if (snapshot->flush_error_log) {
            error_log_err = save_blob(NS_DIAG, KEY_ERR_LOG, &snapshot->error_log, sizeof(snapshot->error_log));
        }
        if (snapshot->flush_stat) {
            stat_err = save_blob(NS_STAT, KEY_STAT, &snapshot->stat, sizeof(snapshot->stat));
        }

        nvs_storage_commit_flush_snapshot(snapshot, error_log_err, stat_err);
    }
}

/**
 * 从 NVS 加载主配置 blob
 *
 * 负责识别旧版本结构，并在需要时执行迁移。
 */
static esp_err_t load_cfg_blob(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NS_CFG, NVS_READONLY, &h);
    if (err != ESP_OK) {
        cfg_sanitize(&s_cfg);
        return save_blob(NS_CFG, KEY_CFG, &s_cfg, sizeof(s_cfg));
    }

    size_t size = 0;
    err = nvs_get_blob(h, KEY_CFG, NULL, &size);
    if (err == ESP_OK && size == sizeof(s_cfg)) {
        size_t read_size = sizeof(s_cfg);
        err = nvs_get_blob(h, KEY_CFG, &s_cfg, &read_size);
        nvs_close(h);
        return err;
    }

    if (err == ESP_OK && size == sizeof(legacy_nvs_user_cfg_v0_t)) {
        legacy_nvs_user_cfg_v0_t legacy = {0};
        size_t read_size = sizeof(legacy);
        err = nvs_get_blob(h, KEY_CFG, &legacy, &read_size);
        nvs_close(h);
        if (err == ESP_OK) {
            cfg_migrate_from_legacy(&legacy);
            cfg_sanitize(&s_cfg);
            return save_blob(NS_CFG, KEY_CFG, &s_cfg, sizeof(s_cfg));
        }
        return err;
    }

    if (err == ESP_OK && size == sizeof(legacy_nvs_user_cfg_v1_t)) {
        legacy_nvs_user_cfg_v1_t legacy = {0};
        size_t read_size = sizeof(legacy);
        err = nvs_get_blob(h, KEY_CFG, &legacy, &read_size);
        nvs_close(h);
        if (err == ESP_OK) {
            cfg_migrate_from_v1(&legacy);
            cfg_sanitize(&s_cfg);
            return save_blob(NS_CFG, KEY_CFG, &s_cfg, sizeof(s_cfg));
        }
        return err;
    }

    nvs_close(h);
    cfg_sanitize(&s_cfg);
    return save_blob(NS_CFG, KEY_CFG, &s_cfg, sizeof(s_cfg));
}

/** 填充默认仪表页配置。 */
static void dashboard_cfg_set_defaults(ui_dashboard_cfg_t *cfg)
{
    if (!cfg) {
        return;
    }

    memset(cfg, 0, sizeof(*cfg));
    cfg->version = UI_DASHBOARD_CFG_VERSION;
    cfg->gauge_page_count = 1;
    cfg->pages[0].slot_count = 3;
    cfg->pages[0].slot_items[0] = 5; /* DISP_ITEM_RPM */
    cfg->pages[0].slot_items[1] = 6; /* DISP_ITEM_SPEED */
    cfg->pages[0].slot_items[2] = 2; /* DISP_ITEM_OIL */
    ui_dashboard_page_apply_gear_defaults(&cfg->pages[0]);
}

/** 清洗仪表页配置，保证页数和槽位都落在合法范围内。 */
static void dashboard_cfg_sanitize(ui_dashboard_cfg_t *cfg)
{
    if (!cfg) {
        return;
    }

    if (cfg->version != UI_DASHBOARD_CFG_VERSION) {
        dashboard_cfg_set_defaults(cfg);
        return;
    }

    if (cfg->gauge_page_count > UI_DASHBOARD_MAX_PAGES) {
        cfg->gauge_page_count = UI_DASHBOARD_MAX_PAGES;
    }

    for (uint8_t i = 0; i < cfg->gauge_page_count; ++i) {
        ui_dashboard_page_cfg_t *page = &cfg->pages[i];
        if (page->slot_count < 1 || page->slot_count > UI_DASHBOARD_MAX_SLOTS) {
            page->slot_count = 1;
        }
        for (uint8_t j = 0; j < UI_DASHBOARD_MAX_SLOTS; ++j) {
            if (page->slot_items[j] > 10) {
                page->slot_items[j] = 5; /* DISP_ITEM_RPM */
            }
        }
        page->rsv &= (uint8_t)(UI_DASHBOARD_PAGE_UNSUPPORTED_MASK | UI_DASHBOARD_PAGE_TYPE_MASK);
        ui_dashboard_page_set_type(page, ui_dashboard_page_get_type(page));
        ui_dashboard_page_set_gear_redline_rpm(page, ui_dashboard_page_get_gear_redline_rpm(page));
        ui_dashboard_page_set_gear_max_rpm(page, ui_dashboard_page_get_gear_max_rpm(page));
        ui_dashboard_page_set_gear_rpm_ring_enabled(page,
                                                    ui_dashboard_page_is_gear_rpm_ring_enabled(page));
        ui_dashboard_page_set_gear_segment_rpm_step(page,
                                                    ui_dashboard_page_get_gear_segment_rpm_step(page));
    }
}

/** 修正默认页索引，避免指向不存在的仪表页。 */
static void dashboard_cfg_sanitize_default_page(nvs_user_cfg_t *cfg)
{
    if (!cfg) {
        return;
    }

    if (cfg->default_page == NVS_DEFAULT_PAGE_MENU) {
        return;
    }

    if (cfg->dashboard_cfg.gauge_page_count == 0u ||
        cfg->default_page > cfg->dashboard_cfg.gauge_page_count) {
        cfg->default_page = NVS_DEFAULT_PAGE_MENU;
    }
}

/** 从最早版本配置迁移到当前结构。 */
static void cfg_migrate_from_legacy(const legacy_nvs_user_cfg_v0_t *legacy)
{
    if (!legacy) {
        return;
    }

    s_cfg.protocol = legacy->protocol;
    s_cfg.theme_cfg = legacy->theme_cfg;
    memcpy(s_cfg.ble_device_name, legacy->ble_device_name, sizeof(s_cfg.ble_device_name));
    s_cfg.default_page = legacy->default_page;
    s_cfg.brightness_day = legacy->brightness_day;
    s_cfg.vehicle_profile_idx = legacy->vehicle_profile_idx;
    s_cfg.brake_temp_warn_c = legacy->brake_temp_warn_c;
    s_cfg.oil_pressure_warn_x10 = legacy->oil_pressure_warn_x10;
    memcpy(s_cfg.temp_display_map, legacy->temp_display_map, sizeof(s_cfg.temp_display_map));
    memcpy(s_cfg.info_display_map, legacy->info_display_map, sizeof(s_cfg.info_display_map));
    s_cfg.needle_source_idx = legacy->needle_source_idx;
    memcpy(s_cfg.rsv, legacy->rsv, sizeof(s_cfg.rsv));
    dashboard_cfg_set_defaults(&s_cfg.dashboard_cfg);
}

/** 从 v1 配置迁移到当前结构。 */
static void cfg_migrate_from_v1(const legacy_nvs_user_cfg_v1_t *legacy)
{
    if (legacy == NULL) {
        return;
    }

    s_cfg.protocol = legacy->protocol;
    s_cfg.theme_cfg = legacy->theme_cfg;
    memcpy(s_cfg.ble_device_name, legacy->ble_device_name, sizeof(s_cfg.ble_device_name));
    s_cfg.default_page = legacy->default_page;
    s_cfg.brightness_day = legacy->brightness_day;
    s_cfg.vehicle_profile_idx = legacy->vehicle_profile_idx;
    s_cfg.brake_temp_warn_c = legacy->brake_temp_warn_c;
    s_cfg.oil_pressure_warn_x10 = legacy->oil_pressure_warn_x10;
    memcpy(s_cfg.temp_display_map, legacy->temp_display_map, sizeof(s_cfg.temp_display_map));
    memcpy(s_cfg.info_display_map, legacy->info_display_map, sizeof(s_cfg.info_display_map));
    s_cfg.needle_source_idx = legacy->needle_source_idx;
    memcpy(s_cfg.rsv, legacy->rsv, sizeof(s_cfg.rsv));

    memset(&s_cfg.dashboard_cfg, 0, sizeof(s_cfg.dashboard_cfg));
    s_cfg.dashboard_cfg.version = UI_DASHBOARD_CFG_VERSION;
    s_cfg.dashboard_cfg.gauge_page_count = legacy->dashboard_cfg.gauge_page_count;
    memcpy(s_cfg.dashboard_cfg.rsv, legacy->dashboard_cfg.rsv, sizeof(s_cfg.dashboard_cfg.rsv));
    for (uint8_t i = 0; i < UI_DASHBOARD_MAX_PAGES; ++i) {
        s_cfg.dashboard_cfg.pages[i].slot_count = legacy->dashboard_cfg.pages[i].slot_count;
        memcpy(s_cfg.dashboard_cfg.pages[i].slot_items,
               legacy->dashboard_cfg.pages[i].slot_items,
               sizeof(s_cfg.dashboard_cfg.pages[i].slot_items));
        s_cfg.dashboard_cfg.pages[i].rsv = legacy->dashboard_cfg.pages[i].rsv;
        ui_dashboard_page_apply_gear_defaults(&s_cfg.dashboard_cfg.pages[i]);
    }
}

/**
 * 清洗主配置
 *
 * 把越界、缺失或历史遗留值收敛回当前固件可以稳定处理的范围。
 */
static void cfg_sanitize(nvs_user_cfg_t *cfg)
{
    static const uint8_t def_info_map[5] = {0, 2, 3, 4, 1};

    if (!cfg) {
        return;
    }

    if (cfg->brightness_day == 0) {
        cfg->brightness_day = 100;
    }
    if (cfg->default_page >= NVS_DEFAULT_PAGE_COUNT) {
        cfg->default_page = NVS_DEFAULT_PAGE_MENU;
    }
    if (cfg->needle_source_idx >= 10) {
        cfg->needle_source_idx = 0;
    }

    uint8_t vehicle_count = 0;
    vehicle_profile_get_all(&vehicle_count);
    if (vehicle_count > 0 && cfg->vehicle_profile_idx >= vehicle_count) {
        cfg->vehicle_profile_idx = 0;
    }
    if (cfg->brake_temp_warn_c < 10 || cfg->brake_temp_warn_c > 1200) {
        cfg->brake_temp_warn_c = 600;
    }
    if (cfg->oil_pressure_warn_x10 > 100) {
        cfg->oil_pressure_warn_x10 = 80;
    }
    if (cfg->rsv[0] >= NVS_OBD_POLL_MODE_COUNT) {
        cfg->rsv[0] = NVS_OBD_POLL_MODE_NORMAL;
    }
    if (cfg->rsv[1] >= NVS_DISPLAY_ROTATION_MODE_COUNT) {
        cfg->rsv[1] = NVS_DISPLAY_ROTATION_MODE_BOARD_DEFAULT;
    }

    for (int i = 0; i < 3; ++i) {
        if (cfg->temp_display_map[i] > 9) {
            cfg->temp_display_map[i] = (uint8_t)i;
        }
    }
    for (int i = 0; i < 5; ++i) {
        if (cfg->info_display_map[i] > 9) {
            cfg->info_display_map[i] = def_info_map[i];
        }
    }

    dashboard_cfg_sanitize(&cfg->dashboard_cfg);
    ui_dashboard_cfg_format_for_vehicle(&cfg->dashboard_cfg, cfg->vehicle_profile_idx);
    dashboard_cfg_sanitize_default_page(cfg);
}

/** 把档位页转速值收敛到支持的范围和精度。 */
static uint16_t ui_dashboard_gear_clamp_rpm(uint16_t rpm)
{
    if (rpm < UI_DASHBOARD_GEAR_REDLINE_RPM_MIN) {
        return UI_DASHBOARD_GEAR_REDLINE_RPM_MIN;
    }
    if (rpm > UI_DASHBOARD_GEAR_REDLINE_RPM_MAX) {
        return UI_DASHBOARD_GEAR_REDLINE_RPM_MAX;
    }
    return (uint16_t)((rpm / 100u) * 100u);
}

/** 为档位页填充默认转速参数。 */
static void ui_dashboard_page_apply_gear_defaults(ui_dashboard_page_cfg_t *page)
{
    if (page == NULL) {
        return;
    }

    page->gear_redline_rpm_100 = (uint8_t)(UI_DASHBOARD_GEAR_REDLINE_RPM_DEFAULT / 100u);
    page->gear_max_rpm_100 = (uint8_t)(UI_DASHBOARD_GEAR_MAX_RPM_DEFAULT / 100u);
    page->gear_flags = UI_DASHBOARD_GEAR_FLAG_RPM_RING;
    page->gear_segment_rpm_step_idx =
        ui_dashboard_gear_segment_step_index_for_rpm(UI_DASHBOARD_GEAR_SEGMENT_RPM_DEFAULT);
}

/** 把转速步进值映射成配置里保存的索引。 */
static uint8_t ui_dashboard_gear_segment_step_index_for_rpm(uint16_t rpm_step)
{
    for (uint8_t i = 0; i < (sizeof(s_ui_dashboard_gear_segment_rpm_options) /
                             sizeof(s_ui_dashboard_gear_segment_rpm_options[0]));
         ++i) {
        if (s_ui_dashboard_gear_segment_rpm_options[i] == rpm_step) {
            return i;
        }
    }

    return 2u;
}

/**
 * 从指定命名空间读取固定长度 blob
 *
 * 长度不匹配时直接拒绝加载，避免把旧结构硬套进新结构。
 */
static esp_err_t load_blob(const char *ns, const char *key, void *out, size_t len)
{
    nvs_handle_t h;
    esp_err_t err;
    size_t size = 0u;

    if (out == NULL || len == 0u) {
        return ESP_ERR_INVALID_ARG;
    }

    err = nvs_open(ns, NVS_READONLY, &h);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_get_blob(h, key, NULL, &size);
    if (err != ESP_OK) {
        nvs_close(h);
        return err;
    }

    if (size != len) {
        ESP_LOGW(TAG,
                 "Skip loading %s/%s due to blob size mismatch: stored=%u expected=%u",
                 ns,
                 key,
                 (unsigned)size,
                 (unsigned)len);
        nvs_close(h);
        return ESP_FAIL;
    }

    size = len;
    err = nvs_get_blob(h, key, out, &size);
    nvs_close(h);
    return err;
}

static esp_err_t load_blob_or_create(const char *ns, const char *key, void *out, size_t len)
{
    // 仅在缺失时创建默认值，避免把其他读取失败误判成可以自动修复的场景。
    esp_err_t err = load_blob(ns, key, out, len);

    if (err == ESP_OK) {
        return ESP_OK;
    }

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return save_blob(ns, key, out, len);
    }

    ESP_LOGW(TAG, "Preserving existing default for %s/%s after load failure: %s", ns, key, esp_err_to_name(err));
    return err;
}

static esp_err_t save_blob(const char *ns, const char *key, const void *data, size_t len)
{
    // 写入和提交必须成对出现，否则掉电后可能只完成缓存写入却没有真正落盘。
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_blob(h, key, data, len);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return err;
}

/** 标记统计数据已脏，并推进脏序号。 */
static void mark_stat_dirty_locked(void)
{
    s_stat_dirty = true;
    s_stat_dirty_seq++;
}

/** 标记错误日志已脏，并推进脏序号。 */
static void mark_error_log_dirty_locked(void)
{
    s_error_log_dirty = true;
    s_error_log_dirty_seq++;
}

/**
 * 采集一次待刷盘快照
 *
 * 把当前脏数据复制出来，供后台线程在无锁状态下慢慢写 flash。
 */
static void nvs_storage_collect_flush_snapshot_locked(nvs_flush_snapshot_t *snapshot, bool include_stat)
{
    if (snapshot == NULL) {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    if (s_error_log_dirty) {
        snapshot->flush_error_log = true;
        snapshot->error_log_dirty_seq = s_error_log_dirty_seq;
        snapshot->error_log = s_error_log;
    }
    if (include_stat && s_stat_dirty) {
        snapshot->flush_stat = true;
        snapshot->stat_dirty_seq = s_stat_dirty_seq;
        snapshot->stat = s_stat;
    }
}

/**
 * 根据刷盘结果回写脏标记
 *
 * 只有序号仍然一致时才清脏，避免把刷盘期间的新修改误判成已落盘。
 */
static void nvs_storage_commit_flush_snapshot(const nvs_flush_snapshot_t *snapshot,
                                              esp_err_t error_log_err,
                                              esp_err_t stat_err)
{
    if (snapshot == NULL || s_mux == NULL) {
        return;
    }

    xSemaphoreTake(s_mux, portMAX_DELAY);
    if (snapshot->flush_error_log &&
        error_log_err == ESP_OK &&
        s_error_log_dirty_seq == snapshot->error_log_dirty_seq) {
        s_error_log_dirty = false;
    }
    if (snapshot->flush_stat &&
        stat_err == ESP_OK &&
        s_stat_dirty_seq == snapshot->stat_dirty_seq) {
        s_stat_dirty = false;
    }
    xSemaphoreGive(s_mux);
}

/** 在持锁状态下向错误日志追加一条记录。 */
static void error_log_append_locked(const char *tag, esp_err_t err, const char *message)
{
    nvs_error_log_logic_append(&s_error_log,
                               (uint32_t)(esp_timer_get_time() / 1000000LL),
                               (int32_t)err,
                               tag,
                               message);
    mark_error_log_dirty_locked();
}
