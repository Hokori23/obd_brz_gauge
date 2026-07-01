/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"

#if CONFIG_OBD_BOARD_WS_175_AMOLED
#include "esp_lv_adapter.h"
#endif

#include "app_obd_dsp/lvgl_buffer_profile.h"
#include "app_obd_dsp/obd_data_cache.h"
#include "app_obd_dsp/vehicle_profiles.h"
#include "bsp_obd_dsp/ads1115_oil_pressure.h"
#include "bsp_obd_dsp/boards/board_api.h"
#include "bsp_obd_dsp/elm327_ble_client.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "bsp_obd_dsp/racechrono_ble_diy.h"
#include "bsp_obd_dsp/rs485_brake_temp.h"
#include "export_path/ui_platform.h"

static const char *TAG = "obd_dsp";

extern void ui_init(void);

SemaphoreHandle_t lvgl_mux = NULL;

#if !CONFIG_OBD_BOARD_WS_175_AMOLED
static bool s_touch_active = false;
static int s_touch_last_x = -1;
static int s_touch_last_y = -1;
static uint32_t s_touch_press_tick = 0;
static uint32_t s_flush_request_count = 0;
static lv_disp_drv_t *s_registered_disp_drv = NULL;
#endif

#define LVGL_TICK_PERIOD_MS    2
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 2
#define LVGL_TASK_STACK_SIZE   (4 * 1024)
#define LVGL_TASK_PRIORITY     2
#define LVGL_TRACE_TOUCH       0
#define LVGL_TRACE_FLUSH       0

#if !CONFIG_OBD_BOARD_WS_175_AMOLED
static lv_color_t *alloc_lvgl_draw_buffer(size_t bytes, const char *label, bool prefer_internal_dma)
{
    lv_color_t *buffer = NULL;

    if (!prefer_internal_dma) {
        buffer = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
        if (buffer != NULL) {
            ESP_LOGI(TAG, "Allocated %s LVGL buffer in PSRAM DMA (%u bytes)", label, (unsigned)bytes);
            return buffer;
        }
    }

    buffer = heap_caps_malloc(bytes, MALLOC_CAP_DMA);
    if (buffer != NULL) {
        if (prefer_internal_dma) {
            ESP_LOGI(TAG, "Allocated %s LVGL buffer in internal DMA (%u bytes)", label, (unsigned)bytes);
        } else {
            ESP_LOGW(TAG, "Falling back to internal DMA for %s LVGL buffer (%u bytes)", label, (unsigned)bytes);
        }
    }

    return buffer;
}

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                    esp_lcd_panel_io_event_data_t *edata,
                                    void *user_ctx)
{
    (void)panel_io;
    (void)edata;
    (void)user_ctx;

    if (s_registered_disp_drv == NULL) {
        return false;
    }

    lv_disp_flush_ready(s_registered_disp_drv);
    return false;
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)drv->user_data;
    esp_err_t err;

    s_flush_request_count++;
#if LVGL_TRACE_FLUSH
    if (s_flush_request_count <= 12 || (s_flush_request_count % 20) == 0 ||
        ((area->x2 - area->x1) > 300 && (area->y2 - area->y1) > 300)) {
        ESP_LOGD(TAG, "flush req #%" PRIu32 ": area=(%d,%d)-(%d,%d) px=%dx%d panel=%p",
                 s_flush_request_count, area->x1, area->y1, area->x2, area->y2,
                 area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (void *)panel);
    }
#endif

    err = esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1,
                                    area->x2 + 1, area->y2 + 1, color_map);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "flush req #%" PRIu32 " failed: %s area=(%d,%d)-(%d,%d)",
                 s_flush_request_count, esp_err_to_name(err),
                 area->x1, area->y1, area->x2, area->y2);
        lv_disp_flush_ready(drv);
    }
}

static void lvgl_rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    (void)disp_drv;
    area->x1 = (area->x1 >> 1) << 1;
    area->y1 = (area->y1 >> 1) << 1;
    area->x2 = ((area->x2 >> 1) << 1) + 1;
    area->y2 = ((area->y2 >> 1) << 1) + 1;
}
#endif

static void lvgl_rounder_area_cb(lv_area_t *area, void *user_data)
{
    (void)user_data;
    area->x1 = (area->x1 >> 1) << 1;
    area->y1 = (area->y1 >> 1) << 1;
    area->x2 = ((area->x2 >> 1) << 1) + 1;
    area->y2 = ((area->y2 >> 1) << 1) + 1;
}

