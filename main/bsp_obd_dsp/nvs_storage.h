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

#define UI_DASHBOARD_GEAR_REDLINE_RPM_MIN     1500u
#define UI_DASHBOARD_GEAR_REDLINE_RPM_MAX     9900u
#define UI_DASHBOARD_GEAR_REDLINE_RPM_DEFAULT 7000u
#define UI_DASHBOARD_GEAR_MAX_RPM_DEFAULT     8000u
#define UI_DASHBOARD_GEAR_SEGMENT_RPM_DEFAULT 500u

typedef enum {
    UI_DASHBOARD_PAGE_TYPE_METRIC = 0u,
    UI_DASHBOARD_PAGE_TYPE_GEAR_DERIVED = 1u,
    UI_DASHBOARD_PAGE_TYPE_G_FORCE_OBD = 2u,
    UI_DASHBOARD_PAGE_TYPE_G_FORCE_ESP32 = 3u,
    UI_DASHBOARD_PAGE_TYPE_GEAR_MONITOR = 4u,
    UI_DASHBOARD_PAGE_TYPE_COUNT
} ui_dashboard_page_type_t;

typedef struct {
    uint8_t slot_count;
    uint8_t slot_items[UI_DASHBOARD_MAX_SLOTS];
    uint8_t rsv; /* slot unsupported bitmask + page type bits */
    uint8_t gear_redline_rpm_100;
    uint8_t gear_max_rpm_100;
    uint8_t gear_flags;
    uint8_t gear_segment_rpm_step_idx;
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

#define NVS_OBD_POLL_MODE_NORMAL 0u
#define NVS_OBD_POLL_MODE_FAST   1u
#define NVS_OBD_POLL_MODE_TURBO  2u
#define NVS_OBD_POLL_MODE_COUNT  3u

#define NVS_RACECHRONO_BLE_ENABLED  0u
#define NVS_RACECHRONO_BLE_DISABLED 1u
#define NVS_RACECHRONO_BLE_COUNT    2u

#define NVS_OIL_PRESSURE_MODE_ALWAYS 0u
#define NVS_OIL_PRESSURE_MODE_DEMAND 1u
#define NVS_OIL_PRESSURE_MODE_COUNT  2u

#define NVS_DISPLAY_ROTATION_MODE_BOARD_DEFAULT 0u
#define NVS_DISPLAY_ROTATION_MODE_NORMAL        1u
#define NVS_DISPLAY_ROTATION_MODE_180          2u
#define NVS_DISPLAY_ROTATION_MODE_COUNT        3u

#define NVS_ERROR_LOG_VERSION  1u
#define NVS_ERROR_LOG_CAPACITY 20u
#define NVS_ERROR_TAG_LEN      16u
#define NVS_ERROR_MSG_LEN      64u

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

typedef struct {
    uint32_t seq;
    uint32_t uptime_s;
    int32_t err_code;
    char tag[NVS_ERROR_TAG_LEN];
    char message[NVS_ERROR_MSG_LEN];
} nvs_error_entry_t;

typedef struct {
    uint32_t version;
    uint32_t next_seq;
    uint8_t head;
    uint8_t count;
    uint8_t rsv[2];
    nvs_error_entry_t entries[NVS_ERROR_LOG_CAPACITY];
} nvs_error_log_t;

esp_err_t nvs_storage_init(void);

const nvs_user_cfg_t *nvs_cfg_get(void);
esp_err_t nvs_cfg_set(const nvs_user_cfg_t *cfg);
uint8_t nvs_cfg_get_obd_poll_mode(const nvs_user_cfg_t *cfg);
uint16_t nvs_cfg_get_obd_poll_slot_delay_ms(const nvs_user_cfg_t *cfg);
uint8_t nvs_cfg_get_racechrono_ble_mode(const nvs_user_cfg_t *cfg);
bool nvs_cfg_is_racechrono_ble_enabled(const nvs_user_cfg_t *cfg);
uint8_t nvs_cfg_get_oil_pressure_mode(const nvs_user_cfg_t *cfg);
bool nvs_cfg_is_oil_pressure_demand_driven(const nvs_user_cfg_t *cfg);
uint8_t nvs_cfg_get_display_rotation_mode(const nvs_user_cfg_t *cfg);
uint16_t nvs_cfg_get_display_rotation_degrees(const nvs_user_cfg_t *cfg, uint16_t board_default_degrees);
bool ui_dashboard_item_supported_for_vehicle(uint8_t vehicle_profile_idx, uint8_t item);
void ui_dashboard_cfg_format_for_vehicle(ui_dashboard_cfg_t *cfg, uint8_t vehicle_profile_idx);
bool ui_dashboard_page_slot_is_unsupported(const ui_dashboard_page_cfg_t *page, uint8_t slot_index);
ui_dashboard_page_type_t ui_dashboard_page_get_type(const ui_dashboard_page_cfg_t *page);
void ui_dashboard_page_set_type(ui_dashboard_page_cfg_t *page, ui_dashboard_page_type_t type);
uint16_t ui_dashboard_page_get_gear_redline_rpm(const ui_dashboard_page_cfg_t *page);
void ui_dashboard_page_set_gear_redline_rpm(ui_dashboard_page_cfg_t *page, uint16_t rpm);
uint16_t ui_dashboard_page_get_gear_max_rpm(const ui_dashboard_page_cfg_t *page);
void ui_dashboard_page_set_gear_max_rpm(ui_dashboard_page_cfg_t *page, uint16_t rpm);
bool ui_dashboard_page_is_gear_rpm_ring_enabled(const ui_dashboard_page_cfg_t *page);
void ui_dashboard_page_set_gear_rpm_ring_enabled(ui_dashboard_page_cfg_t *page, bool enabled);
uint16_t ui_dashboard_page_get_gear_segment_rpm_step(const ui_dashboard_page_cfg_t *page);
void ui_dashboard_page_set_gear_segment_rpm_step(ui_dashboard_page_cfg_t *page, uint16_t rpm_step);

const nvs_stat_t *nvs_stat_get(void);
void nvs_stat_add_odometer(uint32_t delta_m);
void nvs_stat_add_runtime(uint32_t delta_s);
void nvs_stat_add_trip(uint32_t delta_m);
void nvs_stat_reset_trip(void);
void nvs_stat_update_speed(uint8_t speed_kmh, uint32_t dt_ms);
nvs_stat_t nvs_stat_get_mileage(void);

void nvs_error_log_record(const char *tag, esp_err_t err, const char *message);
uint8_t nvs_error_log_count(void);
void nvs_error_log_copy(nvs_error_log_t *out);
