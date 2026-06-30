#include "../../main/app_obd_dsp/default_page_ids.h"

_Static_assert(DEFAULT_PAGE_TEMP == 0u, "Temp page index should stay stable for NVS");
_Static_assert(DEFAULT_PAGE_INFO == 1u, "Info page index should stay stable for NVS");
_Static_assert(DEFAULT_PAGE_BRAKE_TEMP == 2u, "Brake page index should stay stable for NVS");
_Static_assert(DEFAULT_PAGE_OIL_PRESSURE == 3u, "Oil pressure page index should stay stable for NVS");
_Static_assert(DEFAULT_PAGE_NEEDLE == 4u, "Needle page index should stay stable for NVS");
_Static_assert(DEFAULT_PAGE_COUNT == 5u, "Default page count should match the current UI roller options");
_Static_assert(DEFAULT_PAGE_NEEDLE + 1u == DEFAULT_PAGE_COUNT,
               "Default page indexes should stay contiguous for direct roller/NVS mapping");
