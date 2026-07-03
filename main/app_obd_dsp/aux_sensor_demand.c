#include "app_obd_dsp/aux_sensor_demand.h"
#include "app_obd_dsp/aux_sensor_demand_logic.h"

#include "bsp_obd_dsp/ads1115_oil_pressure.h"
#include "bsp_obd_dsp/racechrono_ble_diy.h"
#include "bsp_obd_dsp/qmi8658_gforce.h"
#include "bsp_obd_dsp/rs485_brake_temp.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "export_path/ui_home_runtime.h"

#define RC_PID_BRAKE_X10 0x0000F109u

static volatile uint32_t s_obd_demand_mask = 0u;
static volatile bool s_gforce_obd_enabled = false;
static volatile bool s_zc6_gear_obd_enabled = false;

/** 判断当前激活车型是否为 ZC6。 */
static bool aux_sensor_is_active_vehicle_zc6(void)
{
    return nvs_cfg_get()->vehicle_profile_idx == 0u;
}

/**
 * 汇总首页当前需要的 OBD 轮询需求掩码
 *
 * 核心职责：根据首页页面类型和显示项，生成本轮应采集的 OBD 数据集合
 */
static uint32_t ui_home_obd_demand_mask(void)
{
    bool zc6_gear_monitor_needed =
        aux_sensor_is_active_vehicle_zc6() &&
        ui_home_runtime_active_page_uses_type(UI_DASHBOARD_PAGE_TYPE_GEAR);
    bool zc6_gear_rpm_ring_needed =
        ui_home_runtime_active_or_warm_page_gear_rpm_ring_enabled();
    const aux_sensor_obd_page_flags_t flags = {
        .uses_gear_page = ui_home_runtime_active_page_uses_type(UI_DASHBOARD_PAGE_TYPE_GEAR) &&
                          !zc6_gear_monitor_needed,
        .uses_gforce_obd_page = ui_home_runtime_active_page_uses_type(UI_DASHBOARD_PAGE_TYPE_G_FORCE_OBD),
        .uses_rpm = ui_home_runtime_active_or_warm_page_uses_item(DISP_ITEM_RPM) || zc6_gear_rpm_ring_needed,
        .uses_iat = ui_home_runtime_active_or_warm_page_uses_item(DISP_ITEM_IAT),
        .uses_speed = ui_home_runtime_active_or_warm_page_uses_item(DISP_ITEM_SPEED),
        .uses_clt = ui_home_runtime_active_or_warm_page_uses_item(DISP_ITEM_CLT),
        .uses_load = ui_home_runtime_active_or_warm_page_uses_item(DISP_ITEM_LOAD),
        .uses_tps = ui_home_runtime_active_or_warm_page_uses_item(DISP_ITEM_TPS),
        .uses_oil = ui_home_runtime_active_or_warm_page_uses_item(DISP_ITEM_OIL),
        .uses_bat = ui_home_runtime_active_or_warm_page_uses_item(DISP_ITEM_BAT),
        .uses_boost = ui_home_runtime_active_or_warm_page_uses_item(DISP_ITEM_BOOST),
    };

    return aux_sensor_demand_mask_from_page_flags(&flags);
}

/**
 * 刷新当前页面驱动的传感器需求状态
 *
 * 核心职责：统一收敛 OBD、刹车温度、IMU、油压等采集模块的启停开关
 */
void aux_sensor_demand_refresh(void)
{
    const nvs_user_cfg_t *cfg = nvs_cfg_get();
    bool brake_needed = ui_home_runtime_active_page_uses_item(DISP_ITEM_BKT) ||
                        racechrono_ble_diy_is_pid_enabled(RC_PID_BRAKE_X10);
    bool gforce_obd_needed = ui_home_runtime_active_page_uses_type(UI_DASHBOARD_PAGE_TYPE_G_FORCE_OBD);
    bool imu_needed = ui_home_runtime_active_page_uses_type(UI_DASHBOARD_PAGE_TYPE_G_FORCE_ESP32);
    bool zc6_gear_obd_needed = aux_sensor_is_active_vehicle_zc6() &&
                               ui_home_runtime_active_page_uses_type(UI_DASHBOARD_PAGE_TYPE_GEAR);
    bool oil_pressure_needed = !nvs_cfg_is_oil_pressure_demand_driven(cfg) ||
                               ui_home_runtime_active_page_uses_item(DISP_ITEM_OILP);

    // ========== 需求广播 ==========
    // 统一从当前激活页面推导所有采集开关，避免多个模块各自判断导致状态漂移。
    s_obd_demand_mask = ui_home_obd_demand_mask();
    s_gforce_obd_enabled = gforce_obd_needed;
    s_zc6_gear_obd_enabled = zc6_gear_obd_needed;
    rs485_brake_temp_set_enabled(brake_needed);
    qmi8658_gforce_set_enabled(imu_needed);
    oil_pressure_set_enabled(oil_pressure_needed);
}

/** 返回当前页面需要的 OBD 轮询需求掩码。 */
uint32_t aux_sensor_demand_get_obd_mask(void)
{
    return s_obd_demand_mask;
}

/** 返回当前是否需要启用 OBD G-force 监控流。 */
bool aux_sensor_demand_is_gforce_obd_enabled(void)
{
    return s_gforce_obd_enabled;
}

/** 返回当前是否需要启用 ZC6 档位 OBD 监控流。 */
bool aux_sensor_demand_is_zc6_gear_obd_enabled(void)
{
    return s_zc6_gear_obd_enabled;
}
