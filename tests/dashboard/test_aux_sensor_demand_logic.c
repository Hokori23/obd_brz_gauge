#include "../../main/app_obd_dsp/aux_sensor_demand_logic.h"

#include <stddef.h>

_Static_assert(AUX_SENSOR_DEMAND_MASK_COMPOSE(true, false, false, false, false, false, false, false, false, false, false) ==
                   (AUX_OBD_DEMAND_RPM | AUX_OBD_DEMAND_SPEED),
               "Gear page must force RPM and speed demand");

_Static_assert(AUX_SENSOR_DEMAND_MASK_COMPOSE(false, true, true, false, true, false, false, true, false, false, false) == 0u,
               "G-force OBD page must suppress normal PID polling demand");

_Static_assert(AUX_SENSOR_DEMAND_MASK_COMPOSE(false, false, true, false, true, false, false, true, false, false, true) ==
                   (AUX_OBD_DEMAND_RPM |
                    AUX_OBD_DEMAND_SPEED |
                    AUX_OBD_DEMAND_TPS |
                    AUX_OBD_DEMAND_BOOST),
               "Metric pages must compose only the requested item bits");

_Static_assert(AUX_SENSOR_DEMAND_MASK_COMPOSE(false, false, false, true, false, true, true, false, true, true, false) ==
                   (AUX_OBD_DEMAND_IAT |
                    AUX_OBD_DEMAND_CLT |
                    AUX_OBD_DEMAND_LOAD |
                    AUX_OBD_DEMAND_OIL |
                    AUX_OBD_DEMAND_BAT),
               "Metric pages should compose the remaining supported demand bits without pulling unrelated sensors");

_Static_assert(AUX_SENSOR_DEMAND_MASK_COMPOSE(true, true, false, false, false, false, false, false, false, false, false) ==
                   (AUX_OBD_DEMAND_RPM | AUX_OBD_DEMAND_SPEED),
               "Gear-page demand must take precedence over G-force OBD suppression when both flags are set");

int test_aux_sensor_demand_logic_dummy_symbol(void)
{
    const aux_sensor_obd_page_flags_t flags = {
        .uses_gear_page = false,
        .uses_gforce_obd_page = false,
        .uses_rpm = true,
        .uses_iat = true,
        .uses_speed = false,
        .uses_clt = false,
        .uses_load = true,
        .uses_tps = false,
        .uses_oil = true,
        .uses_bat = false,
        .uses_boost = true,
    };

    if (aux_sensor_demand_mask_from_page_flags(NULL) != 0u) {
        return 1;
    }
    if (aux_sensor_demand_mask_from_page_flags(&flags) !=
        (AUX_OBD_DEMAND_RPM |
         AUX_OBD_DEMAND_IAT |
         AUX_OBD_DEMAND_LOAD |
         AUX_OBD_DEMAND_OIL |
         AUX_OBD_DEMAND_BOOST)) {
        return 2;
    }
    return 0;
}
