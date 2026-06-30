#include "../../main/export_path/ui_layout.h"

_Static_assert(UI_LAYOUT_ROUND_SCREEN_DIAMETER(360, 360) == 360,
    "base round shell diameter should remain 360");
_Static_assert(UI_LAYOUT_ROUND_SCREEN_DIAMETER(466, 466) == 466,
    "466 round shell diameter should expand to full panel");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(360, 360, 10) == 10,
    "base ring arc width should remain unchanged");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 10) == 13,
    "466 ring arc width should scale up");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(360, 360, 8) == 8,
    "base narrow ring arc width should remain unchanged");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 8) == 10,
    "466 narrow ring arc width should scale up");
_Static_assert(UI_LAYOUT_BLACK_EAR_OFFSET_Y(360, 360) == -142,
    "base black ear offset should remain unchanged");
_Static_assert(UI_LAYOUT_BLACK_EAR_OFFSET_Y(466, 466) == -184,
    "466 black ear offset should scale outward");

_Static_assert(UI_LAYOUT_BLE_TITLE_Y(360, 360) == 18,
    "base BLE title y should remain unchanged");
_Static_assert(UI_LAYOUT_BLE_TITLE_Y(466, 466) == 23,
    "466 BLE title y should scale");
_Static_assert(UI_LAYOUT_ROUND_SCREEN_DIAMETER(466, 466) == 466,
    "466 BLE shell diameter should follow the round screen diameter");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 10) == 13,
    "466 BLE shell ring width should scale from its 10px base");
_Static_assert(UI_LAYOUT_BLE_SPINNER_SIZE(466, 466) == 31,
    "466 BLE spinner size should scale");
_Static_assert(UI_LAYOUT_BLE_STATUS_Y(466, 466) == 65,
    "466 BLE status y should scale");
_Static_assert(UI_LAYOUT_BLE_SAVED_PANEL_WIDTH(466, 466) == 342,
    "466 BLE saved panel width should scale");
_Static_assert(UI_LAYOUT_BLE_SAVED_PANEL_HEIGHT(466, 466) == 41,
    "466 BLE saved panel height should scale");
_Static_assert(UI_LAYOUT_BLE_DIVIDER_WIDTH(466, 466) == 311,
    "466 BLE divider width should scale");
_Static_assert(UI_LAYOUT_BLE_LIST_WIDTH(466, 466) == 342,
    "466 BLE list width should scale");
_Static_assert(UI_LAYOUT_BLE_LIST_HEIGHT(466, 466) == 188,
    "466 BLE list height should scale");
_Static_assert(UI_LAYOUT_BLE_HINT_Y(466, 466) == -19,
    "466 BLE hint inset should scale");

_Static_assert(UI_LAYOUT_SETTINGS_TITLE_Y(466, 466) == -179,
    "466 settings title offset should scale");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 10) == 13,
    "466 settings shell ring width should scale from its 10px base");
_Static_assert(UI_LAYOUT_SETTINGS_ROLLER_WIDTH(466, 466) == 181,
    "466 settings roller width should scale");
_Static_assert(UI_LAYOUT_SETTINGS_DIVIDER_WIDTH(466, 466) == 285,
    "466 settings divider width should scale");
_Static_assert(UI_LAYOUT_SETTINGS_DIV1_Y(360, 360) == -54,
    "base settings first divider y should remain unchanged");
_Static_assert(UI_LAYOUT_SETTINGS_DIV1_Y(466, 466) == -70,
    "466 settings first divider y should scale");
_Static_assert(UI_LAYOUT_SETTINGS_DIV2_Y(360, 360) == 26,
    "base settings second divider y should remain unchanged");
_Static_assert(UI_LAYOUT_SETTINGS_DIV2_Y(466, 466) == 34,
    "466 settings second divider y should scale");
_Static_assert(UI_LAYOUT_SETTINGS_SLIDER_WIDTH(466, 466) == 233,
    "466 settings slider width should scale");
_Static_assert(UI_LAYOUT_SETTINGS_SLIDER_HEIGHT(466, 466) == 13,
    "466 settings slider height should scale");
_Static_assert(UI_LAYOUT_SETTINGS_HINT_Y(466, 466) == 161,
    "466 settings hint y should scale");

