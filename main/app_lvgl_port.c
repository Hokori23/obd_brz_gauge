#include "app_lvgl_port.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "app_obd_dsp/lvgl_buffer_profile.h"
#include "app_obd_dsp/lvgl_perf_trace.h"
#include "bsp_obd_dsp/boards/board_api.h"
#include "bsp_obd_dsp/nvs_storage.h"
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
#include "bsp_obd_dsp/boards/board_ws_175_amoled_spec.h"
#endif

static const char *TAG = "app_lvgl";

typedef struct {
    bool active;
    char reason[16];
    uint8_t from_page;
    uint8_t to_page;
    int64_t start_us;
    uint32_t flush_count;
    uint32_t pixel_count;
    uint32_t max_pixels;
    uint32_t max_width;
    uint32_t max_height;
    uint32_t full_width_flushes;
    uint32_t line_band_flushes;
    uint32_t tail_band_flushes;
    uint32_t rounds_started;
    uint32_t done_count;
    int64_t submit_us_total;
    int64_t done_us_total;
    int64_t submit_us_max;
    int64_t done_us_max;
} lvgl_perf_trace_state_t;

static SemaphoreHandle_t s_lvgl_mux = NULL;
static TaskHandle_t s_lvgl_task = NULL;
static esp_timer_handle_t s_lvgl_tick_timer = NULL;
static lv_disp_drv_t *s_registered_disp_drv = NULL;
static bool s_lvgl_initialized = false;

static bool s_touch_active = false;
static int s_touch_last_x = -1;
static int s_touch_last_y = -1;
static uint32_t s_touch_press_tick = 0;
static uint32_t s_flush_request_count = 0;
static uint32_t s_flush_window_count = 0;
static uint32_t s_flush_window_pixels = 0;
static uint32_t s_flush_window_max_pixels = 0;
static uint32_t s_flush_window_max_width = 0;
static uint32_t s_flush_window_max_height = 0;
static uint32_t s_flush_window_done_count = 0;
static int64_t s_flush_window_submit_us = 0;
static int64_t s_flush_window_done_us = 0;
static int64_t s_flush_window_max_submit_us = 0;
static int64_t s_flush_window_max_done_us = 0;
static int64_t s_flush_pending_start_us = 0;
static portMUX_TYPE s_lvgl_perf_trace_mux = portMUX_INITIALIZER_UNLOCKED;
static lvgl_perf_trace_state_t s_lvgl_perf_trace = {0};

#define LVGL_TICK_PERIOD_MS    2
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 2
#define LVGL_TASK_STACK_SIZE   (10 * 1024)
#define LVGL_TASK_PRIORITY     2
#define LVGL_TRACE_TOUCH       0
#define LVGL_TRACE_FLUSH       0
#define LVGL_PERF_PERIODIC_LOG 0
#define LVGL_PERF_LOG_WINDOW_US 5000000LL

static lv_disp_rot_t display_rotation(void);
static lv_color_t *alloc_lvgl_draw_buffer(size_t bytes, const char *label, bool prefer_internal_dma);
static void lvgl_perf_trace_reset_locked(void);
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                    esp_lcd_panel_io_event_data_t *edata,
                                    void *user_ctx);
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
static void lvgl_rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area);
static void lvgl_wait_cb(lv_disp_drv_t *drv);
static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);
static void increase_lvgl_tick(void *arg);
static bool lvgl_lock(int timeout_ms);
static void lvgl_unlock(void);
static void lvgl_port_task(void *arg);

/**
 * 解析当前显示旋转模式
 *
 * 核心职责：把配置层的旋转选项映射成 LVGL 可识别的方向。
 */
static lv_disp_rot_t display_rotation(void)
{
#if CONFIG_OBD_BOARD_WS_175_AMOLED
    switch (nvs_cfg_get_display_rotation_degrees(nvs_cfg_get(), BOARD_WS_175_AMOLED_DISPLAY_ROTATION)) {
    case 180u:
        return LV_DISP_ROT_180;
    case 0u:
    default:
        return LV_DISP_ROT_NONE;
    }
#else
    return LV_DISP_ROT_NONE;
#endif
}

