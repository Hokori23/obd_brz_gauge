#include "app_obd_dsp/aux_sensor_demand.h"

#include "bsp_obd_dsp/racechrono_ble_diy.h"
#include "bsp_obd_dsp/rs485_brake_temp.h"
#include "export_path/ui_home_runtime.h"

#define RC_PID_BRAKE_X10 0x0000F109u

void aux_sensor_demand_refresh(void)
{
    bool brake_needed = ui_home_runtime_active_page_uses_item(DISP_ITEM_BKT) ||
                        racechrono_ble_diy_is_pid_enabled(RC_PID_BRAKE_X10);

    rs485_brake_temp_set_enabled(brake_needed);
}
