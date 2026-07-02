#include "ui_platform.h"
#include "ui_layout.h"

#include <stddef.h>

static uint16_t s_width = UI_PLATFORM_BASE_RES;
static uint16_t s_height = UI_PLATFORM_BASE_RES;

void ui_platform_init(uint16_t width, uint16_t height)
{
    s_width = (width == 0) ? UI_PLATFORM_BASE_RES : width;
    s_height = (height == 0) ? UI_PLATFORM_BASE_RES : height;
}

uint16_t ui_screen_width(void)
{
    return s_width;
}

uint16_t ui_screen_height(void)
{
    return s_height;
}

int16_t ui_center_x(void)
{
    return UI_PLATFORM_CENTER_X(s_width);
}

int16_t ui_center_y(void)
{
    return UI_PLATFORM_CENTER_Y(s_height);
}

int16_t ui_round_radius(void)
{
    return UI_PLATFORM_ROUND_RADIUS(s_width, s_height);
}

int16_t ui_safe_margin(void)
{
    return UI_PLATFORM_SAFE_MARGIN(s_width, s_height);
}

int16_t ui_scale_px(int16_t base_px)
{
    return UI_PLATFORM_SCALE_PX(s_width, s_height, base_px);
}

int16_t ui_layout_px(int16_t base_px)
{
    return ui_scale_px(base_px);
}

void ui_round_shell_layout_get(uint16_t base_ring_arc_width, ui_round_shell_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    layout->ring_diameter = (uint16_t)ui_layout_px(UI_PLATFORM_BASE_RES);
    layout->ring_arc_width = (uint16_t)ui_layout_px((int16_t)base_ring_arc_width);
    layout->black_ear_offset_y = ui_layout_px(-142);
}

void ui_ble_scan_layout_get(ui_ble_scan_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(10, &layout->shell);
    layout->title_y = ui_layout_px(18);
    layout->spinner_size = (uint16_t)ui_layout_px(24);
    layout->spinner_x = ui_layout_px(72);
    layout->spinner_y = ui_layout_px(20);
    layout->status_y = ui_layout_px(50);
    layout->saved_header_y = ui_layout_px(72);
    layout->saved_panel_width = (uint16_t)ui_layout_px(264);
    layout->saved_panel_height = (uint16_t)ui_layout_px(32);
    layout->saved_panel_y = ui_layout_px(90);
    layout->saved_panel_radius = (uint16_t)ui_layout_px(6);
    layout->saved_panel_pad = (uint16_t)ui_layout_px(4);
    layout->saved_name_x = ui_layout_px(4);
    layout->delete_button_width = (uint16_t)ui_layout_px(30);
    layout->delete_button_height = (uint16_t)ui_layout_px(24);
    layout->delete_button_x = ui_layout_px(-2);
    layout->delete_button_radius = (uint16_t)ui_layout_px(4);
    layout->delete_button_pad = (uint16_t)ui_layout_px(2);
    layout->divider_width = (uint16_t)ui_layout_px(240);
    layout->divider_y = ui_layout_px(128);
    layout->nearby_y = ui_layout_px(134);
    layout->list_width = (uint16_t)ui_layout_px(264);
    layout->list_height = (uint16_t)ui_layout_px(145);
    layout->list_y = ui_layout_px(152);
    layout->list_radius = (uint16_t)ui_layout_px(8);
    layout->list_pad = (uint16_t)ui_layout_px(4);
    layout->hint_y = ui_layout_px(-15);
    layout->spinner_arc_width = (uint16_t)ui_layout_px(3);
}

void ui_settings_layout_get(ui_settings_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(10, &layout->shell);
    layout->title_y = ui_layout_px(-138);
    layout->label_page_y = ui_layout_px(-112);
    layout->roller_width = (uint16_t)ui_layout_px(140);
    layout->roller_radius = (uint16_t)ui_layout_px(8);
    layout->roller_page_y = ui_layout_px(-82);
    layout->divider_width = (uint16_t)ui_layout_px(220);
    layout->divider1_y = ui_layout_px(-54);
    layout->label_vehicle_y = ui_layout_px(-30);
    layout->roller_vehicle_y = ui_layout_px(-2);
    layout->divider2_y = ui_layout_px(26);
    layout->label_poll_y = ui_layout_px(40);
    layout->roller_poll_y = ui_layout_px(68);
    layout->divider3_y = ui_layout_px(94);
    layout->label_brightness_y = ui_layout_px(106);
    layout->slider_width = (uint16_t)ui_layout_px(180);
    layout->slider_height = (uint16_t)ui_layout_px(10);
    layout->slider_y = ui_layout_px(130);
    layout->slider_knob_pad = (uint16_t)ui_layout_px(5);
    layout->brightness_value_y = ui_layout_px(152);
    layout->hint_y = ui_layout_px(124);
}