/**
 * 为 LVGL 申请绘制缓冲区
 *
 * 核心职责：按 DMA 能力和内存类型选择合适的绘图缓冲。
 */
static lv_color_t *alloc_lvgl_draw_buffer(size_t bytes, const char *label, bool prefer_internal_dma)
{
    lv_color_t *buffer = NULL;

    if (!prefer_internal_dma) {
        // 先尝试 PSRAM DMA，尽量把内部 SRAM 留给更敏感的实时路径。
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

/** 重置一次 LVGL 性能采样窗口。 */
static void lvgl_perf_trace_reset_locked(void)
{
    memset(&s_lvgl_perf_trace, 0, sizeof(s_lvgl_perf_trace));
}

/**
 * 开始一次页面切换性能采样
 *
 * 记录切换起点，便于后续统计 flush 和耗时。
 */
void app_lvgl_perf_trace_begin(const char *reason, uint8_t from_page, uint8_t to_page)
{
    portENTER_CRITICAL(&s_lvgl_perf_trace_mux);
    lvgl_perf_trace_reset_locked();
    s_lvgl_perf_trace.active = true;
    s_lvgl_perf_trace.from_page = from_page;
    s_lvgl_perf_trace.to_page = to_page;
    s_lvgl_perf_trace.start_us = esp_timer_get_time();
    if (reason != NULL) {
        snprintf(s_lvgl_perf_trace.reason, sizeof(s_lvgl_perf_trace.reason), "%s", reason);
    }
    portEXIT_CRITICAL(&s_lvgl_perf_trace_mux);
}

/** 更新当前性能采样的目标页面。 */
void app_lvgl_perf_trace_update_target(uint8_t to_page)
{
    portENTER_CRITICAL(&s_lvgl_perf_trace_mux);
    if (s_lvgl_perf_trace.active) {
        s_lvgl_perf_trace.to_page = to_page;
    }
    portEXIT_CRITICAL(&s_lvgl_perf_trace_mux);
}

/** 取消当前性能采样并清空窗口。 */
void app_lvgl_perf_trace_cancel(void)
{
    portENTER_CRITICAL(&s_lvgl_perf_trace_mux);
    lvgl_perf_trace_reset_locked();
    portEXIT_CRITICAL(&s_lvgl_perf_trace_mux);
}

/**
 * 结束一次性能采样并导出统计结果
 *
 * 只有采样处于激活状态时才会填充输出结构。
 */
bool app_lvgl_perf_trace_end(app_lvgl_perf_trace_stats_t *out_stats)
{
    bool active = false;

    portENTER_CRITICAL(&s_lvgl_perf_trace_mux);
    active = s_lvgl_perf_trace.active;
    if (active && out_stats != NULL) {
        memset(out_stats, 0, sizeof(*out_stats));
        snprintf(out_stats->reason, sizeof(out_stats->reason), "%s", s_lvgl_perf_trace.reason);
        out_stats->from_page = s_lvgl_perf_trace.from_page;
        out_stats->to_page = s_lvgl_perf_trace.to_page;
        out_stats->duration_ms = (uint32_t)((esp_timer_get_time() - s_lvgl_perf_trace.start_us) / 1000LL);
        out_stats->flushes = s_lvgl_perf_trace.flush_count;
        out_stats->pixels = s_lvgl_perf_trace.pixel_count;
        out_stats->avg_pixels = (s_lvgl_perf_trace.flush_count > 0)
                                    ? (s_lvgl_perf_trace.pixel_count / s_lvgl_perf_trace.flush_count)
                                    : 0;
        out_stats->max_width = s_lvgl_perf_trace.max_width;
        out_stats->max_height = s_lvgl_perf_trace.max_height;
        out_stats->full_width_flushes = s_lvgl_perf_trace.full_width_flushes;
        out_stats->line_band_flushes = s_lvgl_perf_trace.line_band_flushes;
        out_stats->tail_band_flushes = s_lvgl_perf_trace.tail_band_flushes;
        out_stats->rounds_started = s_lvgl_perf_trace.rounds_started;
        out_stats->avg_submit_us = (s_lvgl_perf_trace.flush_count > 0)
                                       ? (s_lvgl_perf_trace.submit_us_total / s_lvgl_perf_trace.flush_count)
                                       : 0;
        out_stats->max_submit_us = s_lvgl_perf_trace.submit_us_max;
        out_stats->avg_done_us = (s_lvgl_perf_trace.done_count > 0)
                                     ? (s_lvgl_perf_trace.done_us_total / s_lvgl_perf_trace.done_count)
                                     : 0;
        out_stats->max_done_us = s_lvgl_perf_trace.done_us_max;
        out_stats->rotation = (uint8_t)display_rotation();
        out_stats->buffer_lines = board_profile()->draw_buffer_lines;
    }
    lvgl_perf_trace_reset_locked();
    portEXIT_CRITICAL(&s_lvgl_perf_trace_mux);

    return active;
}

/**
 * 面板刷屏完成回调
 *
 * 核心职责：统计传输完成耗时，并通知 LVGL 当前 flush 已结束。
 */
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                    esp_lcd_panel_io_event_data_t *edata,
                                    void *user_ctx)
{
    int64_t done_us = 0;
    bool trace_active = false;

    (void)panel_io;
    (void)edata;
    (void)user_ctx;

    if (s_flush_pending_start_us != 0) {
        done_us = esp_timer_get_time() - s_flush_pending_start_us;
        if (done_us > 0) {
            s_flush_window_done_count++;
            s_flush_window_done_us += done_us;
            if (done_us > s_flush_window_max_done_us) {
                s_flush_window_max_done_us = done_us;
            }
        }
        s_flush_pending_start_us = 0;
    }

    portENTER_CRITICAL_ISR(&s_lvgl_perf_trace_mux);
    trace_active = s_lvgl_perf_trace.active;
    if (trace_active && done_us > 0) {
        s_lvgl_perf_trace.done_count++;
        s_lvgl_perf_trace.done_us_total += done_us;
        if (done_us > s_lvgl_perf_trace.done_us_max) {
            s_lvgl_perf_trace.done_us_max = done_us;
        }
    }
    portEXIT_CRITICAL_ISR(&s_lvgl_perf_trace_mux);

    if (s_registered_disp_drv == NULL) {
        return false;
    }

    lv_disp_flush_ready(s_registered_disp_drv);
    return false;
}

/**
 * LVGL 显示刷新回调
 *
 * 把 LVGL 输出区域提交给底层面板，并顺手采集刷新性能数据。
 */
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)drv->user_data;
    esp_err_t err;
    const uint32_t width = (uint32_t)(area->x2 - area->x1 + 1);
    const uint32_t height = (uint32_t)(area->y2 - area->y1 + 1);
    const uint32_t pixels = width * height;
    const int64_t start_us = esp_timer_get_time();
    const uint32_t band_lines = board_profile()->draw_buffer_lines;
    const bool spans_full_width = (area->x1 == 0) && (width == (uint32_t)drv->hor_res);
    const bool is_line_band = spans_full_width && (height == band_lines);
    const bool is_tail_band = spans_full_width &&
                              (area->y2 == (drv->ver_res - 1)) &&
                              (height < band_lines);
    const bool starts_round = is_line_band && (area->y1 == 0);

    s_flush_request_count++;
    s_flush_window_count++;
    s_flush_window_pixels += pixels;
    if (pixels > s_flush_window_max_pixels) {
        s_flush_window_max_pixels = pixels;
        s_flush_window_max_width = width;
        s_flush_window_max_height = height;
    }
    portENTER_CRITICAL(&s_lvgl_perf_trace_mux);
    if (s_lvgl_perf_trace.active) {
        s_lvgl_perf_trace.flush_count++;
        s_lvgl_perf_trace.pixel_count += pixels;
        if (pixels > s_lvgl_perf_trace.max_pixels) {
            s_lvgl_perf_trace.max_pixels = pixels;
            s_lvgl_perf_trace.max_width = width;
            s_lvgl_perf_trace.max_height = height;
        }
        if (spans_full_width) {
            s_lvgl_perf_trace.full_width_flushes++;
        }
        if (is_line_band) {
            s_lvgl_perf_trace.line_band_flushes++;
        }
        if (is_tail_band) {
            s_lvgl_perf_trace.tail_band_flushes++;
        }
        if (starts_round) {
            s_lvgl_perf_trace.rounds_started++;
        }
    }
    portEXIT_CRITICAL(&s_lvgl_perf_trace_mux);
