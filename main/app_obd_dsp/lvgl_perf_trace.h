#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char reason[16];
    uint8_t from_page;
    uint8_t to_page;
    uint32_t duration_ms;
    uint32_t flushes;
    uint32_t pixels;
    uint32_t avg_pixels;
    uint32_t max_width;
    uint32_t max_height;
    uint32_t full_width_flushes;
    uint32_t line_band_flushes;
    uint32_t tail_band_flushes;
    uint32_t rounds_started;
    int64_t avg_submit_us;
    int64_t max_submit_us;
    int64_t avg_done_us;
    int64_t max_done_us;
    uint8_t rotation;
    uint32_t buffer_lines;
} app_lvgl_perf_trace_stats_t;

void app_lvgl_perf_trace_begin(const char *reason, uint8_t from_page, uint8_t to_page);
void app_lvgl_perf_trace_update_target(uint8_t to_page);
void app_lvgl_perf_trace_cancel(void);
bool app_lvgl_perf_trace_end(app_lvgl_perf_trace_stats_t *out_stats);
