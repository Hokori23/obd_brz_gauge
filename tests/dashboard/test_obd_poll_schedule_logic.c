#include "../../main/app_obd_dsp/obd_poll_schedule_logic.h"

#include <stddef.h>

int test_obd_poll_schedule_logic_dummy_symbol(void)
{
    static const obd_poll_schedule_entry_t seq[] = {
        {.slot_id = 1u, .demand_mask = 1u << 0},
        {.slot_id = 2u, .demand_mask = 1u << 1},
        {.slot_id = 3u, .demand_mask = 1u << 2},
        {.slot_id = 4u, .demand_mask = 1u << 0},
    };
    size_t index = 0u;

    if (obd_poll_schedule_find_next_active_index(NULL, 4u, 0u, 1u, &index)) {
        return 1;
    }
    if (obd_poll_schedule_find_next_active_index(seq, 0u, 0u, 1u, &index)) {
        return 2;
    }
    if (obd_poll_schedule_find_next_active_index(seq, 4u, 0u, 0u, &index)) {
        return 3;
    }
    if (!obd_poll_schedule_find_next_active_index(seq, 4u, 0u, 1u << 1, &index) || index != 1u) {
        return 4;
    }
    if (!obd_poll_schedule_find_next_active_index(seq, 4u, 2u, 1u << 0, &index) || index != 3u) {
        return 5;
    }
    if (!obd_poll_schedule_find_next_active_index(seq, 4u, 0u, 1u << 2, &index) || index != 2u) {
        return 6;
    }
    if (!obd_poll_schedule_find_next_active_index(seq, 4u, 1u, 1u << 0, &index) || index != 3u) {
        return 7;
    }

    return 0;
}
