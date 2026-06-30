#include "../../main/bsp_obd_dsp/boards/board_runtime_touch.h"

_Static_assert(BOARD_RUNTIME_TOUCH_AVAILABLE(true, true),
               "Runtime touch should stay enabled when the board supports touch and init succeeds");
_Static_assert(!BOARD_RUNTIME_TOUCH_AVAILABLE(true, false),
               "Runtime touch should disable cleanly when touch init fails");
_Static_assert(!BOARD_RUNTIME_TOUCH_AVAILABLE(false, true),
               "Runtime touch should stay disabled for boards that do not support touch");
_Static_assert(!BOARD_RUNTIME_TOUCH_AVAILABLE(false, false),
               "Runtime touch should stay disabled when neither capability nor init is present");
