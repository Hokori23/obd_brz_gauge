#include "app_obd_dsp/aux_sensor_demand.h"
#include "app_obd_dsp/aux_sensor_demand_logic.h"

#include "bsp_obd_dsp/racechrono_ble_diy.h"
#include "bsp_obd_dsp/qmi8658_gforce.h"
#include "bsp_obd_dsp/rs485_brake_temp.h"
#include "export_path/ui_home_runtime.h"

#define RC_PID_BRAKE_X10 0x0000F109u

static volatile uint32_t s_obd_demand_mask = 0u;
static volatile bool s_gforce_obd_enabled = false;

static uint32_t ui_home_obd_demand_mask(void)
{
    const aux_sensor_obd_page_flags_t flags = {
        .uses_gear_page = ui_home_runtime_active_page_uses_type(UI_DASHBOARD_PAGE_TYPE_GEAR),
        .uses_gforce_obd_page = ui_home_runtime_active_page_uses_type(UI_DASHBOARD_PAGE_TYPE_G_FORCE_OBD),
        .uses_rpm = ui_home_runtime_active_page_uses_item(DISP_ITEM_RPM),
        .uses_iat = ui_home_runtime_active_page_uses_item(DISP_ITEM_IAT),
        .uses_speed = ui_home_runtime_active_page_uses_item(DISP_ITEM_SPEED),
        .uses_clt = ui_home_runtime_active_page_uses_item(DISP_ITEM_CLT),
        .uses_load = ui_home_runtime_active_page_uses_item(DISP_ITEM_LOAD),
        .uses_tps = ui_home_runtime_active_page_uses_item(DISP_ITEM_TPS),
        .uses_oil = ui_home_runtime_active_page_uses_item(DISP_ITEM_OIL),
        .uses_bat = ui_home_runtime_active_page_uses_item(DISP_ITEM_BAT),
        .uses_boost = ui_home_runtime_active_page_uses_item(DISP_ITEM_BOOST),
    };

    return aux_sensor_demand_mask_from_page_flags(&flags);
}

void aux_sensor_demand_refresh(void)
{
    bool brake_needed = ui_home_runtime_active_page_uses_item(DISP_ITEM_BKT) ||
                        racechrono_ble_diy_is_pid_enabled(RC_PID_BRAKE_X10);
    bool gforce_obd_needed = ui_home_runtime_active_page_uses_type(UI_DASHBOARD_PAGE_TYPE_G_FORCE_OBD);
    bool imu_needed = ui_home_runtime_active_page_uses_type(UI_DASHBOARD_PAGE_TYPE_G_FORCE_ESP32);

    s_obd_demand_mask = ui_home_obd_demand_mask();
    s_gforce_obd_enabled = gforce_obd_needed;
    rs485_brake_temp_set_enabled(brake_needed);
    qmi8658_gforce_set_enabled(imu_needed);
}

uint32_t aux_sensor_demand_get_obd_mask(void)
{
    return s_obd_demand_mask;
}

bool aux_sensor_demand_is_gforce_obd_enabled(void)
{
    return s_gforce_obd_enabled;
}
