#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AUX_OBD_DEMAND_RPM   = 1u << 0,
    AUX_OBD_DEMAND_IAT   = 1u << 1,
    AUX_OBD_DEMAND_SPEED = 1u << 2,
    AUX_OBD_DEMAND_CLT   = 1u << 3,
    AUX_OBD_DEMAND_LOAD  = 1u << 4,
    AUX_OBD_DEMAND_TPS   = 1u << 5,
    AUX_OBD_DEMAND_OIL   = 1u << 6,
    AUX_OBD_DEMAND_BAT   = 1u << 7,
    AUX_OBD_DEMAND_BOOST = 1u << 8,
    AUX_OBD_DEMAND_IGN   = 1u << 9,
} aux_obd_demand_mask_t;

void aux_sensor_demand_refresh(void);
uint32_t aux_sensor_demand_get_obd_mask(void);
bool aux_sensor_demand_is_gforce_obd_enabled(void);
bool aux_sensor_demand_is_zc6_gear_obd_enabled(void);
bool aux_sensor_demand_is_zc6_tpms_enabled(void);

#ifdef __cplusplus
}
#endif
