#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

// 启动 RS485 刹车温度采集任务（Modbus RTU）
void rs485_brake_temp_start(void);
void rs485_brake_temp_set_enabled(bool enabled);
bool rs485_brake_temp_is_enabled(void);

#ifdef __cplusplus
}
#endif
