#pragma once

#include <stdint.h>
#include "obd_data_cache.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#define VEHICLE_MAX_GEARS 9   // 容纳 index0 + 最多 8 个前进挡 (如 ZF 8HP)

// 油温查询模式优先级
typedef enum {
    OIL_TEMP_MODE_NONE = 0xFF,
    OIL_TEMP_MODE_PID_5C = 0,       // 标准 PID 01 5C
    OIL_TEMP_MODE_UDS_22_10_17 = 1, // UDS Mode 22 10 17
    OIL_TEMP_MODE_TOYOTA_21_01 = 2, // Toyota Mode 21 01
    OIL_TEMP_MODE_MAZDA_22_111F = 3, // Mazda Skyactiv Mode 22 PID 111F, 单字节 A-50
    OIL_TEMP_MODE_MAZDA_22_1310 = 4, // Mazda Skyactiv Mode 22 PID 1310, 双字节 (A*256+B)/100-40
} oil_temp_query_mode_t;

// 车辆档位传动比范围 (用于档位识别)
typedef struct {
    float min_ratio;
    float max_ratio;
    enGear gear;
} gear_ratio_range_t;

// 油温查询策略
typedef struct {
    oil_temp_query_mode_t primary;     // 首选查询模式
    oil_temp_query_mode_t secondary;   // 备用1
    oil_temp_query_mode_t tertiary;    // 备用2
    uint16_t offset_c;                 // 温度偏移量，单位 0.1°C（有符号）
} oil_temp_strategy_t;

// 车辆参数配置
typedef struct {
    const char *name;                    // 显示名称 (e.g. "BRZ ZC6")
    float final_drive_ratio;             // 主减速比
    float tire_rolling_radius_m;         // 轮胎滚动半径 (m)
    uint8_t gear_count;                  // 前进挡数量 (5 or 6)
    float gear_ratios[VEHICLE_MAX_GEARS]; // 各挡传动比, index 0 unused, 1~gear_count 有效
    float gear_tolerance;                // 档位识别容差 (e.g. 0.15 = ±15%)
    oil_temp_strategy_t oil_temp_strategy; // 油温查询策略
    bool has_boost;                      // 是否有涡轮增压(决定是否查询/显示涡轮压力)
} vehicle_profile_t;

// 获取所有预定义的车辆配置
const vehicle_profile_t *vehicle_profile_get_all(uint8_t *count);

// 获取指定索引的车辆配置
const vehicle_profile_t *vehicle_profile_get(uint8_t index);

// 获取当前激活的车辆配置
const vehicle_profile_t *vehicle_profile_get_active(void);

// 设置激活的车辆配置 (同时保存到 NVS)
void vehicle_profile_set_active(uint8_t index);

// 计算速度常数: 1 / (final_drive * 0.377 * tire_radius)
float vehicle_profile_calc_constant(const vehicle_profile_t *p);

// 根据当前激活的车辆配置生成档位范围数组
// 返回范围数组指针, count 输出有效元素数量
const gear_ratio_range_t *vehicle_profile_get_gear_ranges(uint8_t *count);

// 获取当前激活车型的油温查询策略
const oil_temp_strategy_t *vehicle_profile_get_oil_temp_strategy(void);
bool vehicle_profile_is_active_zc6(void);

#ifdef __cplusplus
}
#endif
