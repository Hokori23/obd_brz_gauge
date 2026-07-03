#pragma once

#include "bsp_obd_dsp/boards/board_api.h"

void app_bootstrap_init_storage_and_profile(void);
void app_bootstrap_init_board_and_display(board_display_context_t *out_ctx);
void app_bootstrap_start_runtime_services(void);
