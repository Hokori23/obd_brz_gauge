#pragma once

#include <stdbool.h>

#define BOARD_RUNTIME_TOUCH_AVAILABLE(board_supports_touch, touch_init_ok) \
    ((bool)((board_supports_touch) && (touch_init_ok)))
