#pragma once

#include "ui_platform.h"

#include <stdint.h>

typedef struct _lv_font_t lv_font_t;

#define UI_FONT_TYPODER_SELECT_SIZE(scaled_px) \
    (((scaled_px) <= 18) ? 16 : \
    ((scaled_px) <= 22) ? 20 : \
    ((scaled_px) < 26) ? 24 : \
    ((scaled_px) <= 30) ? 28 : \
    ((scaled_px) <= 34) ? 32 : \
    ((scaled_px) <= 40) ? 36 : \
    ((scaled_px) <= 50) ? 44 : \
    ((scaled_px) <= 78) ? 56 : \
    ((scaled_px) <= 120) ? 100 : \
    140)

#define UI_FONT_TYPODER_SIZE(width, height, base_px) \
    UI_FONT_TYPODER_SELECT_SIZE(UI_PLATFORM_SCALE_PX((width), (height), (base_px)))

#define UI_FONT_HINT_SELECT_SIZE(scaled_px) \
    (((scaled_px) <= 13) ? 12 : \
    ((scaled_px) <= 15) ? 14 : \
    16)

#define UI_FONT_HINT_SIZE(width, height, base_px) \
    UI_FONT_HINT_SELECT_SIZE(UI_PLATFORM_SCALE_PX((width), (height), (base_px)))

#define UI_FONT_BRAKE_TEMP_BASE_SIZE(value_x10) \
    ((((value_x10) >= 10000) || ((value_x10) <= -10000)) ? 24 : \
    (((value_x10) >= 1000) || ((value_x10) <= -1000)) ? 28 : \
    32)

const lv_font_t *ui_font_typoder(int16_t base_px);
const lv_font_t *ui_font_hint(int16_t base_px);
const lv_font_t *ui_font_baby(int16_t base_px);
const lv_font_t *ui_font_brake_temp_value(int16_t value_x10);
void ui_font_profile_init(void);
