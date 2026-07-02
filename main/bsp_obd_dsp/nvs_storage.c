#include "nvs_storage.h"

#include <string.h>

#include "app_obd_dsp/vehicle_profiles.h"
#include "esp_log.h"
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
#define STAT_FLUSH_PERIOD_MS 30000

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
static bool s_stat_dirty = false;
static SemaphoreHandle_t s_mux;

static esp_err_t load_cfg_blob(void);
static void dashboard_cfg_set_defaults(ui_dashboard_cfg_t *cfg);
static void dashboard_cfg_sanitize(ui_dashboard_cfg_t *cfg);
static void dashboard_cfg_sanitize_default_page(nvs_user_cfg_t *cfg);
static void cfg_migrate_from_legacy(const legacy_nvs_user_cfg_v0_t *legacy);
static void cfg_sanitize(nvs_user_cfg_t *cfg);
static esp_err_t load_blob(const char *ns, const char *key, void *out, size_t len);
static esp_err_t save_blob(const char *ns, const char *key, const void *data, size_t len);
static void stat_flush_task(void *arg);

#define UI_DASHBOARD_PAGE_UNSUPPORTED_MASK ((uint8_t)((1u << UI_DASHBOARD_MAX_SLOTS) - 1u))
#define UI_DASHBOARD_PAGE_TYPE_SHIFT       6u
#define UI_DASHBOARD_PAGE_TYPE_MASK        ((uint8_t)(0x3u << UI_DASHBOARD_PAGE_TYPE_SHIFT))

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

bool ui_dashboard_page_slot_is_unsupported(const ui_dashboard_page_cfg_t *page, uint8_t slot_index)
{
    if (page == NULL || slot_index >= UI_DASHBOARD_MAX_SLOTS) {
        return false;
    }

    return (page->rsv & (uint8_t)(1u << slot_index)) != 0u;
}

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
    }
}

esp_err_t nvs_storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    load_cfg_blob();
    load_blob(NS_STAT, KEY_STAT, &s_stat, sizeof(s_stat));
    cfg_sanitize(&s_cfg);

    s_mux = xSemaphoreCreateMutex();
    xTaskCreate(stat_flush_task, "nvs_flush", 2048, NULL, 4, NULL);
    return ESP_OK;
}

const nvs_user_cfg_t *nvs_cfg_get(void)
{
    return &s_cfg;
}

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

const nvs_stat_t *nvs_stat_get(void)
{
    return &s_stat;
}

void nvs_stat_add_odometer(uint32_t delta_m)
{
    xSemaphoreTake(s_mux, portMAX_DELAY);
    s_stat.odometer_m += delta_m;
    s_stat_dirty = true;
    xSemaphoreGive(s_mux);
}

void nvs_stat_add_runtime(uint32_t delta_s)
{
    xSemaphoreTake(s_mux, portMAX_DELAY);
    s_stat.run_time_s += delta_s;
    s_stat_dirty = true;
    xSemaphoreGive(s_mux);
}

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

    s_stat_dirty = true;
    xSemaphoreGive(s_mux);
}

void nvs_stat_reset_trip(void)
{
    xSemaphoreTake(s_mux, portMAX_DELAY);
    s_stat.trip_m = 0;
    s_stat.max_speed_kmh = 0;
    s_stat.avg_speed_kmh = 0;
    s_stat.trip_run_time_s = 0;
    s_stat_dirty = true;
    xSemaphoreGive(s_mux);
}

nvs_stat_t nvs_stat_get_mileage(void)
{
    xSemaphoreTake(s_mux, portMAX_DELAY);
    nvs_stat_t stat = s_stat;
    xSemaphoreGive(s_mux);
    return stat;
}

static void stat_flush_task(void *arg)
{
    (void)arg;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(STAT_FLUSH_PERIOD_MS));
        if (!s_stat_dirty) {
            continue;
        }
        xSemaphoreTake(s_mux, portMAX_DELAY);
        if (save_blob(NS_STAT, KEY_STAT, &s_stat, sizeof(s_stat)) == ESP_OK) {
            s_stat_dirty = false;
        }
        xSemaphoreGive(s_mux);
    }
}

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

    nvs_close(h);
    cfg_sanitize(&s_cfg);
    return save_blob(NS_CFG, KEY_CFG, &s_cfg, sizeof(s_cfg));
}

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
}

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
    }
}

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

static esp_err_t load_blob(const char *ns, const char *key, void *out, size_t len)
{
    nvs_handle_t h;
    size_t size = len;
    esp_err_t err;

    if (nvs_open(ns, NVS_READONLY, &h) == ESP_OK) {
        err = nvs_get_blob(h, key, out, &size);
        nvs_close(h);
        if (err == ESP_OK && size == len) {
            return ESP_OK;
        }
    }

    memset(out, 0, len);
    return save_blob(ns, key, out, len);
}

static esp_err_t save_blob(const char *ns, const char *key, const void *data, size_t len)
{
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
