#ifndef BSP_OBD_DSP_NVS_ERROR_LOGIC_H
#define BSP_OBD_DSP_NVS_ERROR_LOGIC_H

#include <stdio.h>
#include <string.h>

#include "bsp_obd_dsp/nvs_storage.h"

static inline void nvs_error_log_logic_sanitize(nvs_error_log_t *log)
{
    if (log == NULL) {
        return;
    }

    if (log->version != NVS_ERROR_LOG_VERSION) {
        memset(log, 0, sizeof(*log));
        log->version = NVS_ERROR_LOG_VERSION;
        return;
    }

    if (log->head >= NVS_ERROR_LOG_CAPACITY) {
        log->head = 0u;
    }
    if (log->count > NVS_ERROR_LOG_CAPACITY) {
        log->count = NVS_ERROR_LOG_CAPACITY;
    }
}

static inline void nvs_error_log_logic_append(nvs_error_log_t *log,
                                              uint32_t uptime_s,
                                              int32_t err_code,
                                              const char *tag,
                                              const char *message)
{
    nvs_error_entry_t *entry;

    if (log == NULL) {
        return;
    }

    nvs_error_log_logic_sanitize(log);

    entry = &log->entries[log->head];
    memset(entry, 0, sizeof(*entry));
    entry->seq = log->next_seq++;
    entry->uptime_s = uptime_s;
    entry->err_code = err_code;

    if (tag != NULL) {
        snprintf(entry->tag, sizeof(entry->tag), "%s", tag);
    }
    if (message != NULL) {
        snprintf(entry->message, sizeof(entry->message), "%s", message);
    }

    log->head = (uint8_t)((log->head + 1u) % NVS_ERROR_LOG_CAPACITY);
    if (log->count < NVS_ERROR_LOG_CAPACITY) {
        log->count++;
    }
}

#endif