_Static_assert(UI_LAYOUT_BRAKE_TEMP_TITLE_Y(466, 466) == -158,
    "466 brake temp title offset should scale");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 8) == 10,
    "466 brake temp shell ring width should scale from its 8px base");
_Static_assert(UI_LAYOUT_BRAKE_TEMP_DOT_SIZE(466, 466) == 13,
    "466 brake temp dot size should scale");
_Static_assert(UI_LAYOUT_BRAKE_TEMP_VALUE_WIDTH(466, 466) == 223,
    "466 brake temp value width should scale");
_Static_assert(UI_LAYOUT_BRAKE_TEMP_TREND_WIDTH(466, 466) == 352,
    "466 brake temp trend width should scale");
_Static_assert(UI_LAYOUT_BRAKE_TEMP_TREND_HEIGHT(466, 466) == 150,
    "466 brake temp trend height should scale");
_Static_assert(UI_LAYOUT_BRAKE_TEMP_TREND_RADIUS(466, 466) == 62,
    "466 brake temp trend radius should scale");
_Static_assert(UI_LAYOUT_BRAKE_TEMP_CHART_LINE_WIDTH(466, 466) == 4,
    "466 brake temp chart line width should scale");

_Static_assert(UI_LAYOUT_OIL_PRESSURE_TITLE_Y(466, 466) == -158,
    "466 oil pressure title offset should scale");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 8) == 10,
    "466 oil pressure shell ring width should scale from its 8px base");
_Static_assert(UI_LAYOUT_OIL_PRESSURE_DOT_SIZE(466, 466) == 13,
    "466 oil pressure dot size should scale");
_Static_assert(UI_LAYOUT_OIL_PRESSURE_VALUE_WIDTH(466, 466) == 285,
    "466 oil pressure value width should scale");
_Static_assert(UI_LAYOUT_OIL_PRESSURE_VALUE_Y(466, 466) == -26,
    "466 oil pressure value y should scale");
_Static_assert(UI_LAYOUT_OIL_PRESSURE_UNIT_Y(466, 466) == 39,
    "466 oil pressure unit y should scale");
_Static_assert(UI_LAYOUT_OIL_PRESSURE_TREND_WIDTH(466, 466) == 352,
    "466 oil pressure trend width should scale");
_Static_assert(UI_LAYOUT_OIL_PRESSURE_TREND_HEIGHT(466, 466) == 150,
    "466 oil pressure trend height should scale");
_Static_assert(UI_LAYOUT_OIL_PRESSURE_TREND_Y(466, 466) == 106,
    "466 oil pressure trend y should scale");
_Static_assert(UI_LAYOUT_OIL_PRESSURE_CHART_LINE_WIDTH(466, 466) == 4,
    "466 oil pressure chart line width should scale");

_Static_assert(UI_LAYOUT_INFO_TILE_NAME_DY(466, 466) == -36,
    "466 info tile name offset should scale");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 8) == 10,
    "466 info shell ring width should scale from its 8px base");
_Static_assert(UI_LAYOUT_INFO_TILE_VALUE_DY(466, 466) == 3,
    "466 info tile value offset should scale");
_Static_assert(UI_LAYOUT_INFO_TILE_UNIT_DY(466, 466) == 44,
    "466 info tile unit offset should scale");
_Static_assert(UI_LAYOUT_INFO_TILE_VALUE_WIDTH(466, 466) == 155,
    "466 info tile value width should scale");
_Static_assert(UI_LAYOUT_INFO_TITLE_Y(466, 466) == -192,
    "466 info title offset should scale");
_Static_assert(UI_LAYOUT_INFO_HDIV1_WIDTH(466, 466) == 337,
    "466 info first divider width should scale");
_Static_assert(UI_LAYOUT_INFO_HDIV2_WIDTH(466, 466) == 259,
    "466 info second divider width should scale");
_Static_assert(UI_LAYOUT_INFO_VDIV_HEIGHT(466, 466) == 254,
    "466 info vertical divider height should scale");
_Static_assert(UI_LAYOUT_INFO_ROW1_CY(466, 466) == -109,
    "466 info row1 center should scale");
_Static_assert(UI_LAYOUT_INFO_ROW2_CY(466, 466) == 28,
    "466 info row2 center should scale");
