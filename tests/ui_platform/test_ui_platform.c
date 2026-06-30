#include "../../main/export_path/ui_platform.h"

_Static_assert(UI_PLATFORM_CENTER_X(360) == 180, "360px base center x should stay unchanged");
_Static_assert(UI_PLATFORM_CENTER_Y(360) == 180, "360px base center y should stay unchanged");
_Static_assert(UI_PLATFORM_ROUND_RADIUS(360, 360) == 180, "360px base radius should stay unchanged");
_Static_assert(UI_PLATFORM_SAFE_MARGIN(360, 360) == 16, "Base safe margin should stay unchanged");
_Static_assert(UI_PLATFORM_SCALE_PX(360, 360, 36) == 36, "Base scale should be identity");
_Static_assert(UI_PLATFORM_SCALE_PX(360, 360, -36) == -36, "Negative identity scaling should be preserved");

_Static_assert(UI_PLATFORM_CENTER_X(466) == 233, "466px center x should be 233");
_Static_assert(UI_PLATFORM_CENTER_Y(466) == 233, "466px center y should be 233");
_Static_assert(UI_PLATFORM_ROUND_RADIUS(466, 466) == 233, "466px round radius should be 233");
_Static_assert(UI_PLATFORM_SAFE_MARGIN(466, 466) == 21, "466px safe margin should round to 21");
_Static_assert(UI_PLATFORM_SCALE_PX(466, 466, 10) == 13, "466px scaled 10 should round to 13");
_Static_assert(UI_PLATFORM_SCALE_PX(466, 466, 36) == 47, "466px scaled 36 should round to 47");
_Static_assert(UI_PLATFORM_SCALE_PX(466, 466, -36) == -47, "Negative scaling should be symmetric");

_Static_assert(UI_PLATFORM_CENTER_X(466) == 233, "Non-square width should still center correctly");
_Static_assert(UI_PLATFORM_CENTER_Y(360) == 180, "Non-square height should still center correctly");
_Static_assert(UI_PLATFORM_ROUND_RADIUS(466, 360) == 180, "Round radius should follow min dimension");
_Static_assert(UI_PLATFORM_SAFE_MARGIN(466, 360) == 16, "Safe margin should follow min dimension");
_Static_assert(UI_PLATFORM_SCALE_PX(466, 360, 100) == 100, "Scaling should use the min dimension");
