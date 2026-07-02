#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GEAR_NEUTRAL,
    GEAR_1,
    GEAR_2,
    GEAR_3,
    GEAR_4,
    GEAR_5,
    GEAR_6,
    GEAR_REVERSE,
} enGear;

typedef enum {
    BRAKE_RS485_IDLE = 0,
    BRAKE_RS485_PROBE,
    BRAKE_RS485_OK,
    BRAKE_RS485_TIMEOUT,
    BRAKE_RS485_PARSE_FAIL,
} brake_rs485_status_t;

void obd_data_set_rpm(uint16_t rpm);
void obd_data_set_speed(uint8_t kmh);
void obd_data_set_coolant_temp(int16_t temp);
void obd_data_set_oil_temp(int16_t temp);
void obd_data_set_intake_temp(int16_t temp);
void obd_data_set_load_pct(int16_t pct);
void obd_data_set_tps(int16_t pct);
void obd_data_set_bat_mv(int32_t mv);
void obd_data_set_oil_pressure_x10(int16_t pressure_x10);
void obd_data_set_boost_x10(int16_t boost_x10);
void obd_data_set_brake_temp_x10(int16_t temp_x10);
void obd_data_set_lat_accel_x100(int16_t accel_x100);
void obd_data_set_lon_accel_x100(int16_t accel_x100);
void obd_data_set_lat_accel_imu_x100(int16_t accel_x100);
void obd_data_set_lon_accel_imu_x100(int16_t accel_x100);
void obd_data_set_brake_rs485_status(brake_rs485_status_t status);
void obd_data_set_actual_gear(enGear gear);
void obd_data_clear_actual_gear(void);

uint16_t obd_data_get_rpm(void);
uint8_t obd_data_get_speed(void);
int16_t obd_data_get_coolant_temp(void);
int16_t obd_data_get_oil_temp(void);
int16_t obd_data_get_intake_temp(void);
int16_t obd_data_get_load_pct(void);
int16_t obd_data_get_tps(void);
int32_t obd_data_get_bat_mv(void);
int16_t obd_data_get_oil_pressure_x10(void);
int16_t obd_data_get_boost_x10(void);
int16_t obd_data_get_brake_temp_x10(void);
int16_t obd_data_get_lat_accel_x100(void);
int16_t obd_data_get_lon_accel_x100(void);
int16_t obd_data_get_lat_accel_imu_x100(void);
int16_t obd_data_get_lon_accel_imu_x100(void);
brake_rs485_status_t obd_data_get_brake_rs485_status(void);
bool obd_data_get_actual_gear(enGear *gear_out);
enGear calculate_gear(float rpm, float speed);
void vMileageDataStatisticTask(void);

#ifdef __cplusplus
}
#endif
