#pragma once

#include "ui_platform.h"

#include <stdint.h>

#define UI_LAYOUT_PX(width, height, base_px) UI_PLATFORM_SCALE_PX((width), (height), (base_px))

#define UI_LAYOUT_ROUND_SCREEN_DIAMETER(width, height) UI_LAYOUT_PX((width), (height), UI_PLATFORM_BASE_RES)
#define UI_LAYOUT_ROUND_RING_ARC_WIDTH(width, height, base_width) UI_LAYOUT_PX((width), (height), (base_width))
#define UI_LAYOUT_BLACK_EAR_OFFSET_Y(width, height) UI_LAYOUT_PX((width), (height), -142)

#define UI_LAYOUT_BLE_TITLE_Y(width, height) UI_LAYOUT_PX((width), (height), 18)
#define UI_LAYOUT_BLE_SPINNER_SIZE(width, height) UI_LAYOUT_PX((width), (height), 24)
#define UI_LAYOUT_BLE_SPINNER_X(width, height) UI_LAYOUT_PX((width), (height), 72)
#define UI_LAYOUT_BLE_SPINNER_Y(width, height) UI_LAYOUT_PX((width), (height), 20)
#define UI_LAYOUT_BLE_STATUS_Y(width, height) UI_LAYOUT_PX((width), (height), 50)
#define UI_LAYOUT_BLE_SAVED_HDR_Y(width, height) UI_LAYOUT_PX((width), (height), 72)
#define UI_LAYOUT_BLE_SAVED_PANEL_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 264)
#define UI_LAYOUT_BLE_SAVED_PANEL_HEIGHT(width, height) UI_LAYOUT_PX((width), (height), 32)
#define UI_LAYOUT_BLE_SAVED_PANEL_Y(width, height) UI_LAYOUT_PX((width), (height), 90)
#define UI_LAYOUT_BLE_SAVED_PANEL_RADIUS(width, height) UI_LAYOUT_PX((width), (height), 6)
#define UI_LAYOUT_BLE_SAVED_PANEL_PAD(width, height) UI_LAYOUT_PX((width), (height), 4)
#define UI_LAYOUT_BLE_SAVED_NAME_X(width, height) UI_LAYOUT_PX((width), (height), 4)
#define UI_LAYOUT_BLE_DELETE_BTN_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 30)
#define UI_LAYOUT_BLE_DELETE_BTN_HEIGHT(width, height) UI_LAYOUT_PX((width), (height), 24)
#define UI_LAYOUT_BLE_DELETE_BTN_X(width, height) UI_LAYOUT_PX((width), (height), -2)
#define UI_LAYOUT_BLE_DELETE_BTN_RADIUS(width, height) UI_LAYOUT_PX((width), (height), 4)
#define UI_LAYOUT_BLE_DELETE_BTN_PAD(width, height) UI_LAYOUT_PX((width), (height), 2)
#define UI_LAYOUT_BLE_DIVIDER_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 240)
#define UI_LAYOUT_BLE_DIVIDER_Y(width, height) UI_LAYOUT_PX((width), (height), 128)
#define UI_LAYOUT_BLE_NEARBY_Y(width, height) UI_LAYOUT_PX((width), (height), 134)
#define UI_LAYOUT_BLE_LIST_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 264)
#define UI_LAYOUT_BLE_LIST_HEIGHT(width, height) UI_LAYOUT_PX((width), (height), 145)
#define UI_LAYOUT_BLE_LIST_Y(width, height) UI_LAYOUT_PX((width), (height), 152)
#define UI_LAYOUT_BLE_LIST_RADIUS(width, height) UI_LAYOUT_PX((width), (height), 8)
#define UI_LAYOUT_BLE_LIST_PAD(width, height) UI_LAYOUT_PX((width), (height), 4)
#define UI_LAYOUT_BLE_HINT_Y(width, height) UI_LAYOUT_PX((width), (height), -15)

