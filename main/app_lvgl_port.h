#pragma once

#include <stdbool.h>

#include "bsp_obd_dsp/boards/board_api.h"
#include "esp_err.h"

esp_err_t app_lvgl_port_init(const board_display_context_t *ctx);
esp_err_t app_lvgl_port_start_task(void);

bool app_lvgl_lock(int timeout_ms);
void app_lvgl_unlock(void);