_Static_assert(UI_LAYOUT_INFO_ROW3_CY(466, 466) == 145,
    "466 info row3 center should scale");
_Static_assert(UI_LAYOUT_INFO_LEFT_COL_X(466, 466) == -106,
    "466 info left column x should scale");
_Static_assert(UI_LAYOUT_INFO_RIGHT_COL_X(466, 466) == 106,
    "466 info right column x should scale");

_Static_assert(UI_LAYOUT_TEMP_DOT_SIZE(466, 466) == 13,
    "466 temp dot size should scale");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 10) == 13,
    "466 temp shell ring width should scale from its 10px base");
_Static_assert(UI_LAYOUT_TEMP_VALUE_WIDTH(466, 466) == 142,
    "466 temp value width should scale");
_Static_assert(UI_LAYOUT_TEMP_VALUE_X(466, 466) == 91,
    "466 temp value x should scale");
_Static_assert(UI_LAYOUT_TEMP_DOT_X(466, 466) == 239,
    "466 temp dot x should scale");
_Static_assert(UI_LAYOUT_TEMP_NAME_X(466, 466) == 259,
    "466 temp name x should scale");
_Static_assert(UI_LAYOUT_TEMP_UNIT_X(466, 466) == -91,
    "466 temp unit x should scale");
_Static_assert(UI_LAYOUT_TEMP_DIVIDER_WIDTH(466, 466) == 285,
    "466 temp divider width should scale");
_Static_assert(UI_LAYOUT_TEMP_ROW1_CY(466, 466) == -84,
    "466 temp row1 center should scale");
_Static_assert(UI_LAYOUT_TEMP_DIV1_Y(466, 466) == -39,
    "466 temp divider1 y should scale");
_Static_assert(UI_LAYOUT_TEMP_ROW2_CY(466, 466) == 6,
    "466 temp row2 center should scale");
_Static_assert(UI_LAYOUT_TEMP_DIV2_Y(466, 466) == 52,
    "466 temp divider2 y should scale");
_Static_assert(UI_LAYOUT_TEMP_ROW3_CY(466, 466) == 97,
    "466 temp row3 center should scale");
_Static_assert(UI_LAYOUT_TEMP_INNER_ARC_DIAMETER(466, 466) == 440,
    "466 temp inner arc diameter should scale");
_Static_assert(UI_LAYOUT_TEMP_INNER_ARC_WIDTH(466, 466) == 26,
    "466 temp inner arc width should scale");
_Static_assert(UI_LAYOUT_BLACK_EAR_OFFSET_Y(466, 466) == -184,
    "466 temp black ear offset should share the round shell scaling");

_Static_assert(UI_LAYOUT_NEEDLE_METER_SIZE(360, 360) == 320,
    "base needle meter size should remain unchanged");
_Static_assert(UI_LAYOUT_NEEDLE_METER_SIZE(466, 466) == 414,
    "466 needle meter size should scale");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 8) == 10,
    "466 needle shell ring width should scale from its 8px base");
_Static_assert(UI_LAYOUT_NEEDLE_TICK_WIDTH(466, 466) == 3,
    "466 needle tick width should scale");
_Static_assert(UI_LAYOUT_NEEDLE_TICK_LENGTH(466, 466) == 10,
    "466 needle tick length should scale");
_Static_assert(UI_LAYOUT_NEEDLE_MAJOR_TICK_LENGTH(466, 466) == 18,
    "466 needle major tick length should scale");
_Static_assert(UI_LAYOUT_NEEDLE_INDICATOR_R_MOD(466, 466) == -91,
    "466 needle indicator inset should scale");
_Static_assert(UI_LAYOUT_NEEDLE_NAME_Y(466, 466) == -67,
    "466 needle name y should scale");
_Static_assert(UI_LAYOUT_NEEDLE_VALUE_Y(466, 466) == 70,
    "466 needle value y should scale");
_Static_assert(UI_LAYOUT_NEEDLE_UNIT_Y(466, 466) == 122,
    "466 needle unit y should scale");

_Static_assert(UI_LAYOUT_NEEDLE_CONFIG_TITLE_Y(466, 466) == -142,
    "466 needle config title y should scale");
_Static_assert(UI_LAYOUT_ROUND_SCREEN_DIAMETER(466, 466) == 466,
    "466 needle config screen radius basis should match round shell diameter");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 0) == 0,
    "needle config keeps a zero-width shell ring when scaling");