#define UI_LAYOUT_SETTINGS_TITLE_Y(width, height) UI_LAYOUT_PX((width), (height), -138)
#define UI_LAYOUT_SETTINGS_LABEL_PAGE_Y(width, height) UI_LAYOUT_PX((width), (height), -112)
#define UI_LAYOUT_SETTINGS_ROLLER_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 140)
#define UI_LAYOUT_SETTINGS_ROLLER_RADIUS(width, height) UI_LAYOUT_PX((width), (height), 8)
#define UI_LAYOUT_SETTINGS_ROLLER_PAGE_Y(width, height) UI_LAYOUT_PX((width), (height), -90)
#define UI_LAYOUT_SETTINGS_DIVIDER_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 220)
#define UI_LAYOUT_SETTINGS_DIV1_Y(width, height) UI_LAYOUT_PX((width), (height), -68)
#define UI_LAYOUT_SETTINGS_LABEL_VEHICLE_Y(width, height) UI_LAYOUT_PX((width), (height), -50)
#define UI_LAYOUT_SETTINGS_ROLLER_VEHICLE_Y(width, height) UI_LAYOUT_PX((width), (height), -26)
#define UI_LAYOUT_SETTINGS_DIV2_Y(width, height) UI_LAYOUT_PX((width), (height), -4)
#define UI_LAYOUT_SETTINGS_LABEL_POLL_Y(width, height) UI_LAYOUT_PX((width), (height), 14)
#define UI_LAYOUT_SETTINGS_ROLLER_POLL_Y(width, height) UI_LAYOUT_PX((width), (height), 38)
#define UI_LAYOUT_SETTINGS_DIV3_Y(width, height) UI_LAYOUT_PX((width), (height), 62)
#define UI_LAYOUT_SETTINGS_LABEL_ROTATION_Y(width, height) UI_LAYOUT_PX((width), (height), 80)
#define UI_LAYOUT_SETTINGS_ROLLER_ROTATION_Y(width, height) UI_LAYOUT_PX((width), (height), 104)
#define UI_LAYOUT_SETTINGS_DIV4_Y(width, height) UI_LAYOUT_PX((width), (height), 128)
#define UI_LAYOUT_SETTINGS_LABEL_BRIGHT_Y(width, height) UI_LAYOUT_PX((width), (height), 146)
#define UI_LAYOUT_SETTINGS_SLIDER_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 168)
#define UI_LAYOUT_SETTINGS_SLIDER_HEIGHT(width, height) UI_LAYOUT_PX((width), (height), 10)
#define UI_LAYOUT_SETTINGS_SLIDER_Y(width, height) UI_LAYOUT_PX((width), (height), 164)
#define UI_LAYOUT_SETTINGS_SLIDER_KNOB_PAD(width, height) UI_LAYOUT_PX((width), (height), 5)
#define UI_LAYOUT_SETTINGS_BRIGHT_VALUE_X(width, height) UI_LAYOUT_PX((width), (height), 86)
#define UI_LAYOUT_SETTINGS_BRIGHT_VALUE_Y(width, height) UI_LAYOUT_PX((width), (height), 146)
#define UI_LAYOUT_SETTINGS_HINT_Y(width, height) UI_LAYOUT_PX((width), (height), 124)

#define UI_LAYOUT_BRAKE_TEMP_TITLE_Y(width, height) UI_LAYOUT_PX((width), (height), -122)
#define UI_LAYOUT_BRAKE_TEMP_DOT_SIZE(width, height) UI_LAYOUT_PX((width), (height), 10)
#define UI_LAYOUT_BRAKE_TEMP_DOT_X(width, height) UI_LAYOUT_PX((width), (height), -106)
#define UI_LAYOUT_BRAKE_TEMP_DOT_Y(width, height) UI_LAYOUT_PX((width), (height), -22)
#define UI_LAYOUT_BRAKE_TEMP_VALUE_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 172)
#define UI_LAYOUT_BRAKE_TEMP_VALUE_X(width, height) UI_LAYOUT_PX((width), (height), -10)
#define UI_LAYOUT_BRAKE_TEMP_VALUE_Y(width, height) UI_LAYOUT_PX((width), (height), -24)
#define UI_LAYOUT_BRAKE_TEMP_UNIT_X(width, height) UI_LAYOUT_PX((width), (height), 88)
#define UI_LAYOUT_BRAKE_TEMP_UNIT_Y(width, height) UI_LAYOUT_PX((width), (height), -26)
#define UI_LAYOUT_BRAKE_TEMP_TREND_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 272)
#define UI_LAYOUT_BRAKE_TEMP_TREND_HEIGHT(width, height) UI_LAYOUT_PX((width), (height), 116)
#define UI_LAYOUT_BRAKE_TEMP_TREND_Y(width, height) UI_LAYOUT_PX((width), (height), 62)
#define UI_LAYOUT_BRAKE_TEMP_TREND_RADIUS(width, height) UI_LAYOUT_PX((width), (height), 48)
#define UI_LAYOUT_BRAKE_TEMP_CHART_LINE_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 3)
#define UI_LAYOUT_BRAKE_TEMP_TICK_LINE_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 1)

