#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_gap_ble_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Start RaceChrono BLE DIY GATTS service (UUID 0x1FF8).
// Requires BLE controller + bluedroid to be enabled by caller.
void racechrono_ble_diy_start(void);

// Forward GAP callbacks from the app's single BLE GAP callback.
void racechrono_ble_diy_handle_gap_event(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

// Query connection state to RaceChrono app.
bool racechrono_ble_diy_is_connected(void);
bool racechrono_ble_diy_is_pid_enabled(uint32_t pid);

#ifdef __cplusplus
}
#endif