_Static_assert(UI_LAYOUT_NEEDLE_CONFIG_ROLLER_WIDTH(466, 466) == 207,
    "466 needle config roller width should scale");
_Static_assert(UI_LAYOUT_NEEDLE_CONFIG_ROLLER_RADIUS(466, 466) == 10,
    "466 needle config roller radius should scale");
_Static_assert(UI_LAYOUT_NEEDLE_CONFIG_ROLLER_Y(466, 466) == 10,
    "466 needle config roller y should scale");
_Static_assert(UI_LAYOUT_NEEDLE_CONFIG_HINT_Y(466, 466) == 155,
    "466 needle config hint y should scale");

_Static_assert(UI_LAYOUT_WARN_TITLE_Y(466, 466) == -158,
    "466 warn title y should scale");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 10) == 13,
    "466 warn shell ring width should scale from its 10px base");
_Static_assert(UI_LAYOUT_WARN_SUBTITLE_Y(466, 466) == -114,
    "466 warn subtitle y should scale");
_Static_assert(UI_LAYOUT_WARN_VALUE_Y(466, 466) == -31,
    "466 warn value y should scale");
_Static_assert(UI_LAYOUT_WARN_SLIDER_WIDTH(466, 466) == 285,
    "466 warn slider width should scale");
_Static_assert(UI_LAYOUT_WARN_SLIDER_HEIGHT(466, 466) == 16,
    "466 warn slider height should scale");
_Static_assert(UI_LAYOUT_WARN_SLIDER_Y(466, 466) == 34,
    "466 warn slider y should scale");
_Static_assert(UI_LAYOUT_WARN_SLIDER_KNOB_PAD(466, 466) == 8,
    "466 warn slider knob pad should scale");
_Static_assert(UI_LAYOUT_WARN_RANGE_Y(466, 466) == 75,
    "466 warn range y should scale");
_Static_assert(UI_LAYOUT_WARN_HINT_Y(466, 466) == 142,
    "466 warn hint y should scale");

_Static_assert(UI_LAYOUT_LOGO_SKY_Y(466, 466) == -26,
    "466 logo sky y should scale");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 10) == 13,
    "466 logo shell ring width should scale from its 10px base");
_Static_assert(UI_LAYOUT_LOGO_GAUGE_Y(466, 466) == 39,
    "466 logo gauge y should scale");
_Static_assert(UI_LAYOUT_LOGO_SKY_LETTER_SPACE(360, 360) == 8,
    "base logo sky letter spacing should remain unchanged");
_Static_assert(UI_LAYOUT_LOGO_SKY_LETTER_SPACE(466, 466) == 10,
    "466 logo sky letter spacing should scale");
_Static_assert(UI_LAYOUT_LOGO_GAUGE_LETTER_SPACE(360, 360) == 12,
    "base logo gauge letter spacing should remain unchanged");
_Static_assert(UI_LAYOUT_LOGO_GAUGE_LETTER_SPACE(466, 466) == 16,
    "466 logo gauge letter spacing should scale");

_Static_assert(UI_LAYOUT_OBD_PROTOCOL_INNER_ARC_DIAMETER(466, 466) == 440,
    "466 OBD protocol inner arc diameter should scale");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 10) == 13,
    "466 OBD protocol shell ring width should scale from its 10px base");
_Static_assert(UI_LAYOUT_OBD_PROTOCOL_INNER_ARC_WIDTH(466, 466) == 26,
    "466 OBD protocol inner arc width should scale");
_Static_assert(UI_LAYOUT_OBD_PROTOCOL_ROLLER_HEIGHT(466, 466) == 194,
    "466 OBD protocol roller height should scale");
_Static_assert(UI_LAYOUT_OBD_PROTOCOL_ROLLER_RADIUS(466, 466) == 13,
    "466 OBD protocol roller radius should scale");
_Static_assert(UI_LAYOUT_OBD_PROTOCOL_SELECTED_RADIUS(466, 466) == 6,
    "466 OBD protocol selected radius should scale");
_Static_assert(UI_LAYOUT_OBD_PROTOCOL_TITLE_Y(466, 466) == -157,
    "466 OBD protocol title y should scale");