#define UI_LAYOUT_OIL_PRESSURE_TITLE_Y(width, height) UI_LAYOUT_PX((width), (height), -122)
#define UI_LAYOUT_OIL_PRESSURE_DOT_SIZE(width, height) UI_LAYOUT_PX((width), (height), 10)
#define UI_LAYOUT_OIL_PRESSURE_DOT_X(width, height) UI_LAYOUT_PX((width), (height), -108)
#define UI_LAYOUT_OIL_PRESSURE_DOT_Y(width, height) UI_LAYOUT_PX((width), (height), -14)
#define UI_LAYOUT_OIL_PRESSURE_VALUE_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 220)
#define UI_LAYOUT_OIL_PRESSURE_VALUE_X(width, height) UI_LAYOUT_PX((width), (height), 0)
#define UI_LAYOUT_OIL_PRESSURE_VALUE_Y(width, height) UI_LAYOUT_PX((width), (height), -20)
#define UI_LAYOUT_OIL_PRESSURE_UNIT_X(width, height) UI_LAYOUT_PX((width), (height), 0)
#define UI_LAYOUT_OIL_PRESSURE_UNIT_Y(width, height) UI_LAYOUT_PX((width), (height), 30)
#define UI_LAYOUT_OIL_PRESSURE_TREND_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 272)
#define UI_LAYOUT_OIL_PRESSURE_TREND_HEIGHT(width, height) UI_LAYOUT_PX((width), (height), 116)
#define UI_LAYOUT_OIL_PRESSURE_TREND_Y(width, height) UI_LAYOUT_PX((width), (height), 82)
#define UI_LAYOUT_OIL_PRESSURE_TREND_RADIUS(width, height) UI_LAYOUT_PX((width), (height), 48)
#define UI_LAYOUT_OIL_PRESSURE_CHART_LINE_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 3)
#define UI_LAYOUT_OIL_PRESSURE_TICK_LINE_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 1)

#define UI_LAYOUT_INFO_TILE_NAME_DY(width, height) UI_LAYOUT_PX((width), (height), -28)
#define UI_LAYOUT_INFO_TILE_VALUE_DY(width, height) UI_LAYOUT_PX((width), (height), 2)
#define UI_LAYOUT_INFO_TILE_UNIT_DY(width, height) UI_LAYOUT_PX((width), (height), 34)
#define UI_LAYOUT_INFO_TILE_VALUE_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 120)
#define UI_LAYOUT_INFO_TITLE_Y(width, height) UI_LAYOUT_PX((width), (height), -148)
#define UI_LAYOUT_INFO_HDIV1_Y(width, height) UI_LAYOUT_PX((width), (height), -36)
#define UI_LAYOUT_INFO_HDIV1_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 260)
#define UI_LAYOUT_INFO_HDIV2_Y(width, height) UI_LAYOUT_PX((width), (height), 68)
#define UI_LAYOUT_INFO_HDIV2_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 200)
#define UI_LAYOUT_INFO_VDIV_X(width, height) UI_LAYOUT_PX((width), (height), 0)
#define UI_LAYOUT_INFO_VDIV_Y(width, height) UI_LAYOUT_PX((width), (height), -31)
#define UI_LAYOUT_INFO_VDIV_HEIGHT(width, height) UI_LAYOUT_PX((width), (height), 196)
#define UI_LAYOUT_INFO_ROW1_CY(width, height) UI_LAYOUT_PX((width), (height), -84)
#define UI_LAYOUT_INFO_ROW2_CY(width, height) UI_LAYOUT_PX((width), (height), 22)
#define UI_LAYOUT_INFO_ROW3_CY(width, height) UI_LAYOUT_PX((width), (height), 112)
#define UI_LAYOUT_INFO_LEFT_COL_X(width, height) UI_LAYOUT_PX((width), (height), -82)
#define UI_LAYOUT_INFO_RIGHT_COL_X(width, height) UI_LAYOUT_PX((width), (height), 82)
#define UI_LAYOUT_INFO_CENTER_COL_X(width, height) UI_LAYOUT_PX((width), (height), 0)

