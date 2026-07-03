#include "app_bootstrap.h"

#include <inttypes.h>

#include "app_obd_dsp/aux_sensor_demand.h"
#include "app_obd_dsp/obd_data_cache.h"
#include "app_obd_dsp/vehicle_profiles.h"
#include "bsp_obd_dsp/ads1115_oil_pressure.h"
#include "bsp_obd_dsp/boards/board_api.h"
#include "bsp_obd_dsp/elm327_ble_client.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "bsp_obd_dsp/qmi8658_gforce.h"
#include "bsp_obd_dsp/racechrono_ble_diy.h"
#include "bsp_obd_dsp/rs485_brake_temp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "export_path/ui_platform.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app_bootstrap";

/**
 * Initialize persisted state and select the active vehicle profile.
 *
 * Core responsibilities: load NVS data, log startup state,
 * and align runtime profile selection with saved settings.
 */
void app_bootstrap_init_storage_and_profile(void)
{
    ESP_LOGI(TAG, "bootstrap: init nvs");
    nvs_storage_init();
    ESP_LOGI(TAG, "bootstrap: nvs ready");

    ESP_LOGI("NVS", "protocol=%d", nvs_cfg_get()->protocol);
    ESP_LOGI("NVS", "theme=%d, dominant=%d, secondary=%d",
             nvs_cfg_get()->theme_cfg.theme,
             nvs_cfg_get()->theme_cfg.user_theme_domiant_color,
             nvs_cfg_get()->theme_cfg.user_theme_secondary_color);

    {
        const nvs_stat_t *stat = nvs_stat_get();
        ESP_LOGI("NVS", "odo=%" PRIu64 " trip=%" PRIu64 " max_spd=%d avg_spd=%d runtime=%" PRIu64,
                 stat->odometer_m, stat->trip_m, stat->max_speed_kmh, stat->avg_speed_kmh, stat->run_time_s);
    }

    ESP_LOGI(TAG, "bootstrap: load vehicle profile");
    vehicle_profile_set_active(nvs_cfg_get()->vehicle_profile_idx);
    ESP_LOGI("NVS", "vehicle_profile=%d (%s)",
             nvs_cfg_get()->vehicle_profile_idx, vehicle_profile_get_active()->name);
}

/**
 * Bring up the board abstraction and populate display context.
 *
 * Core responsibilities: initialize board drivers, inspect board
 * capabilities, start the panel, and publish UI layout dimensions.
 */
void app_bootstrap_init_board_and_display(board_display_context_t *out_ctx)
{
    const board_profile_t *profile = NULL;

    ESP_ERROR_CHECK(out_ctx != NULL ? ESP_OK : ESP_ERR_INVALID_ARG);

    ESP_LOGI(TAG, "bootstrap: board_init begin");
    ESP_ERROR_CHECK(board_init());
    ESP_LOGI(TAG, "bootstrap: board_init done");

    profile = board_profile();
    ESP_LOGI(TAG, "Board initialized: %s", board_name());
    ESP_ERROR_CHECK(profile != NULL ? ESP_OK : ESP_ERR_INVALID_STATE);
    ESP_LOGI(TAG, "Board profile: %s %ux%u %u-bit buffer_lines=%u touch=%s",
             profile->name, profile->hor_res, profile->ver_res, profile->color_bits,
             profile->draw_buffer_lines, profile->has_touch ? "yes" : "no");
    if (profile->has_touch) {
        ESP_LOGI(TAG, "Touch transform: swap_xy=%d mirror_x=%d mirror_y=%d",
                 profile->touch_swap_xy, profile->touch_mirror_x, profile->touch_mirror_y);
    }

    ESP_LOGI(TAG, "bootstrap: board_display_init begin");
    ESP_ERROR_CHECK(board_display_init(out_ctx));
    ESP_LOGI(TAG, "bootstrap: board_display_init done");

    ui_platform_init(out_ctx->hor_res, out_ctx->ver_res);
    ESP_LOGI(TAG, "Display ready: %ux%u %u-bit touch=%s",
             out_ctx->hor_res, out_ctx->ver_res, out_ctx->color_bits,
             out_ctx->has_touch ? "yes" : "no");
}

/**
 * Start runtime services after the UI thread is alive.
 *
 * Core responsibilities: connect OBD BLE, start optional telemetry,
 * refresh demand flags, and launch board-specific sensor tasks.
 */
void app_bootstrap_start_runtime_services(void)
{
    const nvs_user_cfg_t *user_cfg = nvs_cfg_get();
    const char *ble_name = (user_cfg->ble_device_name[0] != '\0') ? user_cfg->ble_device_name : "OBDII";

    ESP_LOGI(TAG, "BLE target device: %s", ble_name);
    elm327_ble_start_default(ble_name);

    // ========== OPTIONAL SERVICES ==========
    // Give the BLE client a short head start so secondary services
    // do not compete with the first connect attempt during boot.
    vTaskDelay(pdMS_TO_TICKS(500));
    if (nvs_cfg_is_racechrono_ble_enabled(user_cfg)) {
        racechrono_ble_diy_start();
    } else {
        ESP_LOGI(TAG, "RaceChrono BLE DIY disabled by settings");
    }

    rs485_brake_temp_start();
    qmi8658_gforce_start();
    aux_sensor_demand_refresh();

#if CONFIG_OBD_BOARD_WS_185
    oil_pressure_start();
#else
    // This sensor path is physically absent on other boards.
    ESP_LOGI(TAG, "Skip ADS1115 oil pressure task on this board profile");
#endif

    vMileageDataStatisticTask();
}