#if LVGL_TRACE_FLUSH
    if (s_flush_request_count <= 12 || (s_flush_request_count % 20) == 0 ||
        ((area->x2 - area->x1) > 300 && (area->y2 - area->y1) > 300)) {
        ESP_LOGI(TAG, "flush req #%" PRIu32 ": area=(%d,%d)-(%d,%d) px=%dx%d panel=%p rot=%d",
                 s_flush_request_count, area->x1, area->y1, area->x2, area->y2,
                 area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (void *)panel,
                 display_rotation());
    }
#endif

    // 这里把提交耗时和完成耗时拆开统计，便于区分软件准备慢还是总线传输慢。
    err = esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1,
                                    area->x2 + 1, area->y2 + 1, color_map);
    s_flush_pending_start_us = (err == ESP_OK) ? start_us : 0;
    if (err == ESP_OK) {
        int64_t submit_us = esp_timer_get_time() - start_us;
        if (submit_us > 0) {
            s_flush_window_submit_us += submit_us;
            if (submit_us > s_flush_window_max_submit_us) {
                s_flush_window_max_submit_us = submit_us;
            }
        }
        portENTER_CRITICAL(&s_lvgl_perf_trace_mux);
        if (s_lvgl_perf_trace.active && submit_us > 0) {
            s_lvgl_perf_trace.submit_us_total += submit_us;
            if (submit_us > s_lvgl_perf_trace.submit_us_max) {
                s_lvgl_perf_trace.submit_us_max = submit_us;
            }
        }
        portEXIT_CRITICAL(&s_lvgl_perf_trace_mux);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "flush req #%" PRIu32 " failed: %s area=(%d,%d)-(%d,%d)",
                 s_flush_request_count, esp_err_to_name(err),
                 area->x1, area->y1, area->x2, area->y2);
        lv_disp_flush_ready(drv);
    }
}