#if !CONFIG_OBD_BOARD_WS_175_AMOLED
static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    esp_lcd_touch_handle_t touch = (esp_lcd_touch_handle_t)drv->user_data;
    esp_lcd_touch_point_data_t touch_point = {0};
    uint8_t tp_cnt = 0;

    assert(touch);

    esp_lcd_touch_read_data(touch);

    if (esp_lcd_touch_get_data(touch, &touch_point, &tp_cnt, 1) == ESP_OK && tp_cnt > 0) {
        data->point.x = touch_point.x;
        data->point.y = touch_point.y;
        data->state = LV_INDEV_STATE_PRESSED;

        if (!s_touch_active) {
            s_touch_active = true;
            s_touch_press_tick = lv_tick_get();
#if LVGL_TRACE_TOUCH
            ESP_LOGD(TAG, "Touch press: x=%d y=%d", touch_point.x, touch_point.y);
#endif
        } else if (s_touch_last_x != touch_point.x || s_touch_last_y != touch_point.y) {
#if LVGL_TRACE_TOUCH
            ESP_LOGD(TAG, "Touch move: x=%d y=%d", touch_point.x, touch_point.y);
#endif
        }

        s_touch_last_x = touch_point.x;
        s_touch_last_y = touch_point.y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        if (s_touch_active) {
#if LVGL_TRACE_TOUCH
            uint32_t held_ms = lv_tick_elaps(s_touch_press_tick);
            ESP_LOGD(TAG, "Touch release: x=%d y=%d held=%" PRIu32 "ms",
                     s_touch_last_x, s_touch_last_y, held_ms);
#endif
            s_touch_active = false;
            s_touch_last_x = -1;
            s_touch_last_y = -1;
            s_touch_press_tick = 0;
        }
    }
}

