#include "../ui.h"
#include "../ui_font_profile.h"
#include "../ui_layout.h"
#include "../ui_round_shell.h"
#include "bsp_obd_dsp/nvs_storage.h"

extern lv_obj_t *ui_ScreenPageTempCustom;
static lv_obj_t *s_temp_rollers[3] = {NULL, NULL, NULL};

static const char *k_data_options =
    "CLT\n"
    "IAT\n"
    "OIL\n"
    "LOAD\n"
    "TPS\n"
    "RPM\n"
    "SPEED\n"
    "BAT\n"
    "OILP\n"
    "BKT\n"
    "BOOST";

static void on_temp_map_changed(lv_event_t *e)
{
    uintptr_t idx = (uintptr_t)lv_event_get_user_data(e);
    if (idx >= 3) return;

    nvs_user_cfg_t cfg = *nvs_cfg_get();
    cfg.temp_display_map[idx] = (uint8_t)lv_roller_get_selected(s_temp_rollers[idx]);
    nvs_cfg_set(&cfg);
}

static void create_row(const ui_temp_custom_layout_t *layout, lv_obj_t *parent, int idx, const char *title, lv_coord_t y)
{
    const lv_coord_t roller_radius = ui_layout_px(8);

    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_font(lbl, ui_font_hint(12), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_align(lbl, LV_ALIGN_CENTER, layout->label_x, y);

    s_temp_rollers[idx] = lv_roller_create(parent);
    lv_obj_clear_flag(s_temp_rollers[idx], LV_OBJ_FLAG_GESTURE_BUBBLE); // 滚动选值时不触发页面手势
    lv_roller_set_options(s_temp_rollers[idx], k_data_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_temp_rollers[idx], 1);
    lv_obj_set_width(s_temp_rollers[idx], layout->roller_width);
    lv_obj_set_height(s_temp_rollers[idx], layout->roller_height);
    lv_obj_set_style_text_font(s_temp_rollers[idx], ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_temp_rollers[idx], ui_font_typoder(20), LV_PART_SELECTED);
    ui_round_shell_apply_roller_theme(s_temp_rollers[idx],
                                      roller_radius,
                                      lv_color_hex(0x222222),
                                      lv_color_hex(0x444444),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0xFFFFFF),
                                      lv_color_hex(0xCFCFCF),
                                      lv_color_hex(0x000000));
    lv_obj_align(s_temp_rollers[idx], LV_ALIGN_CENTER, layout->roller_x, y);
    lv_obj_add_event_cb(s_temp_rollers[idx], on_temp_map_changed, LV_EVENT_VALUE_CHANGED, (void *)idx);
}

void ui_ScreenPageTempCustom_screen_init(void)
{
    const nvs_user_cfg_t *cfg = nvs_cfg_get();
    ui_temp_custom_layout_t layout;
    ui_temp_custom_layout_get(&layout);

    ui_ScreenPageTempCustom = lv_obj_create(NULL);
    ui_round_screen_apply_base(ui_ScreenPageTempCustom, lv_color_hex(0x000000));
    ui_round_shell_create_ring(ui_ScreenPageTempCustom, &layout.shell);

    lv_obj_t *title = lv_label_create(ui_ScreenPageTempCustom);
    lv_label_set_text(title, "TEMP CUSTOM");
    lv_obj_set_style_text_font(title, ui_font_typoder(24), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, layout.title_y);

    create_row(&layout, ui_ScreenPageTempCustom, 0, "ROW 1", layout.row_y[0]);
    create_row(&layout, ui_ScreenPageTempCustom, 1, "ROW 2", layout.row_y[1]);
    create_row(&layout, ui_ScreenPageTempCustom, 2, "ROW 3", layout.row_y[2]);

    lv_roller_set_selected(s_temp_rollers[0], cfg->temp_display_map[0], LV_ANIM_OFF);
    lv_roller_set_selected(s_temp_rollers[1], cfg->temp_display_map[1], LV_ANIM_OFF);
    lv_roller_set_selected(s_temp_rollers[2], cfg->temp_display_map[2], LV_ANIM_OFF);

    lv_obj_t *hint = lv_label_create(ui_ScreenPageTempCustom);
    lv_label_set_text(hint, "Swipe up to return");
    lv_obj_set_style_text_font(hint, ui_font_hint(12), LV_PART_MAIN);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, layout.hint_y);

    lv_obj_t *ear = lv_img_create(ui_ScreenPageTempCustom);
    lv_img_set_src(ear, &ui_img_pngblackear_png);
    lv_obj_align(ear, LV_ALIGN_CENTER, 0, layout.shell.black_ear_offset_y);

    lv_obj_add_event_cb(ui_ScreenPageTempCustom, ui_event_temp_custom_background, LV_EVENT_GESTURE, NULL);
}
