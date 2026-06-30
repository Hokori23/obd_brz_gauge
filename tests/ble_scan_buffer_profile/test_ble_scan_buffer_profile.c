#include "../../main/bsp_obd_dsp/ble_scan_buffer_profile.h"

_Static_assert(BLE_SCAN_BUFFER_CAPACITY == BLE_SCAN_MAX_DEVICES,
               "BLE scan buffer capacity should match public max device contract");
_Static_assert(BLE_SCAN_BUFFER_BYTES(1) == sizeof(ble_scan_result_t),
               "single scan result byte size should stay exact");
_Static_assert(BLE_SCAN_BUFFER_BYTES(BLE_SCAN_BUFFER_CAPACITY) ==
                   (uint32_t)BLE_SCAN_MAX_DEVICES * (uint32_t)sizeof(ble_scan_result_t),
               "scan buffer bytes should scale linearly with capacity");

int main(void)
{
    return 0;
}