void ui_brake_temp_layout_get(ui_brake_temp_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(8, &layout->shell);
    layout->title_y = ui_layout_px(-122);
    layout->dot_size = (uint16_t)ui_layout_px(10);
    layout->dot_x = ui_layout_px(-106);
    layout->dot_y = ui_layout_px(-22);
    layout->value_width = (uint16_t)ui_layout_px(172);
    layout->value_x = ui_layout_px(-10);
    layout->value_y = ui_layout_px(-24);
    layout->unit_x = ui_layout_px(88);
    layout->unit_y = ui_layout_px(-26);
    layout->trend_width = (uint16_t)ui_layout_px(272);
    layout->trend_height = (uint16_t)ui_layout_px(116);
    layout->trend_y = ui_layout_px(62);
    layout->trend_radius = (uint16_t)ui_layout_px(48);
    layout->chart_line_width = (uint16_t)ui_layout_px(3);
    layout->tick_line_width = (uint16_t)ui_layout_px(1);
}

void ui_oil_pressure_layout_get(ui_oil_pressure_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(8, &layout->shell);
    layout->title_y = ui_layout_px(-122);
    layout->dot_size = (uint16_t)ui_layout_px(10);
    layout->dot_x = ui_layout_px(-108);
    layout->dot_y = ui_layout_px(-14);
    layout->value_width = (uint16_t)ui_layout_px(220);
    layout->value_x = ui_layout_px(0);
    layout->value_y = ui_layout_px(-20);
    layout->unit_x = ui_layout_px(0);
    layout->unit_y = ui_layout_px(30);
    layout->trend_width = (uint16_t)ui_layout_px(272);
    layout->trend_height = (uint16_t)ui_layout_px(116);
    layout->trend_y = ui_layout_px(82);
    layout->trend_radius = (uint16_t)ui_layout_px(48);
    layout->chart_line_width = (uint16_t)ui_layout_px(3);
    layout->tick_line_width = (uint16_t)ui_layout_px(1);
}

void ui_info_layout_get(ui_info_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(8, &layout->shell);
    layout->tile_name_dy = ui_layout_px(-28);
    layout->tile_value_dy = ui_layout_px(2);
    layout->tile_unit_dy = ui_layout_px(34);
    layout->tile_value_width = (uint16_t)ui_layout_px(120);
    layout->title_y = ui_layout_px(-148);
    layout->hdiv1_y = ui_layout_px(-36);
    layout->hdiv1_width = (uint16_t)ui_layout_px(260);
    layout->hdiv2_y = ui_layout_px(68);
    layout->hdiv2_width = (uint16_t)ui_layout_px(200);
    layout->vdiv_x = ui_layout_px(0);
    layout->vdiv_y = ui_layout_px(-31);
    layout->vdiv_height = (uint16_t)ui_layout_px(196);
    layout->row1_cy = ui_layout_px(-84);
    layout->row2_cy = ui_layout_px(22);
    layout->row3_cy = ui_layout_px(112);
    layout->left_col_x = ui_layout_px(-82);
    layout->right_col_x = ui_layout_px(82);
    layout->center_col_x = ui_layout_px(0);
}

void ui_temp_layout_get(ui_temp_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(10, &layout->shell);
    layout->dot_size = (uint16_t)ui_layout_px(10);
    layout->value_width = (uint16_t)ui_layout_px(110);
    layout->value_x = ui_layout_px(70);
    layout->dot_x = ui_layout_px(185);
    layout->name_x = ui_layout_px(200);
    layout->unit_x = ui_layout_px(-70);
    layout->divider_width = (uint16_t)ui_layout_px(220);
    layout->row1_cy = ui_layout_px(-65);
    layout->divider1_y = ui_layout_px(-30);
    layout->row2_cy = ui_layout_px(5);
    layout->divider2_y = ui_layout_px(40);
    layout->row3_cy = ui_layout_px(75);
    layout->inner_arc_diameter = (uint16_t)ui_layout_px(340);
    layout->inner_arc_width = (uint16_t)ui_layout_px(20);
}

