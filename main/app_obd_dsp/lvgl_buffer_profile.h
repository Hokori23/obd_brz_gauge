#pragma once

#include <stdint.h>

#define LVGL_BUFFER_PIXEL_COUNT(hor_res, draw_buffer_lines) \
    ((uint32_t)(hor_res) * (uint32_t)(draw_buffer_lines))

#define LVGL_BUFFER_BYTE_COUNT(hor_res, draw_buffer_lines, pixel_size_bytes) \
    (LVGL_BUFFER_PIXEL_COUNT((hor_res), (draw_buffer_lines)) * (uint32_t)(pixel_size_bytes))
