#include "../../main/export_path/ui_font_profile.h"

_Static_assert(UI_FONT_TYPODER_SIZE(360, 360, 16) == 16,
    "base Typoder 16 should remain unchanged");
_Static_assert(UI_FONT_TYPODER_SIZE(466, 466, 16) == 20,
    "466 Typoder 16 should upscale to 20");
_Static_assert(UI_FONT_TYPODER_SIZE(466, 466, 20) == 28,
    "466 Typoder 20 should upscale to 28");
_Static_assert(UI_FONT_TYPODER_SIZE(360, 360, 24) == 24,
    "base Typoder 24 should remain unchanged");
_Static_assert(UI_FONT_TYPODER_SIZE(466, 466, 24) == 32,
    "466 Typoder 24 should upscale to 32");
_Static_assert(UI_FONT_TYPODER_SIZE(360, 360, 32) == 32,
    "base Typoder 32 should remain unchanged");
_Static_assert(UI_FONT_TYPODER_SIZE(466, 466, 32) == 44,
    "466 Typoder 32 should upscale into the large-title bucket");
_Static_assert(UI_FONT_TYPODER_SIZE(466, 466, 28) == 36,
    "466 Typoder 28 should upscale to 36");
_Static_assert(UI_FONT_TYPODER_SIZE(466, 466, 36) == 44,
    "466 Typoder 36 should upscale to 44");
_Static_assert(UI_FONT_TYPODER_SIZE(466, 466, 44) == 56,
    "466 Typoder 44 should upscale to 56");
_Static_assert(UI_FONT_TYPODER_SIZE(466, 466, 56) == 56,
    "466 Typoder 56 should clamp to the largest available font");
_Static_assert(UI_FONT_TYPODER_SIZE(466, 466, 12) == 16,
    "466 Typoder 12 should still stay in the smallest Typoder bucket");

_Static_assert(UI_FONT_HINT_SIZE(360, 360, 12) == 12,
    "base hint 12 should remain unchanged");
_Static_assert(UI_FONT_HINT_SIZE(466, 466, 12) == 16,
    "466 hint 12 should upscale to 16");
_Static_assert(UI_FONT_HINT_SIZE(466, 466, 14) == 16,
    "466 hint 14 should stay in the large hint bucket");
_Static_assert(UI_FONT_HINT_SIZE(466, 466, 10) == 12,
    "466 hint 10 should remain in the compact bucket after rounding");
_Static_assert(UI_FONT_HINT_SIZE(320, 320, 12) == 12,
    "smaller displays should keep hint fonts compact");

_Static_assert(UI_FONT_BRAKE_TEMP_BASE_SIZE(999) == 32,
    "three-digit brake temp x10 values should use the largest base font");
_Static_assert(UI_FONT_BRAKE_TEMP_BASE_SIZE(1000) == 28,
    "four-digit brake temp x10 values should step down one font size");
_Static_assert(UI_FONT_BRAKE_TEMP_BASE_SIZE(-1000) == 28,
    "negative four-digit brake temp x10 values should follow absolute magnitude");
_Static_assert(UI_FONT_BRAKE_TEMP_BASE_SIZE(10000) == 24,
    "five-digit brake temp x10 values should use the most compact base font");
