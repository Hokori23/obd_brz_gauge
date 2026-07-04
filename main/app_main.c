/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "app_bootstrap.h"
#include "app_lvgl_port.h"

#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "export_path/ui.h"

static const char *TAG = "obd_dsp";

/**
 * Application entrypoint.
 *
 * Core responsibilities: bootstrap storage and board state,
 * bring up LVGL, create the UI, and start background services.
 */
void app_main(void)
{
    board_display_context_t board_ctx = {0};
    bool display_only = false;

    ESP_LOGI(TAG, "app_main: enter");

    app_bootstrap_init_storage_and_profile();
    app_bootstrap_init_board_and_display(&board_ctx);
    display_only = app_bootstrap_display_only_enabled();
    if (display_only) {
        ESP_LOGW(TAG,
                 "display-only bootstrap enabled: stop after board_display_init "
                 "(LVGL, UI, and runtime services are skipped)");
        return;
    }
    ESP_ERROR_CHECK(app_lvgl_port_init(&board_ctx));
    ESP_ERROR_CHECK(app_lvgl_port_start_task());

    // ========== UI STARTUP ==========
    // Hold the LVGL mutex before touching the default display.
    // This keeps screen creation ordered with the dedicated LVGL task.
    ESP_LOGI(TAG, "Start UI");
    if (app_lvgl_lock(-1)) {
        lv_disp_set_bg_color(lv_disp_get_default(), lv_color_black());
        lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_COVER);
        ui_init();
        app_lvgl_unlock();
    }

    app_bootstrap_start_runtime_services();
}
