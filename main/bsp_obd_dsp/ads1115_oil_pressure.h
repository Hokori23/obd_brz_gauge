#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 启动机油压力采集任务（ESP32 ADC 直连）。
void oil_pressure_start(void);
void oil_pressure_set_enabled(bool enabled);

// 兼容旧接口名（历史上为 ADS1115 方案）。
void ads1115_oil_pressure_start(void);

#ifdef __cplusplus
}
#endif
