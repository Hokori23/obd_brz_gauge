#ifndef APP_OBD_DSP_OBD_POLL_SCHEDULE_LOGIC_H
#define APP_OBD_DSP_OBD_POLL_SCHEDULE_LOGIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t slot_id;
    uint32_t demand_mask;
} obd_poll_schedule_entry_t;

static inline bool obd_poll_schedule_find_next_active_index(
    const obd_poll_schedule_entry_t *seq,
    size_t seq_len,
    size_t start_index,
    uint32_t demand_mask,
    size_t *index_out)
{
    if (seq == NULL || seq_len == 0u || demand_mask == 0u || index_out == NULL) {
        return false;
    }

    for (size_t offset = 0; offset < seq_len; ++offset) {
        size_t idx = (start_index + offset) % seq_len;
        if ((seq[idx].demand_mask & demand_mask) != 0u) {
            *index_out = idx;
            return true;
        }
    }

    return false;
}

#endif