#define UI_LAYOUT_TEMP_DOT_SIZE(width, height) UI_LAYOUT_PX((width), (height), 10)
#define UI_LAYOUT_TEMP_VALUE_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 110)
#define UI_LAYOUT_TEMP_VALUE_X(width, height) UI_LAYOUT_PX((width), (height), 70)
#define UI_LAYOUT_TEMP_DOT_X(width, height) UI_LAYOUT_PX((width), (height), 185)
#define UI_LAYOUT_TEMP_NAME_X(width, height) UI_LAYOUT_PX((width), (height), 200)
#define UI_LAYOUT_TEMP_UNIT_X(width, height) UI_LAYOUT_PX((width), (height), -70)
#define UI_LAYOUT_TEMP_DIVIDER_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 220)
#define UI_LAYOUT_TEMP_ROW1_CY(width, height) UI_LAYOUT_PX((width), (height), -65)
#define UI_LAYOUT_TEMP_DIV1_Y(width, height) UI_LAYOUT_PX((width), (height), -30)
#define UI_LAYOUT_TEMP_ROW2_CY(width, height) UI_LAYOUT_PX((width), (height), 5)
#define UI_LAYOUT_TEMP_DIV2_Y(width, height) UI_LAYOUT_PX((width), (height), 40)
#define UI_LAYOUT_TEMP_ROW3_CY(width, height) UI_LAYOUT_PX((width), (height), 75)
#define UI_LAYOUT_TEMP_INNER_ARC_DIAMETER(width, height) UI_LAYOUT_PX((width), (height), 340)
#define UI_LAYOUT_TEMP_INNER_ARC_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 20)

#define UI_LAYOUT_NEEDLE_METER_SIZE(width, height) UI_LAYOUT_PX((width), (height), 320)
#define UI_LAYOUT_NEEDLE_TICK_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 2)
#define UI_LAYOUT_NEEDLE_TICK_LENGTH(width, height) UI_LAYOUT_PX((width), (height), 8)
#define UI_LAYOUT_NEEDLE_MAJOR_TICK_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 4)
#define UI_LAYOUT_NEEDLE_MAJOR_TICK_LENGTH(width, height) UI_LAYOUT_PX((width), (height), 14)
#define UI_LAYOUT_NEEDLE_MAJOR_LABEL_GAP(width, height) UI_LAYOUT_PX((width), (height), 14)
#define UI_LAYOUT_NEEDLE_INDICATOR_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 4)
#define UI_LAYOUT_NEEDLE_INDICATOR_R_MOD(width, height) UI_LAYOUT_PX((width), (height), -70)
#define UI_LAYOUT_NEEDLE_NAME_Y(width, height) UI_LAYOUT_PX((width), (height), -52)
#define UI_LAYOUT_NEEDLE_VALUE_Y(width, height) UI_LAYOUT_PX((width), (height), 54)
#define UI_LAYOUT_NEEDLE_UNIT_Y(width, height) UI_LAYOUT_PX((width), (height), 94)

#define UI_LAYOUT_NEEDLE_CONFIG_TITLE_Y(width, height) UI_LAYOUT_PX((width), (height), -110)
#define UI_LAYOUT_NEEDLE_CONFIG_ROLLER_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 160)
#define UI_LAYOUT_NEEDLE_CONFIG_ROLLER_RADIUS(width, height) UI_LAYOUT_PX((width), (height), 8)
#define UI_LAYOUT_NEEDLE_CONFIG_ROLLER_Y(width, height) UI_LAYOUT_PX((width), (height), 8)
#define UI_LAYOUT_NEEDLE_CONFIG_HINT_Y(width, height) UI_LAYOUT_PX((width), (height), 120)

