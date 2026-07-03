#include "../../main/app_obd_dsp/ui_metric_layout_logic.h"

int test_ui_metric_layout_logic_dummy_symbol(void)
{
    uint8_t rows = 0u;
    uint8_t row_slots[3] = {0u, 0u, 0u};

    if (ui_metric_layout_build_row_model(0u, row_slots, &rows)) {
        return 1;
    }
    if (!ui_metric_layout_build_row_model(1u, row_slots, &rows) || rows != 1u || row_slots[0] != 1u) {
        return 2;
    }
    if (!ui_metric_layout_build_row_model(3u, row_slots, &rows) ||
        rows != 2u || row_slots[0] != 2u || row_slots[1] != 1u) {
        return 3;
    }
    if (!ui_metric_layout_build_row_model(5u, row_slots, &rows) ||
        rows != 3u || row_slots[0] != 2u || row_slots[1] != 2u || row_slots[2] != 1u) {
        return 4;
    }
    if (!ui_metric_layout_build_row_model(6u, row_slots, &rows) ||
        rows != 3u || row_slots[0] != 2u || row_slots[1] != 2u || row_slots[2] != 2u) {
        return 5;
    }
    if (ui_metric_layout_build_row_model(7u, row_slots, &rows)) {
        return 6;
    }

    return 0;
}

