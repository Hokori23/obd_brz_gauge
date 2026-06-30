#pragma once

#define UI_PLATFORM_BASE_RES 360
#define UI_PLATFORM_SAFE_MARGIN_BASE_PX 16
#define UI_PLATFORM_MIN_DIM(width, height) (((width) < (height)) ? (width) : (height))
#define UI_PLATFORM_CENTER_X(width) ((int16_t)((width) / 2))
#define UI_PLATFORM_CENTER_Y(height) ((int16_t)((height) / 2))
#define UI_PLATFORM_ROUND_RADIUS(width, height) ((int16_t)(UI_PLATFORM_MIN_DIM((width), (height)) / 2))
#define UI_PLATFORM_ROUND_DIVIDE(numerator, denominator) \
    (((numerator) >= 0) ? (((numerator) + ((denominator) / 2)) / (denominator)) : (((numerator) - ((denominator) / 2)) / (denominator)))
#define UI_PLATFORM_SCALE_PX(width, height, base_px) \
    ((int16_t)UI_PLATFORM_ROUND_DIVIDE(((int32_t)(base_px) * (int32_t)UI_PLATFORM_MIN_DIM((width), (height))), UI_PLATFORM_BASE_RES))
#define UI_PLATFORM_SAFE_MARGIN(width, height) UI_PLATFORM_SCALE_PX((width), (height), UI_PLATFORM_SAFE_MARGIN_BASE_PX)

#include <stdint.h>

void ui_platform_init(uint16_t width, uint16_t height);
uint16_t ui_screen_width(void);
uint16_t ui_screen_height(void);
int16_t ui_center_x(void);
int16_t ui_center_y(void);
int16_t ui_round_radius(void);
int16_t ui_safe_margin(void);
int16_t ui_scale_px(int16_t base_px);
int16_t ui_layout_px(int16_t base_px);
