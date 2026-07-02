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

int test_aux_sensor_demand_logic_dummy_symbol(void)
{
    if (aux_sensor_demand_mask_from_page_flags(NULL) != 0u) {
        return 1;
    }
    return 0;
}