#define UI_LAYOUT_WARN_TITLE_Y(width, height) UI_LAYOUT_PX((width), (height), -122)
#define UI_LAYOUT_WARN_SUBTITLE_Y(width, height) UI_LAYOUT_PX((width), (height), -88)
#define UI_LAYOUT_WARN_VALUE_Y(width, height) UI_LAYOUT_PX((width), (height), -24)
#define UI_LAYOUT_WARN_SLIDER_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 220)
#define UI_LAYOUT_WARN_SLIDER_HEIGHT(width, height) UI_LAYOUT_PX((width), (height), 12)
#define UI_LAYOUT_WARN_SLIDER_Y(width, height) UI_LAYOUT_PX((width), (height), 26)
#define UI_LAYOUT_WARN_SLIDER_KNOB_PAD(width, height) UI_LAYOUT_PX((width), (height), 6)
#define UI_LAYOUT_WARN_RANGE_Y(width, height) UI_LAYOUT_PX((width), (height), 58)
#define UI_LAYOUT_WARN_HINT_Y(width, height) UI_LAYOUT_PX((width), (height), 110)

#define UI_LAYOUT_LOGO_SKY_Y(width, height) UI_LAYOUT_PX((width), (height), -20)
#define UI_LAYOUT_LOGO_GAUGE_Y(width, height) UI_LAYOUT_PX((width), (height), 30)
#define UI_LAYOUT_LOGO_SKY_LETTER_SPACE(width, height) UI_LAYOUT_PX((width), (height), 8)
#define UI_LAYOUT_LOGO_GAUGE_LETTER_SPACE(width, height) UI_LAYOUT_PX((width), (height), 12)

#define UI_LAYOUT_OBD_PROTOCOL_INNER_ARC_DIAMETER(width, height) UI_LAYOUT_PX((width), (height), 340)
#define UI_LAYOUT_OBD_PROTOCOL_INNER_ARC_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 20)
#define UI_LAYOUT_OBD_PROTOCOL_ROLLER_HEIGHT(width, height) UI_LAYOUT_PX((width), (height), 150)
#define UI_LAYOUT_OBD_PROTOCOL_ROLLER_RADIUS(width, height) UI_LAYOUT_PX((width), (height), 10)
#define UI_LAYOUT_OBD_PROTOCOL_SELECTED_RADIUS(width, height) UI_LAYOUT_PX((width), (height), 5)
#define UI_LAYOUT_OBD_PROTOCOL_TITLE_Y(width, height) UI_LAYOUT_PX((width), (height), -121)
#define UI_LAYOUT_OBD_PROTOCOL_HINT_X(width, height) UI_LAYOUT_PX((width), (height), -2)
#define UI_LAYOUT_OBD_PROTOCOL_HINT_Y(width, height) UI_LAYOUT_PX((width), (height), 128)

#define UI_LAYOUT_INFO_CUSTOM_TITLE_Y(width, height) UI_LAYOUT_PX((width), (height), -132)
#define UI_LAYOUT_INFO_CUSTOM_LABEL_X(width, height) UI_LAYOUT_PX((width), (height), -100)
#define UI_LAYOUT_INFO_CUSTOM_ROLLER_X(width, height) UI_LAYOUT_PX((width), (height), 30)
#define UI_LAYOUT_INFO_CUSTOM_ROLLER_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 170)
#define UI_LAYOUT_INFO_CUSTOM_ROLLER_HEIGHT(width, height) UI_LAYOUT_PX((width), (height), 38)
#define UI_LAYOUT_INFO_CUSTOM_ROW1_Y(width, height) UI_LAYOUT_PX((width), (height), -82)
#define UI_LAYOUT_INFO_CUSTOM_ROW2_Y(width, height) UI_LAYOUT_PX((width), (height), -38)
#define UI_LAYOUT_INFO_CUSTOM_ROW3_Y(width, height) UI_LAYOUT_PX((width), (height), 6)
#define UI_LAYOUT_INFO_CUSTOM_ROW4_Y(width, height) UI_LAYOUT_PX((width), (height), 50)
#define UI_LAYOUT_INFO_CUSTOM_ROW5_Y(width, height) UI_LAYOUT_PX((width), (height), 94)
#define UI_LAYOUT_INFO_CUSTOM_HINT_Y(width, height) UI_LAYOUT_PX((width), (height), -22)

