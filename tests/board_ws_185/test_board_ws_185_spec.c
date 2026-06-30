#include "../../main/bsp_obd_dsp/boards/board_ws_185_spec.h"

_Static_assert(sizeof(BOARD_WS_185_NAME) == sizeof("Waveshare ESP32-S3-Touch-LCD-1.85"),
               "WS185 board name contract should stay stable");
_Static_assert(BOARD_WS_185_H_RES == 360, "WS185 horizontal resolution should stay 360");
_Static_assert(BOARD_WS_185_V_RES == 360, "WS185 vertical resolution should stay 360");
_Static_assert(BOARD_WS_185_COLOR_BITS == 16, "WS185 color depth should stay RGB565");
_Static_assert(BOARD_WS_185_DRAW_BUFFER_LINES == 20, "WS185 draw buffer lines should stay board-defined");
_Static_assert(BOARD_WS_185_HAS_TOUCH == 1, "WS185 should keep touch enabled");
_Static_assert(BOARD_WS_185_TOUCH_SWAP_XY == 0, "WS185 touch should not swap axes by default");
_Static_assert(BOARD_WS_185_TOUCH_MIRROR_X == 0, "WS185 touch should not mirror X by default");
_Static_assert(BOARD_WS_185_TOUCH_MIRROR_Y == 0, "WS185 touch should not mirror Y by default");