void ui_needle_layout_get(ui_needle_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(8, &layout->shell);
    layout->meter_size = (uint16_t)ui_layout_px(320);
    layout->tick_width = (uint16_t)ui_layout_px(2);
    layout->tick_length = (uint16_t)ui_layout_px(8);
    layout->major_tick_width = (uint16_t)ui_layout_px(4);
    layout->major_tick_length = (uint16_t)ui_layout_px(14);
    layout->major_label_gap = (uint16_t)ui_layout_px(14);
    layout->indicator_width = (uint16_t)ui_layout_px(4);
    layout->indicator_r_mod = ui_layout_px(-70);
    layout->name_y = ui_layout_px(-52);
    layout->value_y = ui_layout_px(54);
    layout->unit_y = ui_layout_px(94);
}

void ui_needle_config_layout_get(ui_needle_config_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(0, &layout->shell);
    layout->title_y = ui_layout_px(-110);
    layout->roller_width = (uint16_t)ui_layout_px(160);
    layout->roller_radius = (uint16_t)ui_layout_px(8);
    layout->roller_y = ui_layout_px(8);
    layout->hint_y = ui_layout_px(120);
}

void ui_warn_layout_get(ui_warn_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(10, &layout->shell);
    layout->title_y = ui_layout_px(-122);
    layout->subtitle_y = ui_layout_px(-88);
    layout->value_y = ui_layout_px(-24);
    layout->slider_width = (uint16_t)ui_layout_px(220);
    layout->slider_height = (uint16_t)ui_layout_px(12);
    layout->slider_y = ui_layout_px(26);
    layout->slider_knob_pad = (uint16_t)ui_layout_px(6);
    layout->range_y = ui_layout_px(58);
    layout->hint_y = ui_layout_px(110);
}

void ui_logo_layout_get(ui_logo_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(10, &layout->shell);
    layout->sky_y = ui_layout_px(-20);
    layout->gauge_y = ui_layout_px(30);
    layout->sky_letter_space = ui_layout_px(8);
    layout->gauge_letter_space = ui_layout_px(12);
}

void ui_obd_protocol_layout_get(ui_obd_protocol_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(10, &layout->shell);
    layout->inner_arc_diameter = (uint16_t)ui_layout_px(340);
    layout->inner_arc_width = (uint16_t)ui_layout_px(20);
    layout->roller_height = (uint16_t)ui_layout_px(150);
    layout->roller_radius = (uint16_t)ui_layout_px(10);
    layout->selected_radius = (uint16_t)ui_layout_px(5);
    layout->title_y = ui_layout_px(-121);
    layout->hint_x = ui_layout_px(-2);
    layout->hint_y = ui_layout_px(128);
}

void ui_info_custom_layout_get(ui_info_custom_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(10, &layout->shell);
    layout->title_y = ui_layout_px(-132);
    layout->label_x = ui_layout_px(-100);
    layout->roller_x = ui_layout_px(30);
    layout->roller_width = (uint16_t)ui_layout_px(170);
    layout->roller_height = (uint16_t)ui_layout_px(38);
    layout->row_y[0] = ui_layout_px(-82);
    layout->row_y[1] = ui_layout_px(-38);
    layout->row_y[2] = ui_layout_px(6);
    layout->row_y[3] = ui_layout_px(50);
    layout->row_y[4] = ui_layout_px(94);
    layout->hint_y = ui_layout_px(-22);
}

void ui_temp_custom_layout_get(ui_temp_custom_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(10, &layout->shell);
    layout->title_y = ui_layout_px(-132);
    layout->label_x = ui_layout_px(-92);
    layout->roller_x = ui_layout_px(34);
    layout->roller_width = (uint16_t)ui_layout_px(170);
    layout->roller_height = (uint16_t)ui_layout_px(42);
    layout->row_y[0] = ui_layout_px(-72);
    layout->row_y[1] = ui_layout_px(-18);
    layout->row_y[2] = ui_layout_px(36);
    layout->hint_y = ui_layout_px(92);
}

void ui_easter_egg_layout_get(ui_easter_egg_layout_t *layout)
{
    if (layout == NULL) {
        return;
    }

    ui_round_shell_layout_get(10, &layout->shell);
    layout->title_y = ui_layout_px(-70);
    layout->info_width = (uint16_t)ui_layout_px(280);
    layout->info_y = ui_layout_px(-5);
    layout->info_line_space = ui_layout_px(2);
    layout->hint_y = ui_layout_px(-58);
}
