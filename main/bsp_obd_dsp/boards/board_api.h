#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

typedef bool (*board_display_flush_ready_cb_t)(esp_lcd_panel_io_handle_t panel_io,
                                               esp_lcd_panel_io_event_data_t *edata,
                                               void *user_ctx);

typedef struct {
    uint16_t hor_res;
    uint16_t ver_res;
    uint16_t draw_buffer_lines;
    uint8_t color_bits;
    bool has_touch;
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t panel_io;
    esp_lcd_touch_handle_t touch;
} board_display_context_t;

typedef struct {
    const char *name;
    uint16_t hor_res;
    uint16_t ver_res;
    uint16_t draw_buffer_lines;
    uint8_t color_bits;
    bool has_touch;
    bool touch_swap_xy;
    bool touch_mirror_x;
    bool touch_mirror_y;
} board_profile_t;

esp_err_t board_init(void);
esp_err_t board_register_display_flush_ready_callback(board_display_flush_ready_cb_t cb, void *user_ctx);
esp_err_t board_display_init(board_display_context_t *ctx);
esp_err_t board_set_brightness(uint8_t percent);
esp_err_t board_get_shared_i2c_bus(i2c_master_bus_handle_t *out_bus);
const board_profile_t *board_profile(void);
const char *board_name(void);
bool board_has_touch(void);
