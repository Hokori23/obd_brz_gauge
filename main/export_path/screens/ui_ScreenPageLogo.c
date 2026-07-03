// 此文件由 SquareLine Studio 生成
// SquareLine Studio 版本：1.5.0
// LVGL 版本：8.3.11
// 工程名：SquareLine_Project

#include "../ui.h"
#include "../ui_font_profile.h"
#include "../ui_layout.h"
#include "../ui_round_shell.h"

/**
 * 创建启动 Logo 页面
 *
 * 核心职责：根据配置加载 GIF 或文字 Logo，并挂上背景点击事件
 */
void ui_ScreenPageLogo_screen_init(void)
{
    ui_logo_layout_t layout;
    ui_logo_layout_get(&layout);

    ui_ScreenPageLogo = lv_obj_create(NULL);
    ui_round_screen_apply_base(ui_ScreenPageLogo, lv_color_hex(0x000000));
#if USE_GIF_LOGO == 1
    imageLogo = lv_gif_create(ui_ScreenPageLogo);
    lv_gif_set_src(imageLogo, &gifSnake400);
    lv_obj_align(imageLogo, LV_ALIGN_CENTER, 0, 0);
#else
    // 使用 Conthrax 字体绘制文字版 SKY GAUGE Logo
    lv_obj_t *label_sky = lv_label_create(ui_ScreenPageLogo);
    lv_label_set_text(label_sky, "SKY");
    lv_obj_set_style_text_font(label_sky, ui_font_typoder(56), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_sky, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(label_sky, layout.sky_letter_space, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(label_sky, LV_ALIGN_CENTER, 0, layout.sky_y);

    lv_obj_t *label_gauge = lv_label_create(ui_ScreenPageLogo);
    lv_label_set_text(label_gauge, "GAUGE");
    lv_obj_set_style_text_font(label_gauge, ui_font_typoder(36), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_gauge, lv_color_hex(0xAAAAAA), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(label_gauge, layout.gauge_letter_space, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(label_gauge, LV_ALIGN_CENTER, 0, layout.gauge_y);

    imageLogo = NULL; // 当前分支不使用图片 Logo

    ui_round_shell_create_ring(ui_ScreenPageLogo, &layout.shell);
#endif
    lv_obj_add_event_cb(ui_ScreenPageLogo, ui_event_logo_background, LV_EVENT_CLICKED, NULL);
}

