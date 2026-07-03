#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ui_metric_layout_build_row_model(uint8_t slot_count,
                                      uint8_t row_slot_counts[3],
                                      uint8_t *row_count_out);

#ifdef __cplusplus
}
#endif

