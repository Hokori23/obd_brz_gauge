#include "../../main/bsp_obd_dsp/boards/board_ws_175_amoled_spec.h"

_Static_assert(sizeof(BOARD_WS_175_AMOLED_NAME) == sizeof("Waveshare ESP32-S3-Touch-AMOLED-1.75"),
               "WS175 board name contract should stay stable");
_Static_assert(BOARD_WS_175_AMOLED_H_RES == 466, "WS175 horizontal resolution should stay 466");
_Static_assert(BOARD_WS_175_AMOLED_V_RES == 466, "WS175 vertical resolution should stay 466");
_Static_assert(BOARD_WS_175_AMOLED_COLOR_BITS == 16, "WS175 color depth should stay RGB565");
_Static_assert(BOARD_WS_175_AMOLED_DRAW_BUFFER_LINES == 50, "WS175 draw buffer lines should stay board-defined");
_Static_assert(BOARD_WS_175_AMOLED_HAS_TOUCH == 1, "WS175 should keep touch enabled");
_Static_assert(BOARD_WS_175_AMOLED_BRIGHTNESS_CMD == 0x51, "WS175 should keep using register 0x51 for brightness");
_Static_assert(BOARD_WS_175_AMOLED_LCD_GAP_X == 0x06, "WS175 should preserve the validated panel X gap");
_Static_assert(BOARD_WS_175_AMOLED_LCD_GAP_Y == 0x00, "WS175 should preserve the validated panel Y gap");
_Static_assert(BOARD_WS_175_AMOLED_LCD_CASET_X0 == 0x0006, "WS175 CASET start should match the panel-visible X origin");
_Static_assert(BOARD_WS_175_AMOLED_LCD_CASET_X1 == 0x01D7, "WS175 CASET end should match the 466px visible width");
_Static_assert(BOARD_WS_175_AMOLED_LCD_RASET_Y0 == 0x0000, "WS175 RASET start should begin at the top row");
_Static_assert(BOARD_WS_175_AMOLED_LCD_RASET_Y1 == 0x01D1, "WS175 RASET end should match the 466px visible height");
_Static_assert((BOARD_WS_175_AMOLED_LCD_CASET_X1 - BOARD_WS_175_AMOLED_LCD_CASET_X0 + 1) == BOARD_WS_175_AMOLED_H_RES,
               "WS175 visible CASET window should span the full advertised horizontal resolution");
_Static_assert((BOARD_WS_175_AMOLED_LCD_RASET_Y1 - BOARD_WS_175_AMOLED_LCD_RASET_Y0 + 1) == BOARD_WS_175_AMOLED_V_RES,
               "WS175 visible RASET window should span the full advertised vertical resolution");
_Static_assert(BOARD_WS_175_AMOLED_BRIGHTNESS_PARAM(0) == 0, "WS175 brightness 0 percent should map to 0x00");
_Static_assert(BOARD_WS_175_AMOLED_BRIGHTNESS_PARAM(1) == 2, "WS175 brightness should round down consistently at 1 percent");
_Static_assert(BOARD_WS_175_AMOLED_BRIGHTNESS_PARAM(50) == 127, "WS175 brightness 50 percent should map to mid-scale");
_Static_assert(BOARD_WS_175_AMOLED_BRIGHTNESS_PARAM(100) == 255, "WS175 brightness 100 percent should map to 0xFF");
_Static_assert(BOARD_WS_175_AMOLED_TRANSFER_BYTES(1) == (466u * 2u), "One line transfer should be one RGB565 row");
_Static_assert(BOARD_WS_175_AMOLED_TRANSFER_BYTES(20) == (466u * 20u * 2u),
               "Twenty lines transfer should match the LVGL DMA buffer footprint");
_Static_assert(BOARD_WS_175_AMOLED_TRANSFER_BYTES(BOARD_WS_175_AMOLED_DRAW_BUFFER_LINES) ==
                   ((uint32_t)BOARD_WS_175_AMOLED_H_RES * (uint32_t)BOARD_WS_175_AMOLED_DRAW_BUFFER_LINES * sizeof(uint16_t)),
               "WS175 draw buffer transfer bytes should stay aligned with the runtime LVGL buffer contract");
_Static_assert(BOARD_WS_175_AMOLED_DISPLAY_ROTATION == 180,
               "WS175 display should stay on the validated 180-degree rotation default");
_Static_assert(BOARD_WS_175_AMOLED_TOUCH_SWAP_XY == 0, "WS175 touch should not swap axes by default");
_Static_assert(BOARD_WS_175_AMOLED_TOUCH_MIRROR_X == 1, "WS175 touch should mirror X by default");
_Static_assert(BOARD_WS_175_AMOLED_TOUCH_MIRROR_Y == 1, "WS175 touch should mirror Y by default");
