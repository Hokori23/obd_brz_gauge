#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "app_obd_dsp/default_page_ids.h"
#include "esp_err.h"

typedef struct {
    uint8_t theme;
    uint32_t user_theme_domiant_color;
    uint32_t user_theme_secondary_color;
    uint8_t rsv[5];
} theme_cfg_t;

#define UI_DASHBOARD_CFG_VERSION 1u
#define UI_DASHBOARD_MAX_PAGES   8u
#define UI_DASHBOARD_MAX_SLOTS   6u

typedef struct {
    uint8_t slot_count;
    uint8_t slot_items[UI_DASHBOARD_MAX_SLOTS];
    uint8_t rsv;
} ui_dashboard_page_cfg_t;

typedef struct {
    uint8_t version;
    uint8_t gauge_page_count;
    uint8_t rsv[2];
    ui_dashboard_page_cfg_t pages[UI_DASHBOARD_MAX_PAGES];
} ui_dashboard_cfg_t;

#define NVS_DEFAULT_PAGE_MENU          DEFAULT_PAGE_MENU
#define NVS_DEFAULT_PAGE_GAUGE_1       DEFAULT_PAGE_GAUGE_1
#define NVS_DEFAULT_PAGE_COUNT         DEFAULT_PAGE_COUNT

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
    ui_dashboard_cfg_t dashboard_cfg;
} nvs_user_cfg_t;

typedef struct {
    uint64_t odometer_m;
    uint64_t trip_m;
    uint64_t run_time_s;
    uint16_t max_speed_kmh;
    uint16_t avg_speed_kmh;
    uint32_t trip_run_time_s;
    uint8_t rsv[2];
} nvs_stat_t;

esp_err_t nvs_storage_init(void);

const nvs_user_cfg_t *nvs_cfg_get(void);
esp_err_t nvs_cfg_set(const nvs_user_cfg_t *cfg);

const nvs_stat_t *nvs_stat_get(void);
void nvs_stat_add_odometer(uint32_t delta_m);
void nvs_stat_add_runtime(uint32_t delta_s);
void nvs_stat_add_trip(uint32_t delta_m);
void nvs_stat_reset_trip(void);
void nvs_stat_update_speed(uint8_t speed_kmh, uint32_t dt_ms);
nvs_stat_t nvs_stat_get_mileage(void);
