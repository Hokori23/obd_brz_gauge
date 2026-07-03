#pragma once

#include "ui_platform.h"
#include "lvgl.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct _lv_font_t lv_font_t;

typedef struct {
    int16_t font_size;
    const lv_font_t *font;
    lv_coord_t text_w;
    lv_coord_t text_h;
} ui_font_fit_t;

#define UI_FONT_TYPODER_SELECT_SIZE(scaled_px) \
    (((scaled_px) <= 18) ? 16 : \
    ((scaled_px) <= 22) ? 20 : \
    ((scaled_px) < 26) ? 24 : \
    ((scaled_px) <= 30) ? 28 : \
    ((scaled_px) <= 34) ? 32 : \
    ((scaled_px) <= 38) ? 36 : \
    ((scaled_px) <= 40) ? 40 : \
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
const lv_font_t *ui_font_typoder_exact(int16_t font_px);
const lv_font_t *ui_font_hint(int16_t base_px);
const lv_font_t *ui_font_baby(int16_t base_px);
const lv_font_t *ui_font_brake_temp_value(int16_t value_x10);
lv_coord_t ui_font_measure_text_width(const lv_font_t *font, const char *text);
const int16_t *ui_font_typoder_candidate_sizes(uint8_t *count_out);
bool ui_font_pick_typoder_fit_for_box(const char *text,
                                      lv_coord_t width,
                                      lv_coord_t height,
                                      lv_coord_t padding,
                                      ui_font_fit_t *fit_out);
int16_t ui_font_pick_typoder_for_box(const char *text,
                                     lv_coord_t width,
                                     lv_coord_t height,
                                     lv_coord_t padding);
void ui_font_profile_init(void);
