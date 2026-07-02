#include "../../main/bsp_obd_dsp/nvs_error_log_logic.h"

#include <string.h>

int test_nvs_error_log_logic_dummy_symbol(void)
{
    nvs_error_log_t log = {0};

    nvs_error_log_logic_sanitize(&log);
    if (log.version != NVS_ERROR_LOG_VERSION || log.head != 0u || log.count != 0u || log.next_seq != 0u) {
        return 1;
    }

    nvs_error_log_logic_append(&log, 12u, -5, "ELM", "timeout");
    if (log.count != 1u || log.head != 1u || log.next_seq != 1u) {
        return 2;
    }
    if (log.entries[0].seq != 0u || log.entries[0].uptime_s != 12u || log.entries[0].err_code != -5) {
        return 3;
    }
    if (strcmp(log.entries[0].tag, "ELM") != 0 || strcmp(log.entries[0].message, "timeout") != 0) {
        return 4;
    }

    for (uint32_t i = 1u; i <= NVS_ERROR_LOG_CAPACITY; ++i) {
        nvs_error_log_logic_append(&log, 100u + i, (int32_t)i, "TAG", "msg");
    }
    if (log.count != NVS_ERROR_LOG_CAPACITY || log.head != 1u || log.next_seq != (uint32_t)(NVS_ERROR_LOG_CAPACITY + 1u)) {
        return 5;
    }
    if (log.entries[0].seq != NVS_ERROR_LOG_CAPACITY || log.entries[0].err_code != (int32_t)NVS_ERROR_LOG_CAPACITY) {
        return 6;
    }
    if (log.entries[1].seq != 1u) {
        return 7;
    }

    log.version = 99u;
    log.head = 99u;
    log.count = 99u;
    log.next_seq = 123u;
    memset(log.entries, 0xAB, sizeof(log.entries));
    nvs_error_log_logic_sanitize(&log);
    if (log.version != NVS_ERROR_LOG_VERSION || log.head != 0u || log.count != 0u || log.next_seq != 0u) {
        return 8;
    }

    return 0;
}
