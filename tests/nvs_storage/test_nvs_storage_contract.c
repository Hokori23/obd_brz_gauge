#include "../../main/app_obd_dsp/default_page_ids.h"

_Static_assert(DEFAULT_PAGE_MENU == 0u, "Menu page index should stay stable for NVS");
_Static_assert(DEFAULT_PAGE_GAUGE_1 == 1u, "Gauge page 1 should stay stable for NVS");
_Static_assert(DEFAULT_PAGE_GAUGE_8 == 8u, "Gauge page 8 should stay stable for NVS");
_Static_assert(DEFAULT_PAGE_COUNT == 9u, "Default page count should cover MENU plus up to 8 gauge pages");
_Static_assert(DEFAULT_PAGE_GAUGE_8 + 1u == DEFAULT_PAGE_COUNT,
               "Default page indexes should stay contiguous for direct roller/NVS mapping");