_Static_assert(UI_LAYOUT_OBD_PROTOCOL_HINT_X(466, 466) == -3,
    "466 OBD protocol hint x should scale");
_Static_assert(UI_LAYOUT_OBD_PROTOCOL_HINT_Y(466, 466) == 166,
    "466 OBD protocol hint y should scale");
_Static_assert(UI_LAYOUT_BLACK_EAR_OFFSET_Y(466, 466) == -184,
    "466 OBD protocol black ear offset should share the round shell scaling");

_Static_assert(UI_LAYOUT_INFO_CUSTOM_TITLE_Y(466, 466) == -171,
    "466 info custom title y should scale");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 10) == 13,
    "466 info custom shell ring width should scale from its 10px base");
_Static_assert(UI_LAYOUT_INFO_CUSTOM_LABEL_X(466, 466) == -129,
    "466 info custom label x should scale");
_Static_assert(UI_LAYOUT_INFO_CUSTOM_ROLLER_X(466, 466) == 39,
    "466 info custom roller x should scale");
_Static_assert(UI_LAYOUT_INFO_CUSTOM_ROLLER_WIDTH(466, 466) == 220,
    "466 info custom roller width should scale");
_Static_assert(UI_LAYOUT_INFO_CUSTOM_ROLLER_HEIGHT(466, 466) == 49,
    "466 info custom roller height should scale");
_Static_assert(UI_LAYOUT_INFO_CUSTOM_ROW1_Y(466, 466) == -106,
    "466 info custom row1 y should scale");
_Static_assert(UI_LAYOUT_INFO_CUSTOM_ROW5_Y(466, 466) == 122,
    "466 info custom row5 y should scale");
_Static_assert(UI_LAYOUT_INFO_CUSTOM_HINT_Y(466, 466) == -28,
    "466 info custom hint y should scale");

_Static_assert(UI_LAYOUT_TEMP_CUSTOM_TITLE_Y(466, 466) == -171,
    "466 temp custom title y should scale");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 10) == 13,
    "466 temp custom shell ring width should scale from its 10px base");
_Static_assert(UI_LAYOUT_TEMP_CUSTOM_LABEL_X(466, 466) == -119,
    "466 temp custom label x should scale");
_Static_assert(UI_LAYOUT_TEMP_CUSTOM_ROLLER_X(466, 466) == 44,
    "466 temp custom roller x should scale");
_Static_assert(UI_LAYOUT_TEMP_CUSTOM_ROLLER_WIDTH(466, 466) == 220,
    "466 temp custom roller width should scale");
_Static_assert(UI_LAYOUT_TEMP_CUSTOM_ROLLER_HEIGHT(466, 466) == 54,
    "466 temp custom roller height should scale");
_Static_assert(UI_LAYOUT_TEMP_CUSTOM_ROW1_Y(466, 466) == -93,
    "466 temp custom row1 y should scale");
_Static_assert(UI_LAYOUT_TEMP_CUSTOM_ROW3_Y(466, 466) == 47,
    "466 temp custom row3 y should scale");
_Static_assert(UI_LAYOUT_TEMP_CUSTOM_HINT_Y(466, 466) == 119,
    "466 temp custom hint y should scale");

_Static_assert(UI_LAYOUT_EASTER_EGG_TITLE_Y(466, 466) == -91,
    "466 easter egg title y should scale");
_Static_assert(UI_LAYOUT_ROUND_RING_ARC_WIDTH(466, 466, 10) == 13,
    "466 easter egg shell ring width should scale from its 10px base");
_Static_assert(UI_LAYOUT_EASTER_EGG_INFO_WIDTH(466, 466) == 362,
    "466 easter egg info width should scale");
_Static_assert(UI_LAYOUT_EASTER_EGG_INFO_Y(466, 466) == -6,
    "466 easter egg info y should scale");
_Static_assert(UI_LAYOUT_EASTER_EGG_INFO_LINE_SPACE(360, 360) == 2,
    "base easter egg info line spacing should remain unchanged");
_Static_assert(UI_LAYOUT_EASTER_EGG_INFO_LINE_SPACE(466, 466) == 3,
    "466 easter egg info line spacing should scale");
_Static_assert(UI_LAYOUT_EASTER_EGG_HINT_Y(466, 466) == -75,
    "466 easter egg hint y should scale");