/** 对刷新区域做硬件友好的像素对齐。 */
static void lvgl_rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    (void)disp_drv;
    area->x1 = (area->x1 >> 1) << 1;
    area->y1 = (area->y1 >> 1) << 1;
    area->x2 = ((area->x2 >> 1) << 1) + 1;
    area->y2 = ((area->y2 >> 1) << 1) + 1;
}

/** 在 LVGL 等待阶段主动让出调度片段。 */
static void lvgl_wait_cb(lv_disp_drv_t *drv)
{
    (void)drv;
    vTaskDelay(pdMS_TO_TICKS(1));
}

/**
 * LVGL 触摸输入回调
 *
 * 读取触摸芯片状态，并维护按下过程中的辅助调试信息。
 */
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

/** 周期推进 LVGL 内部时钟。 */
static void increase_lvgl_tick(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

/**
 * 获取 LVGL 全局互斥锁
 *
 * 所有跨任务操作 LVGL 的路径都要经过这里，避免图形状态并发破坏。
 */
static bool lvgl_lock(int timeout_ms)
{
    if (s_lvgl_mux == NULL) {
        return false;
    }

    return xSemaphoreTake(s_lvgl_mux,
                          (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

/** 释放 LVGL 全局互斥锁。 */
static void lvgl_unlock(void)
{
    if (s_lvgl_mux == NULL) {
        return;
    }

    xSemaphoreGive(s_lvgl_mux);
}

/** 向业务层暴露 LVGL 锁接口。 */
bool app_lvgl_lock(int timeout_ms)
{
    return lvgl_lock(timeout_ms);
}

/** 向业务层暴露 LVGL 解锁接口。 */
void app_lvgl_unlock(void)
{
    lvgl_unlock();
}

/**
 * LVGL 后台任务
 *
 * 驱动 LVGL 定时器循环，并按窗口汇总刷新性能数据。
 */
static void lvgl_port_task(void *arg)
{
    uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    int64_t last_perf_log_us = esp_timer_get_time();

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

        {
            const int64_t now_us = esp_timer_get_time();
            if ((now_us - last_perf_log_us) >= LVGL_PERF_LOG_WINDOW_US) {
                // 周期性清空窗口统计，避免长期运行后平均值失真。
                if (LVGL_PERF_PERIODIC_LOG && s_flush_window_count > 0) {
                    const int64_t window_us = now_us - last_perf_log_us;
                    const uint32_t avg_pixels = s_flush_window_pixels / s_flush_window_count;
                    const uint32_t flushes_per_s = (uint32_t)((s_flush_window_count * 1000000ULL) / (uint64_t)window_us);
                    const uint32_t pixels_per_s = (uint32_t)((s_flush_window_pixels * 1000000ULL) / (uint64_t)window_us);
                    const int64_t avg_submit_us = (s_flush_window_count > 0)
                                                      ? (s_flush_window_submit_us / s_flush_window_count)
                                                      : 0;
                    const int64_t avg_done_us = (s_flush_window_done_count > 0)
                                                    ? (s_flush_window_done_us / s_flush_window_done_count)
                                                    : 0;
                    ESP_LOGI(TAG,
                             "LVGL perf(5s): flushes=%" PRIu32 " flushes/s=%" PRIu32
                             " px=%" PRIu32 " px/s=%" PRIu32
                             " avg_px=%" PRIu32 " max_rect=%" PRIu32 "x%" PRIu32
                             " submit_us(avg/max)=%" PRIi64 "/%" PRIi64
                             " done_us(avg/max)=%" PRIi64 "/%" PRIi64
                             " rot=%d lines=%u",
                             s_flush_window_count,
                             flushes_per_s,
                             s_flush_window_pixels,
                             pixels_per_s,
                             avg_pixels,
                             s_flush_window_max_width,
                             s_flush_window_max_height,
                             avg_submit_us,
                             s_flush_window_max_submit_us,
                             avg_done_us,
                             s_flush_window_max_done_us,
                             display_rotation(),
                             (unsigned)board_profile()->draw_buffer_lines);
                }

                s_flush_window_count = 0;
                s_flush_window_pixels = 0;
                s_flush_window_max_pixels = 0;
                s_flush_window_max_width = 0;
                s_flush_window_max_height = 0;
                s_flush_window_done_count = 0;
                s_flush_window_submit_us = 0;
                s_flush_window_done_us = 0;
                s_flush_window_max_submit_us = 0;
                s_flush_window_max_done_us = 0;
                last_perf_log_us = now_us;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

/**
 * 初始化 LVGL 端口层
 *
 * 核心职责：绑定显示驱动、分配缓冲区、注册触摸输入并启动时钟。
 */
esp_err_t app_lvgl_port_init(const board_display_context_t *ctx)
{
    static lv_disp_draw_buf_t disp_buf;
    static lv_disp_drv_t disp_drv;
    static lv_indev_drv_t indev_drv;
    bool prefer_internal_lvgl_dma = false;
    uint32_t lvgl_buff_size;
    size_t lvgl_buff_bytes;
    lv_color_t *buf1;
    lv_color_t *buf2;
    lv_disp_t *disp;

    if (s_lvgl_initialized) {
        return ESP_OK;
    }
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (ctx->draw_buffer_lines == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Initialize LVGL");
    lv_init();

    lv_disp_drv_init(&disp_drv);
    ESP_ERROR_CHECK(board_register_display_flush_ready_callback(notify_lvgl_flush_ready, NULL));

    // ========== 缓冲区准备 ==========
    // 缓冲区大小必须和板级配置保持一致，否则分段 flush 的统计和时序都会失真。
    lvgl_buff_size = LVGL_BUFFER_PIXEL_COUNT(ctx->hor_res, ctx->draw_buffer_lines);
    lvgl_buff_bytes = LVGL_BUFFER_BYTE_COUNT(ctx->hor_res, ctx->draw_buffer_lines, sizeof(lv_color_t));
    buf1 = alloc_lvgl_draw_buffer(lvgl_buff_bytes, "buf1", prefer_internal_lvgl_dma);
    buf2 = alloc_lvgl_draw_buffer(lvgl_buff_bytes, "buf2", prefer_internal_lvgl_dma);
    if (buf1 == NULL || buf2 == NULL) {
        if (buf1 != NULL) {
            heap_caps_free(buf1);
        }
        if (buf2 != NULL) {
            heap_caps_free(buf2);
        }
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "LVGL buffers: pixels=%" PRIu32 " bytes=%u buf1_psram=%s buf2_psram=%s",
             lvgl_buff_size, (unsigned)lvgl_buff_bytes,
             esp_ptr_external_ram(buf1) ? "yes" : "no",
             esp_ptr_external_ram(buf2) ? "yes" : "no");

    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, lvgl_buff_size);

    disp_drv.hor_res = ctx->hor_res;
    disp_drv.ver_res = ctx->ver_res;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.rounder_cb = lvgl_rounder_cb;
    disp_drv.wait_cb = lvgl_wait_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = ctx->panel;
    disp_drv.sw_rotate = (display_rotation() != LV_DISP_ROT_NONE);
    disp_drv.rotated = display_rotation();

    disp = lv_disp_drv_register(&disp_drv);
    s_registered_disp_drv = disp->driver;

    ESP_LOGI(TAG, "Registered LVGL display driver: template=%p runtime=%p disp=%p",
             (void *)&disp_drv, (void *)s_registered_disp_drv, (void *)disp);
    ESP_LOGI(TAG, "LVGL software rotation=%d mode=%d",
             disp_drv.sw_rotate, disp_drv.rotated);

    // ========== LVGL 时钟 ==========
    if (s_lvgl_tick_timer == NULL) {
        const esp_timer_create_args_t lvgl_tick_timer_args = {
            .callback = &increase_lvgl_tick,
            .name = "lvgl_tick",
        };
        ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &s_lvgl_tick_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(s_lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));
    }

    // ========== 输入设备 ==========
    if (ctx->has_touch && ctx->touch != NULL) {
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.disp = disp;
        indev_drv.read_cb = lvgl_touch_cb;
        indev_drv.user_data = ctx->touch;
        lv_indev_drv_register(&indev_drv);
    }

    // 互斥锁要在初始化末尾创建，避免中途失败时留下半初始化同步对象。
    if (s_lvgl_mux == NULL) {
        s_lvgl_mux = xSemaphoreCreateMutex();
        if (s_lvgl_mux == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    s_lvgl_initialized = true;
    return ESP_OK;
}

/**
 * 启动 LVGL 后台任务
 *
 * 仅在端口初始化完成后创建任务，避免任务提前访问未就绪驱动。
 */
esp_err_t app_lvgl_port_start_task(void)
{
    if (!s_lvgl_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_lvgl_task != NULL) {
        return ESP_OK;
    }

    if (xTaskCreate(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, &s_lvgl_task) != pdPASS) {
        s_lvgl_task = NULL;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
