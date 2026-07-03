#include "ui_metric_layout_logic.h"

#include <string.h>

bool ui_metric_layout_build_row_model(uint8_t slot_count,
                                      uint8_t row_slot_counts[3],
                                      uint8_t *row_count_out)
{
    uint8_t rows = 0u;
    uint8_t local_slots[3] = {0u, 0u, 0u};

    if (row_slot_counts == NULL || row_count_out == NULL || slot_count == 0u) {
        return false;
    }

    switch (slot_count) {
    case 1u:
        rows = 1u;
        local_slots[0] = 1u;
        break;
    case 2u:
        rows = 2u;
        local_slots[0] = 1u;
        local_slots[1] = 1u;
        break;
    case 3u:
        rows = 2u;
        local_slots[0] = 2u;
        local_slots[1] = 1u;
        break;
    case 4u:
        rows = 2u;
        local_slots[0] = 2u;
        local_slots[1] = 2u;
        break;
    case 5u:
        rows = 3u;
        local_slots[0] = 2u;
        local_slots[1] = 2u;
        local_slots[2] = 1u;
        break;
    case 6u:
        rows = 3u;
        local_slots[0] = 2u;
        local_slots[1] = 2u;
        local_slots[2] = 2u;
        break;
    default:
        return false;
    }

    memcpy(row_slot_counts, local_slots, sizeof(local_slots));
    *row_count_out = rows;
    return true;
}

