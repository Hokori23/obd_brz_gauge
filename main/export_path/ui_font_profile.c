#include "ui_font_profile.h"

#include "ui.h"

static lv_font_t s_typoder_16;
static lv_font_t s_typoder_20;
static lv_font_t s_typoder_24;
static lv_font_t s_typoder_28;
static lv_font_t s_typoder_32;
static lv_font_t s_typoder_36;
static lv_font_t s_typoder_44;
static lv_font_t s_typoder_56;
static lv_font_t s_taikong_32;
static lv_font_t s_taikong_40;
static lv_font_t s_taikong_48;
static lv_font_t s_taikong_56;
static lv_font_t s_taikong_64;
static lv_font_t s_taikong_72;
static lv_font_t s_taikong_108;
static lv_font_t s_taikong_128;
static lv_font_t s_baby_gear_48;
static lv_font_t s_baby_56;
static lv_font_t s_baby_108;
static lv_font_t s_baby_156;
static lv_font_t s_baby_180;
static lv_font_t s_baby_200;

static void ui_font_set_fallback(lv_font_t *dst, const lv_font_t *src, const lv_font_t *fallback)
{
    *dst = *src;
    dst->fallback = fallback;
}

void ui_font_profile_init(void)
{
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;

    ui_font_set_fallback(&s_typoder_16, &ui_font_FontTypoderSize16, &lv_font_montserrat_16);
    ui_font_set_fallback(&s_typoder_20, &ui_font_FontTypoderSize20, &lv_font_montserrat_16);
    ui_font_set_fallback(&s_typoder_24, &ui_font_FontTypoderSize24, &lv_font_montserrat_26);
    ui_font_set_fallback(&s_typoder_28, &ui_font_FontTypoderSize28, &lv_font_montserrat_26);
    ui_font_set_fallback(&s_typoder_32, &ui_font_FontTypoderSize32, &lv_font_montserrat_32);
    ui_font_set_fallback(&s_typoder_36, &ui_font_FontTypoderSize36, &lv_font_montserrat_32);
    ui_font_set_fallback(&s_typoder_44, &ui_font_FontTypoderSize44, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_typoder_56, &ui_font_FontTypoderSize56, &lv_font_montserrat_48);

    ui_font_set_fallback(&s_taikong_32, &ui_font_FontTaikongSize32, &lv_font_montserrat_32);
    ui_font_set_fallback(&s_taikong_40, &ui_font_FontTaikongSize40, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_taikong_48, &ui_font_FontTaikongSize48, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_taikong_56, &ui_font_FontTaikongSize56, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_taikong_64, &ui_font_FontTaikongSize64, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_taikong_72, &ui_font_FontTaikongSize72, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_taikong_108, &ui_font_FontTaikongSize108, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_taikong_128, &ui_font_FontTaikongSize128, &lv_font_montserrat_48);

    ui_font_set_fallback(&s_baby_gear_48, &ui_font_FontBabyGearNumSize48, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_baby_56, &ui_font_FontBabySize56, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_baby_108, &ui_font_FontBabySize108, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_baby_156, &ui_font_FontBabySize156, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_baby_180, &ui_font_FontBabySize180, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_baby_200, &ui_font_FontBabySize200, &lv_font_montserrat_48);
}

static int16_t ui_font_typoder_size_runtime(int16_t base_px)
{
    return UI_FONT_TYPODER_SELECT_SIZE(ui_layout_px(base_px));
}

static int16_t ui_font_hint_size_runtime(int16_t base_px)
{
    return UI_FONT_HINT_SELECT_SIZE(ui_layout_px(base_px));
}

const lv_font_t *ui_font_typoder(int16_t base_px)
{
    switch (ui_font_typoder_size_runtime(base_px)) {
    case 16:
        return &s_typoder_16;
    case 20:
        return &s_typoder_20;
    case 24:
        return &s_typoder_24;
    case 28:
        return &s_typoder_28;
    case 32:
        return &s_typoder_32;
    case 36:
        return &s_typoder_36;
    case 44:
        return &s_typoder_44;
    case 56:
    default:
        return &s_typoder_56;
    }
}

const lv_font_t *ui_font_hint(int16_t base_px)
{
    switch (ui_font_hint_size_runtime(base_px)) {
    case 12:
        return &lv_font_montserrat_12;
    case 14:
        return &lv_font_montserrat_14;
    case 16:
    default:
        return &lv_font_montserrat_16;
    }
}

const lv_font_t *ui_font_brake_temp_value(int16_t value_x10)
{
    return ui_font_typoder(UI_FONT_BRAKE_TEMP_BASE_SIZE(value_x10));
}
