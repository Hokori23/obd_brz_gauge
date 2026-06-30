#include "../../main/app_obd_dsp/lvgl_buffer_profile.h"

_Static_assert(LVGL_BUFFER_PIXEL_COUNT(360u, 20u) == 7200u,
               "360x20 LVGL buffer should keep the validated pixel count");
_Static_assert(LVGL_BUFFER_PIXEL_COUNT(466u, 20u) == 9320u,
               "466x20 LVGL buffer should match the WS175 draw buffer contract");
_Static_assert(LVGL_BUFFER_BYTE_COUNT(466u, 20u, 2u) == 18640u,
               "WS175 RGB565 single LVGL draw buffer should be 18,640 bytes");
_Static_assert(LVGL_BUFFER_BYTE_COUNT(466u, 20u, 4u) == 37280u,
               "Formula should scale correctly for larger pixel sizes");