#define UI_LAYOUT_TEMP_CUSTOM_TITLE_Y(width, height) UI_LAYOUT_PX((width), (height), -132)
#define UI_LAYOUT_TEMP_CUSTOM_LABEL_X(width, height) UI_LAYOUT_PX((width), (height), -92)
#define UI_LAYOUT_TEMP_CUSTOM_ROLLER_X(width, height) UI_LAYOUT_PX((width), (height), 34)
#define UI_LAYOUT_TEMP_CUSTOM_ROLLER_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 170)
#define UI_LAYOUT_TEMP_CUSTOM_ROLLER_HEIGHT(width, height) UI_LAYOUT_PX((width), (height), 42)
#define UI_LAYOUT_TEMP_CUSTOM_ROW1_Y(width, height) UI_LAYOUT_PX((width), (height), -72)
#define UI_LAYOUT_TEMP_CUSTOM_ROW2_Y(width, height) UI_LAYOUT_PX((width), (height), -18)
#define UI_LAYOUT_TEMP_CUSTOM_ROW3_Y(width, height) UI_LAYOUT_PX((width), (height), 36)
#define UI_LAYOUT_TEMP_CUSTOM_HINT_Y(width, height) UI_LAYOUT_PX((width), (height), 92)

#define UI_LAYOUT_EASTER_EGG_TITLE_Y(width, height) UI_LAYOUT_PX((width), (height), -70)
#define UI_LAYOUT_EASTER_EGG_INFO_WIDTH(width, height) UI_LAYOUT_PX((width), (height), 280)
#define UI_LAYOUT_EASTER_EGG_INFO_Y(width, height) UI_LAYOUT_PX((width), (height), -5)
#define UI_LAYOUT_EASTER_EGG_INFO_LINE_SPACE(width, height) UI_LAYOUT_PX((width), (height), 2)
#define UI_LAYOUT_EASTER_EGG_HINT_Y(width, height) UI_LAYOUT_PX((width), (height), -58)

