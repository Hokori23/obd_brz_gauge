#include "../../main/bsp_obd_dsp/nvs_storage.h"

_Static_assert(NVS_ERROR_LOG_CAPACITY == 20u,
               "NVS error log should keep the latest 20 entries");
_Static_assert(NVS_ERROR_LOG_VERSION == 1u,
               "NVS error log version should stay stable for migration");
_Static_assert(sizeof(((nvs_error_entry_t *)0)->tag) == NVS_ERROR_TAG_LEN,
               "Error log tag buffer size should match the public contract");
_Static_assert(sizeof(((nvs_error_entry_t *)0)->message) == NVS_ERROR_MSG_LEN,
               "Error log message buffer size should match the public contract");
