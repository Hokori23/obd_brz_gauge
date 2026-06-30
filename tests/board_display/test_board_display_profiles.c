#include "../../main/bsp_obd_dsp/boards/board_ws_175_amoled_spec.h"
#include "../../main/bsp_obd_dsp/boards/board_ws_185_spec.h"

_Static_assert(BOARD_WS_185_H_RES == 360,
               "WS185 should preserve the validated 360x360 resolution");
_Static_assert(BOARD_WS_185_V_RES == 360,
               "WS185 should preserve the validated 360x360 resolution");
_Static_assert(BOARD_WS_185_COLOR_BITS == 16,
               "WS185 should preserve the validated 16-bit color depth");
_Static_assert(BOARD_WS_185_DRAW_BUFFER_LINES == 20,
               "WS185 should keep the existing 20-line LVGL draw buffer");
_Static_assert(BOARD_WS_185_HAS_TOUCH == 1,
               "WS185 should keep touch enabled in its board profile contract");
_Static_assert(BOARD_WS_185_TOUCH_SWAP_XY == 0,
               "WS185 should keep the existing touch axis orientation");
_Static_assert(BOARD_WS_185_TOUCH_MIRROR_X == 0,
               "WS185 should keep the existing touch X mirror setting");
_Static_assert(BOARD_WS_185_TOUCH_MIRROR_Y == 0,
               "WS185 should keep the existing touch Y mirror setting");

_Static_assert(BOARD_WS_175_AMOLED_H_RES == 466,
               "WS175 should preserve the validated 466x466 resolution");
_Static_assert(BOARD_WS_175_AMOLED_V_RES == 466,
               "WS175 should preserve the validated 466x466 resolution");
_Static_assert(BOARD_WS_175_AMOLED_COLOR_BITS == 16,
               "WS175 should preserve the validated 16-bit color depth");
_Static_assert(BOARD_WS_175_AMOLED_DRAW_BUFFER_LINES == 20,
               "WS175 should keep the validated 20-line LVGL draw buffer");
_Static_assert(BOARD_WS_175_AMOLED_HAS_TOUCH == 1,
               "WS175 should keep touch enabled in its board profile contract");
_Static_assert(BOARD_WS_175_AMOLED_TOUCH_SWAP_XY == 0,
               "WS175 should keep the validated touch axis orientation");
_Static_assert(BOARD_WS_175_AMOLED_TOUCH_MIRROR_X == 1,
               "WS175 should keep the validated touch X mirror setting");
_Static_assert(BOARD_WS_175_AMOLED_TOUCH_MIRROR_Y == 1,
               "WS175 should keep the validated touch Y mirror setting");
