#ifndef APP_OBD_DSP_AUX_SENSOR_DEMAND_LOGIC_H
#define APP_OBD_DSP_AUX_SENSOR_DEMAND_LOGIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_obd_dsp/aux_sensor_demand.h"

typedef struct {
    bool uses_gear_page;
    bool uses_gforce_obd_page;
    bool uses_rpm;
    bool uses_iat;
    bool uses_speed;
    bool uses_clt;
    bool uses_load;
    bool uses_tps;
    bool uses_oil;
    bool uses_bat;
    bool uses_boost;
} aux_sensor_obd_page_flags_t;

#define AUX_SENSOR_DEMAND_MASK_COMPOSE(uses_gear_page,           \
                                       uses_gforce_obd_page,     \
                                       uses_rpm,                 \
                                       uses_iat,                 \
                                       uses_speed,               \
                                       uses_clt,                 \
                                       uses_load,                \
                                       uses_tps,                 \
                                       uses_oil,                 \
                                       uses_bat,                 \
                                       uses_boost)               \
    ((uses_gear_page)                                                   \
         ? (AUX_OBD_DEMAND_RPM | AUX_OBD_DEMAND_SPEED)                  \
         : ((uses_gforce_obd_page)                                      \
                ? 0u                                                    \
                : (((uses_rpm) ? AUX_OBD_DEMAND_RPM : 0u) |            \
                   ((uses_iat) ? AUX_OBD_DEMAND_IAT : 0u) |            \
                   ((uses_speed) ? AUX_OBD_DEMAND_SPEED : 0u) |        \
                   ((uses_clt) ? AUX_OBD_DEMAND_CLT : 0u) |            \
                   ((uses_load) ? AUX_OBD_DEMAND_LOAD : 0u) |          \
                   ((uses_tps) ? AUX_OBD_DEMAND_TPS : 0u) |            \
                   ((uses_oil) ? AUX_OBD_DEMAND_OIL : 0u) |            \
                   ((uses_bat) ? AUX_OBD_DEMAND_BAT : 0u) |            \
                   ((uses_boost) ? AUX_OBD_DEMAND_BOOST : 0u))))

static inline uint32_t aux_sensor_demand_mask_from_page_flags(
    const aux_sensor_obd_page_flags_t *flags)
{
    if (flags == NULL) {
        return 0u;
    }

    return AUX_SENSOR_DEMAND_MASK_COMPOSE(flags->uses_gear_page,
                                          flags->uses_gforce_obd_page,
                                          flags->uses_rpm,
                                          flags->uses_iat,
                                          flags->uses_speed,
                                          flags->uses_clt,
                                          flags->uses_load,
                                          flags->uses_tps,
                                          flags->uses_oil,
                                          flags->uses_bat,
                                          flags->uses_boost);
}

#endif
