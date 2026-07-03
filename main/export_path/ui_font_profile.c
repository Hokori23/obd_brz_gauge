#include "ui_font_profile.h"

#include "ui.h"

static lv_font_t s_typoder_16;
static lv_font_t s_typoder_20;
static lv_font_t s_typoder_24;
static lv_font_t s_typoder_28;
static lv_font_t s_typoder_32;
static lv_font_t s_typoder_36;
static lv_font_t s_typoder_40;
static lv_font_t s_typoder_44;
static lv_font_t s_typoder_56;
static lv_font_t s_typoder_100;
static lv_font_t s_typoder_140;
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
    ui_font_set_fallback(&s_typoder_40, &ui_font_FontTypoderSize40, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_typoder_44, &ui_font_FontTypoderSize44, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_typoder_56, &ui_font_FontTypoderSize56, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_typoder_100, &ui_font_FontTypoderSize100, &lv_font_montserrat_48);
    ui_font_set_fallback(&s_typoder_140, &ui_font_FontTypoderSize140, &lv_font_montserrat_48);

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
    case 40:
        return &s_typoder_40;
    case 44:
        return &s_typoder_44;
    case 56:
        return &s_typoder_56;
    case 100:
        return &s_typoder_100;
    case 140:
    default:
        return &s_typoder_140;
    }
}

const lv_font_t *ui_font_typoder_exact(int16_t font_px)
{
    switch (font_px) {
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
    case 40:
        return &s_typoder_40;
    case 44:
        return &s_typoder_44;
    case 56:
        return &s_typoder_56;
    case 100:
        return &s_typoder_100;
    case 140:
    default:
        return &s_typoder_140;
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

const lv_font_t *ui_font_baby(int16_t base_px)
{
    int16_t scaled_px = ui_layout_px(base_px);

    if (scaled_px <= 64) {
        return &s_baby_56;
    }
    if (scaled_px <= 132) {
        return &s_baby_108;
    }
    if (scaled_px <= 168) {
        return &s_baby_156;
    }
    if (scaled_px <= 190) {
        return &s_baby_180;
    }
    return &s_baby_200;
}

const lv_font_t *ui_font_brake_temp_value(int16_t value_x10)
{
    return ui_font_typoder(UI_FONT_BRAKE_TEMP_BASE_SIZE(value_x10));
}

const int16_t *ui_font_typoder_candidate_sizes(uint8_t *count_out)
{
    static const int16_t k_candidates[] = {
        140, 100, 56, 44, 40, 36, 32, 28, 24, 20, 16
    };

    if (count_out != NULL) {
        *count_out = (uint8_t)(sizeof(k_candidates) / sizeof(k_candidates[0]));
    }
    return k_candidates;
}

lv_coord_t ui_font_measure_text_width(const lv_font_t *font, const char *text)
{
    lv_point_t size = {0};

    if (font == NULL || text == NULL) {
        return 0;
    }
    lv_txt_get_size(&size, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size.x;
}

static bool ui_font_measure_text_box(const lv_font_t *font,
                                     const char *text,
                                     lv_point_t *size_out,
                                     lv_coord_t *height_out)
{
    lv_point_t size = {0};
    lv_coord_t text_height;

    if (font == NULL || text == NULL) {
        return false;
    }
    lv_txt_get_size(&size, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    text_height = (lv_coord_t)LV_MAX(size.y, font->line_height);
    if (size_out != NULL) {
        *size_out = size;
    }
    if (height_out != NULL) {
        *height_out = text_height;
    }
    return true;
}

bool ui_font_pick_typoder_fit_for_box(const char *text,
                                      lv_coord_t width,
                                      lv_coord_t height,
                                      lv_coord_t padding,
                                      ui_font_fit_t *fit_out)
{
    uint8_t candidate_count = 0;
    const int16_t *candidates = ui_font_typoder_candidate_sizes(&candidate_count);
    lv_coord_t safe_w;
    lv_coord_t safe_h;

    if (text == NULL) {
        text = "";
    }
    safe_w = (width > (padding * 2)) ? (width - (padding * 2)) : 1;
    safe_h = (height > (padding * 2)) ? (height - (padding * 2)) : 1;

    for (uint8_t i = 0; i < candidate_count; ++i) {
        int16_t candidate = candidates[i];
        const lv_font_t *font = ui_font_typoder_exact(candidate);
        lv_point_t size = {0};
        lv_coord_t text_height = 0;

        if (ui_font_measure_text_box(font, text, &size, &text_height) &&
            size.x <= safe_w && text_height <= safe_h) {
            if (fit_out != NULL) {
                fit_out->font_size = candidate;
                fit_out->font = font;
                fit_out->text_w = size.x;
                fit_out->text_h = text_height;
            }
            return true;
        }
    }

    if (fit_out != NULL) {
        const lv_font_t *font = ui_font_typoder_exact(16);
        lv_point_t size = {0};
        lv_coord_t text_height = 0;

        ui_font_measure_text_box(font, text, &size, &text_height);
        fit_out->font_size = 16;
        fit_out->font = font;
        fit_out->text_w = size.x;
        fit_out->text_h = text_height;
    }
    return false;
}

int16_t ui_font_pick_typoder_for_box(const char *text,
                                     lv_coord_t width,
                                     lv_coord_t height,
                                     lv_coord_t padding)
{
    ui_font_fit_t fit = {0};

    ui_font_pick_typoder_fit_for_box(text, width, height, padding, &fit);
    return (fit.font_size > 0) ? fit.font_size : 16;
}