static void increase_lvgl_tick(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static bool lvgl_lock(int timeout_ms)
{
    assert(lvgl_mux && "lvgl_mux not created");
    return xSemaphoreTake(lvgl_mux,
                          (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

static void lvgl_unlock(void)
{
    assert(lvgl_mux && "lvgl_mux not created");
    xSemaphoreGive(lvgl_mux);
}
#endif

bool app_lvgl_lock(int timeout_ms)
{
#if CONFIG_OBD_BOARD_WS_175_AMOLED
    return esp_lv_adapter_lock(timeout_ms) == ESP_OK;
#else
    return lvgl_lock(timeout_ms);
#endif
}

void app_lvgl_unlock(void)
{
#if CONFIG_OBD_BOARD_WS_175_AMOLED
    esp_lv_adapter_unlock();
#else
    lvgl_unlock();
#endif
}

#if !CONFIG_OBD_BOARD_WS_175_AMOLED
static void lvgl_port_task(void *arg)
{
    uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;

    (void)arg;
    ESP_LOGI(TAG, "Starting LVGL task");

    while (1) {
        if (lvgl_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            lvgl_unlock();
        }

        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }

        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}
#endif

void app_main(void)
{
    board_display_context_t board_ctx = {0};
    const board_profile_t *profile = NULL;

#if !CONFIG_OBD_BOARD_WS_175_AMOLED
    static lv_disp_draw_buf_t disp_buf;
    static lv_disp_drv_t disp_drv;
    bool prefer_internal_lvgl_dma = false;
#endif

    ESP_LOGI(TAG, "app_main: enter");
    ESP_LOGI(TAG, "app_main: init nvs");
    nvs_storage_init();
    ESP_LOGI(TAG, "app_main: nvs ready");

    ESP_LOGI("NVS", "protocol=%d", nvs_cfg_get()->protocol);
    ESP_LOGI("NVS", "theme=%d, dominant=%d, secondary=%d",
             nvs_cfg_get()->theme_cfg.theme,
             nvs_cfg_get()->theme_cfg.user_theme_domiant_color,
             nvs_cfg_get()->theme_cfg.user_theme_secondary_color);

    const nvs_stat_t *stat = nvs_stat_get();
    ESP_LOGI("NVS", "odo=%" PRIu64 " trip=%" PRIu64 " max_spd=%d avg_spd=%d runtime=%" PRIu64,
             stat->odometer_m, stat->trip_m, stat->max_speed_kmh, stat->avg_speed_kmh, stat->run_time_s);

    ESP_LOGI(TAG, "app_main: load vehicle profile");
    vehicle_profile_set_active(nvs_cfg_get()->vehicle_profile_idx);
    ESP_LOGI("NVS", "vehicle_profile=%d (%s)",
             nvs_cfg_get()->vehicle_profile_idx, vehicle_profile_get_active()->name);

    ESP_LOGI(TAG, "app_main: board_init begin");
    ESP_ERROR_CHECK(board_init());
    ESP_LOGI(TAG, "app_main: board_init done");

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

    ESP_LOGI(TAG, "app_main: board_display_init begin");
    ESP_ERROR_CHECK(board_display_init(&board_ctx));
    ESP_LOGI(TAG, "app_main: board_display_init done");
    ui_platform_init(board_ctx.hor_res, board_ctx.ver_res);
    ESP_LOGI(TAG, "Display ready: %ux%u %u-bit touch=%s",
             board_ctx.hor_res, board_ctx.ver_res, board_ctx.color_bits,
             board_ctx.has_touch ? "yes" : "no");

#if CONFIG_OBD_BOARD_WS_175_AMOLED
    {
        esp_lv_adapter_config_t lvgl_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
        esp_lv_adapter_display_config_t disp_cfg = {
            .panel = board_ctx.panel,
            .panel_io = board_ctx.panel_io,
            .profile = {
                .interface = ESP_LV_ADAPTER_PANEL_IF_OTHER,
                .rotation = ESP_LV_ADAPTER_ROTATE_0,
                .hor_res = board_ctx.hor_res,
                .ver_res = board_ctx.ver_res,
                .buffer_height = board_ctx.ver_res,
                .use_psram = true,
                .enable_ppa_accel = true,
                .require_double_buffer = true,
            },
            // QSPI panel is registered as PANEL_IF_OTHER in esp_lvgl_adapter.
            // This interface only supports NONE or TE_SYNC tear modes.
            .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE,
        };
        lv_display_t *disp = NULL;

        ESP_LOGI(TAG, "Initialize LVGL through esp_lvgl_adapter");
        ESP_ERROR_CHECK(esp_lv_adapter_init(&lvgl_cfg));

        disp = esp_lv_adapter_register_display(&disp_cfg);
        ESP_ERROR_CHECK(disp != NULL ? ESP_OK : ESP_FAIL);
        ESP_ERROR_CHECK(esp_lv_adapter_set_area_rounder_cb(disp, lvgl_rounder_area_cb, NULL));

        if (board_ctx.has_touch && board_ctx.touch != NULL) {
            const esp_lv_adapter_touch_config_t touch_cfg =
                ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, board_ctx.touch);
            ESP_ERROR_CHECK(esp_lv_adapter_register_touch(&touch_cfg) != NULL ? ESP_OK : ESP_FAIL);
        }

        ESP_ERROR_CHECK(esp_lv_adapter_start());
    }
#else
    ESP_LOGI(TAG, "Initialize LVGL");
    lv_init();

    lv_disp_drv_init(&disp_drv);
    ESP_ERROR_CHECK(board_register_display_flush_ready_callback(notify_lvgl_flush_ready, NULL));

    prefer_internal_lvgl_dma = false;
    ESP_ERROR_CHECK(board_ctx.draw_buffer_lines > 0 ? ESP_OK : ESP_ERR_INVALID_STATE);

    uint32_t lvgl_buff_size = LVGL_BUFFER_PIXEL_COUNT(board_ctx.hor_res, board_ctx.draw_buffer_lines);
    size_t lvgl_buff_bytes = LVGL_BUFFER_BYTE_COUNT(board_ctx.hor_res, board_ctx.draw_buffer_lines, sizeof(lv_color_t));
    lv_color_t *buf1 = alloc_lvgl_draw_buffer(lvgl_buff_bytes, "buf1", prefer_internal_lvgl_dma);
    lv_color_t *buf2 = alloc_lvgl_draw_buffer(lvgl_buff_bytes, "buf2", prefer_internal_lvgl_dma);

    assert(buf1);
    assert(buf2);

    ESP_LOGI(TAG, "LVGL buffers: pixels=%" PRIu32 " bytes=%u buf1_psram=%s buf2_psram=%s",
             lvgl_buff_size, (unsigned)lvgl_buff_bytes,
             esp_ptr_external_ram(buf1) ? "yes" : "no",
             esp_ptr_external_ram(buf2) ? "yes" : "no");

    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, lvgl_buff_size);

    disp_drv.hor_res = board_ctx.hor_res;
    disp_drv.ver_res = board_ctx.ver_res;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.rounder_cb = lvgl_rounder_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = board_ctx.panel;

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    s_registered_disp_drv = disp->driver;

    ESP_LOGI(TAG, "Registered LVGL display driver: template=%p runtime=%p disp=%p",
             (void *)&disp_drv, (void *)s_registered_disp_drv, (void *)disp);

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    if (board_ctx.has_touch && board_ctx.touch != NULL) {
        static lv_indev_drv_t indev_drv;

        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.disp = disp;
        indev_drv.read_cb = lvgl_touch_cb;
        indev_drv.user_data = board_ctx.touch;
        lv_indev_drv_register(&indev_drv);
    }

    lvgl_mux = xSemaphoreCreateMutex();
    assert(lvgl_mux);
    xTaskCreate(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);
#endif

    ESP_LOGI(TAG, "Start UI");
    if (app_lvgl_lock(-1)) {
        ui_init();
        app_lvgl_unlock();
    }

    const nvs_user_cfg_t *user_cfg = nvs_cfg_get();
    const char *ble_name = (user_cfg->ble_device_name[0] != '\0') ? user_cfg->ble_device_name : "OBDII";
    ESP_LOGI(TAG, "BLE target device: %s", ble_name);
    elm327_ble_start_default(ble_name);

    vTaskDelay(pdMS_TO_TICKS(500));
    racechrono_ble_diy_start();

    rs485_brake_temp_start();

#if CONFIG_OBD_BOARD_WS_185
    oil_pressure_start();
#else
    ESP_LOGI(TAG, "Skip ADS1115 oil pressure task on this board profile");
#endif

    vMileageDataStatisticTask();
}
