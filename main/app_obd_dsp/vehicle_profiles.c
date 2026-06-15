#include "vehicle_profiles.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "esp_log.h"
#include <string.h>

#define TAG "vehicle_profile"

// 预定义车辆配置
static const vehicle_profile_t s_profiles[] = {
    {
        .name = "BRZ ZC6",
        .final_drive_ratio = 4.100f,
        .tire_rolling_radius_m = 0.314f,   // 215/45R17
        .gear_count = 6,
        .gear_ratios = {0, 3.626f, 2.188f, 1.541f, 1.213f, 1.000f, 0.767f},
        .gear_tolerance = 0.15f,
        .oil_temp_strategy = {
            .primary = OIL_TEMP_MODE_TOYOTA_21_01,  // BRZ ZC6 固定 Mode 21(单一模式，不回退)
            .secondary = OIL_TEMP_MODE_NONE,
            .tertiary = OIL_TEMP_MODE_NONE,
            .offset_c = 0,  // 无偏移
        },
    },
    {
        .name = "BRZ ZD8",
        .final_drive_ratio = 3.700f,       // ZD8 新代差速比
        .tire_rolling_radius_m = 0.318f,   // 225/40R18
        .gear_count = 6,
        .gear_ratios = {0, 3.765f, 2.476f, 1.633f, 1.190f, 0.932f, 0.751f},
        .gear_tolerance = 0.15f,
        .oil_temp_strategy = {
            .primary = OIL_TEMP_MODE_PID_5C,        // ZD8 固定标准 PID(单一模式，不回退)
            .secondary = OIL_TEMP_MODE_NONE,
            .tertiary = OIL_TEMP_MODE_NONE,
            .offset_c = 0,
        },
    },
    {
        .name = "MX-5 ND",
        .final_drive_ratio = 2.866f,       // ND 6MT（所有手动一致，自动为 3.583）
        .tire_rolling_radius_m = 0.300f,   // 195/50R16
        .gear_count = 6,
        .gear_ratios = {0, 5.087f, 2.991f, 2.035f, 1.594f, 1.286f, 1.000f},
        .gear_tolerance = 0.15f,
        .oil_temp_strategy = {
            // 先试 PID 1310(双字节)，连续失败则回退到 111F(单字节)——两种已知 Mazda 油温 PID 都覆盖
            .primary = OIL_TEMP_MODE_MAZDA_22_1310,
            .secondary = OIL_TEMP_MODE_MAZDA_22_111F,
            .tertiary = OIL_TEMP_MODE_NONE,
            .offset_c = 0,
        },
        // .has_boost 默认 false (自吸)
    },
    {
        .name = "BMW G",
        .final_drive_ratio = 2.813f,       // G20 330i 主减速比
        .tire_rolling_radius_m = 0.330f,   // 225/45R18
        .gear_count = 8,                   // ZF 8HP 8速
        .gear_ratios = {0, 5.250f, 3.360f, 2.172f, 1.720f, 1.316f, 1.000f, 0.822f, 0.640f},
        .gear_tolerance = 0.12f,
        .oil_temp_strategy = {
            // BMW 油温多需厂商协议(F系 Mode 22 PID 4402, B-64)，通用 OBD 下常读不到，
            // 这里先用标准 PID 兜底，读不到则显示无效，后续可加 BMW 专用模式。
            .primary = OIL_TEMP_MODE_PID_5C,
            .secondary = OIL_TEMP_MODE_NONE,
            .tertiary = OIL_TEMP_MODE_NONE,
            .offset_c = 0,
        },
        .has_boost = true,                 // 涡轮增压(B48/B58)，读取涡轮压力
    },
};

#define PROFILE_COUNT (sizeof(s_profiles) / sizeof(s_profiles[0]))

// 缓存当前激活配置的档位范围
static gear_ratio_range_t s_gear_ranges[VEHICLE_MAX_GEARS];
static uint8_t s_gear_range_count = 0;
static uint8_t s_active_idx = 0;
static bool s_ranges_dirty = true;

// 根据配置重新计算档位传动比范围
static void rebuild_gear_ranges(const vehicle_profile_t *p)
{
    s_gear_range_count = 0;
    for (uint8_t i = 1; i <= p->gear_count && i < VEHICLE_MAX_GEARS; i++) {
        float total = p->gear_ratios[i] * p->final_drive_ratio;
        s_gear_ranges[s_gear_range_count].min_ratio = total * (1.0f - p->gear_tolerance);
        s_gear_ranges[s_gear_range_count].max_ratio = total * (1.0f + p->gear_tolerance);
        s_gear_ranges[s_gear_range_count].gear = (enGear)i; // GEAR_1=1, GEAR_2=2, ...
        s_gear_range_count++;
    }
    s_ranges_dirty = false;
    ESP_LOGI(TAG, "Rebuilt gear ranges for '%s' (%d gears)", p->name, p->gear_count);
}

const vehicle_profile_t *vehicle_profile_get_all(uint8_t *count)
{
    if (count) *count = (uint8_t)PROFILE_COUNT;
    return s_profiles;
}

const vehicle_profile_t *vehicle_profile_get(uint8_t index)
{
    if (index >= PROFILE_COUNT) return &s_profiles[0];
    return &s_profiles[index];
}

const vehicle_profile_t *vehicle_profile_get_active(void)
{
    return vehicle_profile_get(s_active_idx);
}

void vehicle_profile_set_active(uint8_t index)
{
    if (index >= PROFILE_COUNT) index = 0;
    s_active_idx = index;
    s_ranges_dirty = true;

    // 保存到 NVS
    nvs_user_cfg_t cfg = *nvs_cfg_get();
    cfg.vehicle_profile_idx = index;
    nvs_cfg_set(&cfg);

    ESP_LOGI(TAG, "Active profile set to [%d] '%s'", index, s_profiles[index].name);
}

float vehicle_profile_calc_constant(const vehicle_profile_t *p)
{
    if (!p) return 0;
    float denom = p->final_drive_ratio * 0.377f * p->tire_rolling_radius_m;
    if (denom == 0) return 0;
    return 1.0f / denom;
}

const gear_ratio_range_t *vehicle_profile_get_gear_ranges(uint8_t *count)
{
    if (s_ranges_dirty) {
        rebuild_gear_ranges(vehicle_profile_get_active());
    }
    if (count) *count = s_gear_range_count;
    return s_gear_ranges;
}

const oil_temp_strategy_t *vehicle_profile_get_oil_temp_strategy(void)
{
    const vehicle_profile_t *p = vehicle_profile_get_active();
    if (!p) return NULL;
    return &p->oil_temp_strategy;
}