typedef struct {
    uint16_t ring_diameter;
    uint16_t ring_arc_width;
    int16_t black_ear_offset_y;
} ui_round_shell_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    int16_t title_y;
    uint16_t spinner_size;
    int16_t spinner_x;
    int16_t spinner_y;
    int16_t status_y;
    int16_t saved_header_y;
    uint16_t saved_panel_width;
    uint16_t saved_panel_height;
    int16_t saved_panel_y;
    uint16_t saved_panel_radius;
    uint16_t saved_panel_pad;
    int16_t saved_name_x;
    uint16_t delete_button_width;
    uint16_t delete_button_height;
    int16_t delete_button_x;
    uint16_t delete_button_radius;
    uint16_t delete_button_pad;
    uint16_t divider_width;
    int16_t divider_y;
    int16_t nearby_y;
    uint16_t list_width;
    uint16_t list_height;
    int16_t list_y;
    uint16_t list_radius;
    uint16_t list_pad;
    int16_t hint_y;
    uint16_t spinner_arc_width;
} ui_ble_scan_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    int16_t title_y;
    int16_t label_page_y;
    uint16_t roller_width;
    uint16_t roller_radius;
    int16_t roller_page_y;
    uint16_t divider_width;
    int16_t divider1_y;
    int16_t label_vehicle_y;
    int16_t roller_vehicle_y;
    int16_t divider2_y;
    int16_t label_poll_y;
    int16_t roller_poll_y;
    int16_t divider3_y;
    int16_t label_rotation_y;
    int16_t roller_rotation_y;
    int16_t divider4_y;
    int16_t label_brightness_y;
    uint16_t slider_width;
    uint16_t slider_height;
    int16_t slider_y;
    uint16_t slider_knob_pad;
    int16_t brightness_value_x;
    int16_t brightness_value_y;
    int16_t hint_y;
} ui_settings_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    int16_t title_y;
    uint16_t dot_size;
    int16_t dot_x;
    int16_t dot_y;
    uint16_t value_width;
    int16_t value_x;
    int16_t value_y;
    int16_t unit_x;
    int16_t unit_y;
    uint16_t trend_width;
    uint16_t trend_height;
    int16_t trend_y;
    uint16_t trend_radius;
    uint16_t chart_line_width;
    uint16_t tick_line_width;
} ui_brake_temp_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    int16_t title_y;
    uint16_t dot_size;
    int16_t dot_x;
    int16_t dot_y;
    uint16_t value_width;
    int16_t value_x;
    int16_t value_y;
    int16_t unit_x;
    int16_t unit_y;
    uint16_t trend_width;
    uint16_t trend_height;
    int16_t trend_y;
    uint16_t trend_radius;
    uint16_t chart_line_width;
    uint16_t tick_line_width;
} ui_oil_pressure_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    int16_t tile_name_dy;
    int16_t tile_value_dy;
    int16_t tile_unit_dy;
    uint16_t tile_value_width;
    int16_t title_y;
    int16_t hdiv1_y;
    uint16_t hdiv1_width;
    int16_t hdiv2_y;
    uint16_t hdiv2_width;
    int16_t vdiv_x;
    int16_t vdiv_y;
    uint16_t vdiv_height;
    int16_t row1_cy;
    int16_t row2_cy;
    int16_t row3_cy;
    int16_t left_col_x;
    int16_t right_col_x;
    int16_t center_col_x;
} ui_info_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    uint16_t dot_size;
    uint16_t value_width;
    int16_t value_x;
    int16_t dot_x;
    int16_t name_x;
    int16_t unit_x;
    uint16_t divider_width;
    int16_t row1_cy;
    int16_t divider1_y;
    int16_t row2_cy;
    int16_t divider2_y;
    int16_t row3_cy;
    uint16_t inner_arc_diameter;
    uint16_t inner_arc_width;
} ui_temp_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    uint16_t meter_size;
    uint16_t tick_width;
    uint16_t tick_length;
    uint16_t major_tick_width;
    uint16_t major_tick_length;
    uint16_t major_label_gap;
    uint16_t indicator_width;
    int16_t indicator_r_mod;
    int16_t name_y;
    int16_t value_y;
    int16_t unit_y;
} ui_needle_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    int16_t title_y;
    uint16_t roller_width;
    uint16_t roller_radius;
    int16_t roller_y;
    int16_t hint_y;
} ui_needle_config_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    int16_t title_y;
    int16_t subtitle_y;
    int16_t value_y;
    uint16_t slider_width;
    uint16_t slider_height;
    int16_t slider_y;
    uint16_t slider_knob_pad;
    int16_t range_y;
    int16_t hint_y;
} ui_warn_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    int16_t sky_y;
    int16_t gauge_y;
    int16_t sky_letter_space;
    int16_t gauge_letter_space;
} ui_logo_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    uint16_t inner_arc_diameter;
    uint16_t inner_arc_width;
    uint16_t roller_height;
    uint16_t roller_radius;
    uint16_t selected_radius;
    int16_t title_y;
    int16_t hint_x;
    int16_t hint_y;
} ui_obd_protocol_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    int16_t title_y;
    int16_t label_x;
    int16_t roller_x;
    uint16_t roller_width;
    uint16_t roller_height;
    int16_t row_y[5];
    int16_t hint_y;
} ui_info_custom_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    int16_t title_y;
    int16_t label_x;
    int16_t roller_x;
    uint16_t roller_width;
    uint16_t roller_height;
    int16_t row_y[3];
    int16_t hint_y;
} ui_temp_custom_layout_t;

typedef struct {
    ui_round_shell_layout_t shell;
    int16_t title_y;
    uint16_t info_width;
    int16_t info_y;
    int16_t info_line_space;
    int16_t hint_y;
} ui_easter_egg_layout_t;

int16_t ui_layout_px(int16_t base_px);
void ui_round_shell_layout_get(uint16_t base_ring_arc_width, ui_round_shell_layout_t *layout);
void ui_ble_scan_layout_get(ui_ble_scan_layout_t *layout);
void ui_settings_layout_get(ui_settings_layout_t *layout);
void ui_brake_temp_layout_get(ui_brake_temp_layout_t *layout);
void ui_oil_pressure_layout_get(ui_oil_pressure_layout_t *layout);
void ui_info_layout_get(ui_info_layout_t *layout);
void ui_temp_layout_get(ui_temp_layout_t *layout);
void ui_needle_layout_get(ui_needle_layout_t *layout);
void ui_needle_config_layout_get(ui_needle_config_layout_t *layout);
void ui_warn_layout_get(ui_warn_layout_t *layout);
void ui_logo_layout_get(ui_logo_layout_t *layout);
void ui_obd_protocol_layout_get(ui_obd_protocol_layout_t *layout);
void ui_info_custom_layout_get(ui_info_custom_layout_t *layout);
void ui_temp_custom_layout_get(ui_temp_custom_layout_t *layout);
void ui_easter_egg_layout_get(ui_easter_egg_layout_t *layout);
