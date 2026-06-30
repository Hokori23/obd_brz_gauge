#pragma once

#include <stdint.h>

#include "elm327_ble_client.h"

#define BLE_SCAN_BUFFER_CAPACITY BLE_SCAN_MAX_DEVICES

#define BLE_SCAN_BUFFER_BYTES(capacity) \
    ((uint32_t)(capacity) * (uint32_t)sizeof(ble_scan_result_t))
