#include "obd_data_cache.h"
#include "vehicle_profiles.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "esp_log.h"
#include <inttypes.h>
#include <math.h>

/*
 * Shared runtime sensor cache.
 *
 * Core responsibilities: hold the latest decoded telemetry,
 * smooth fast-changing values, and provide thread-safe getters/setters.
 */


// 使用简单全局变量 + 临界区保护
static volatile uint16_t s_rpm = 0;
static volatile uint8_t  s_speed = 0;
static volatile int16_t  s_coolant_temp = -40;
static volatile int16_t  s_oil_temp = -100;  // 真实机油温度 °C, -100=无效
static volatile int16_t  s_intake_temp = -40;
static volatile int16_t  s_load_pct = -1;   // 发动机负荷 0~100%, -1=无效
static volatile int16_t  s_tps = -1;         // 节气门开度 0~100%, -1=无效
static volatile int32_t  s_bat_mv = -1;     // 电压 mV, -1=无效
static volatile int16_t  s_oil_pressure_x10 = -1; // 油压, 0.1bar, -1=无效
static volatile int16_t  s_brake_temp_x10 = -1000; // 刹车温度, 0.1°C, -1000=无效
static volatile int16_t  s_boost_x10 = -32768; // 涡轮表压, 0.1bar(可为负=真空), -32768=无效
static volatile int16_t  s_map_kpa = -1; // 进气歧管绝对压力 kPa, -1=无效
static volatile int16_t  s_ign_timing_x10 = -32768; // 点火提前角 0.1deg, -32768=无效
static volatile int16_t  s_lat_accel_x100 = -32768; // 0.01g, -32768=invalid
static volatile int16_t  s_lon_accel_x100 = -32768; // 0.01g, -32768=invalid
static volatile int16_t  s_lat_accel_imu_x100 = -32768; // 0.01g, -32768=invalid
static volatile int16_t  s_lon_accel_imu_x100 = -32768; // 0.01g, -32768=invalid
static volatile brake_rs485_status_t s_brake_rs485_status = BRAKE_RS485_IDLE;
static volatile enGear s_actual_gear = GEAR_NEUTRAL;
static volatile bool s_actual_gear_valid = false;
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

/** 写入最新的原始发动机转速。 */
void obd_data_set_rpm(uint16_t rpm)
{
    portENTER_CRITICAL(&s_mux);
    s_rpm = rpm;
    portEXIT_CRITICAL(&s_mux);
}

/** 写入最新的原始车速。 */
void obd_data_set_speed(uint8_t kmh)
{
    portENTER_CRITICAL(&s_mux);
    s_speed = kmh;
    portEXIT_CRITICAL(&s_mux);
}

/** 写入最新的冷却液温度。 */
void obd_data_set_coolant_temp(int16_t temp)
{
    portENTER_CRITICAL(&s_mux);
    s_coolant_temp = temp;
    portEXIT_CRITICAL(&s_mux);
}

/**
 * Store oil temperature when the decoded value looks plausible.
 *
 * Core responsibility: reject parser glitches early so transient junk
 * data does not pollute UI widgets or warning logic.
 */
void obd_data_set_oil_temp(int16_t temp)
{
    // 有效范围 -20~150°C，超出视为解析错误丢弃
    if (temp < -20 || temp > 150) return;
    portENTER_CRITICAL(&s_mux);
    s_oil_temp = temp;
    portEXIT_CRITICAL(&s_mux);
}

/** 写入最新的进气温度。 */
void obd_data_set_intake_temp(int16_t temp)
{
    portENTER_CRITICAL(&s_mux);
    s_intake_temp = temp;
    portEXIT_CRITICAL(&s_mux);
}

#define RPM_SMOOTH_TIME_MS   1000  // 转速缓升缓降时间常数 (ms)
#define SPEED_SMOOTH_TIME_MS 1000  // 速度缓升缓降时间常数 (ms)
#define FALL_TO_ZERO_MS      500  // 归零缓降时间常数 (ms)

// 实时转速（缓升缓降）获取
/** 返回经过一阶平滑后的发动机转速。 */
uint16_t obd_data_get_rpm(void)
{
    static TickType_t last_tick = 0;
    static float smooth = 0.f;

    uint16_t raw;
    portENTER_CRITICAL(&s_mux);
    raw = s_rpm;
    portEXIT_CRITICAL(&s_mux);

    TickType_t now_tick = xTaskGetTickCount();
    uint32_t dt_ms = (now_tick - last_tick) * portTICK_PERIOD_MS;
    if (dt_ms > 1000) dt_ms = 1000;

    uint32_t tc = (raw == 0) ? FALL_TO_ZERO_MS : RPM_SMOOTH_TIME_MS;
    float alpha = (float)dt_ms / (float)tc;
    if (alpha > 1.0f) alpha = 1.0f;

    smooth += alpha * ((float)raw - smooth);
    last_tick = now_tick;

    return (uint16_t)(smooth + 0.5f);
}


// 实时速度（缓升缓降）获取
/** 返回使用同一平滑策略处理后的车速。 */
uint8_t obd_data_get_speed(void)
{
    static TickType_t last_tick = 0;
    static float smooth = 0.f; // 保留小数以获得更细腻的过渡

    // 1. 取原始速度
    uint8_t raw;
    portENTER_CRITICAL(&s_mux);
    raw = s_speed;
    portEXIT_CRITICAL(&s_mux);

    // 2. 计算距离上次调用的时间，单位 ms
    TickType_t now_tick = xTaskGetTickCount();
    uint32_t dt_ms = (now_tick - last_tick) * portTICK_PERIOD_MS;
    if (dt_ms > 1000) dt_ms = 1000; // 限制单次过大步长，防止休眠后跳变

    // 3. 时间常数的一阶滤波 alpha = dt / SPEED_SMOOTH_TIME_MS
    uint32_t tc = (raw == 0) ? FALL_TO_ZERO_MS : SPEED_SMOOTH_TIME_MS;
    float alpha = (float)dt_ms / (float)tc;
    if (alpha > 1.0f) alpha = 1.0f;

    // 4. 更新平滑值
    smooth += alpha * ((float)raw - smooth);
    last_tick = now_tick;
    return (uint8_t)(smooth + 0.5f); // 四舍五入返回
}

/** 返回最新的冷却液温度。 */
int16_t obd_data_get_coolant_temp(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_coolant_temp;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 返回最新的机油温度。 */
int16_t obd_data_get_oil_temp(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_oil_temp;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 返回最新的进气温度。 */
int16_t obd_data_get_intake_temp(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_intake_temp;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 写入最新的发动机负载百分比。 */
void obd_data_set_load_pct(int16_t pct)
{
    portENTER_CRITICAL(&s_mux);
    s_load_pct = pct;
    portEXIT_CRITICAL(&s_mux);
}

/** 返回最新的发动机负载百分比。 */
int16_t obd_data_get_load_pct(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_load_pct;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 写入最新的节气门开度百分比。 */
void obd_data_set_tps(int16_t pct)
{
    portENTER_CRITICAL(&s_mux);
    s_tps = pct;
    portEXIT_CRITICAL(&s_mux);
}

/** 返回最新的节气门开度百分比。 */
int16_t obd_data_get_tps(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_tps;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 写入最新的控制模块电压，单位为毫伏。 */
void obd_data_set_bat_mv(int32_t mv)
{
    portENTER_CRITICAL(&s_mux);
    s_bat_mv = mv;
    portEXIT_CRITICAL(&s_mux);
}

/** 返回最新的控制模块电压，单位为毫伏。 */
int32_t obd_data_get_bat_mv(void)
{
    int32_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_bat_mv;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 仅在数值合理时写入机油压力。 */
void obd_data_set_oil_pressure_x10(int16_t pressure_x10)
{
    // 合理范围: 0.0bar ~ 20.0bar
    if (pressure_x10 < 0 || pressure_x10 > 200) return;
    portENTER_CRITICAL(&s_mux);
    s_oil_pressure_x10 = pressure_x10;
    portEXIT_CRITICAL(&s_mux);
}

/** 返回最新的机油压力，单位为 0.1 bar。 */
int16_t obd_data_get_oil_pressure_x10(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_oil_pressure_x10;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 仅在数值合理时写入增压压力。 */
void obd_data_set_boost_x10(int16_t boost_x10)
{
    // 涡轮表压合理范围: -1.5bar(真空) ~ +30.0bar
    if (boost_x10 < -15 || boost_x10 > 300) return;
    portENTER_CRITICAL(&s_mux);
    s_boost_x10 = boost_x10;
    portEXIT_CRITICAL(&s_mux);
}

void obd_data_set_map_kpa(int16_t map_kpa)
{
    if (map_kpa < 0 || map_kpa > 255) return;
    portENTER_CRITICAL(&s_mux);
    s_map_kpa = map_kpa;
    portEXIT_CRITICAL(&s_mux);
}

int16_t obd_data_get_map_kpa(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_map_kpa;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

void obd_data_set_ign_timing_x10(int16_t ign_x10)
{
    if (ign_x10 < -640 || ign_x10 > 635) return;
    portENTER_CRITICAL(&s_mux);
    s_ign_timing_x10 = ign_x10;
    portEXIT_CRITICAL(&s_mux);
}

int16_t obd_data_get_ign_timing_x10(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_ign_timing_x10;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 返回最新的增压压力，单位为 0.1 bar。 */
int16_t obd_data_get_boost_x10(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_boost_x10;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 仅在数值合理时写入刹车温度。 */
void obd_data_set_brake_temp_x10(int16_t temp_x10)
{
    // 合理范围: -50.0°C ~ 1200.0°C
    if (temp_x10 < -500 || temp_x10 > 12000) return;
    portENTER_CRITICAL(&s_mux);
    s_brake_temp_x10 = temp_x10;
    portEXIT_CRITICAL(&s_mux);
}

/** 写入 OBD 推导的横向加速度，单位为 0.01 g。 */
void obd_data_set_lat_accel_x100(int16_t accel_x100)
{
    if (accel_x100 != -32768 && (accel_x100 < -500 || accel_x100 > 500)) return;
    portENTER_CRITICAL(&s_mux);
    s_lat_accel_x100 = accel_x100;
    portEXIT_CRITICAL(&s_mux);
}

/** 写入 OBD 推导的纵向加速度，单位为 0.01 g。 */
void obd_data_set_lon_accel_x100(int16_t accel_x100)
{
    if (accel_x100 != -32768 && (accel_x100 < -500 || accel_x100 > 500)) return;
    portENTER_CRITICAL(&s_mux);
    s_lon_accel_x100 = accel_x100;
    portEXIT_CRITICAL(&s_mux);
}

/** 写入 IMU 推导的横向加速度，单位为 0.01 g。 */
void obd_data_set_lat_accel_imu_x100(int16_t accel_x100)
{
    if (accel_x100 != -32768 && (accel_x100 < -500 || accel_x100 > 500)) return;
    portENTER_CRITICAL(&s_mux);
    s_lat_accel_imu_x100 = accel_x100;
    portEXIT_CRITICAL(&s_mux);
}

/** 写入 IMU 推导的纵向加速度，单位为 0.01 g。 */
void obd_data_set_lon_accel_imu_x100(int16_t accel_x100)
{
    if (accel_x100 != -32768 && (accel_x100 < -500 || accel_x100 > 500)) return;
    portENTER_CRITICAL(&s_mux);
    s_lon_accel_imu_x100 = accel_x100;
    portEXIT_CRITICAL(&s_mux);
}

/** 写入最新的 RS485 刹车温度传输状态。 */
void obd_data_set_brake_rs485_status(brake_rs485_status_t status)
{
    portENTER_CRITICAL(&s_mux);
    s_brake_rs485_status = status;
    portEXIT_CRITICAL(&s_mux);
}

/** 写入最新的实际档位，并标记当前结果有效。 */
void obd_data_set_actual_gear(enGear gear)
{
    portENTER_CRITICAL(&s_mux);
    s_actual_gear = gear;
    s_actual_gear_valid = true;
    portEXIT_CRITICAL(&s_mux);
}

/** 在数据源不可靠时清除缓存的实际档位。 */
void obd_data_clear_actual_gear(void)
{
    portENTER_CRITICAL(&s_mux);
    s_actual_gear = GEAR_NEUTRAL;
    s_actual_gear_valid = false;
    portEXIT_CRITICAL(&s_mux);
}

/** 返回最新的刹车温度，单位为 0.1 摄氏度。 */
int16_t obd_data_get_brake_temp_x10(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_brake_temp_x10;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 返回最新的 OBD 横向加速度，单位为 0.01 g。 */
int16_t obd_data_get_lat_accel_x100(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_lat_accel_x100;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 返回最新的 OBD 纵向加速度，单位为 0.01 g。 */
int16_t obd_data_get_lon_accel_x100(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_lon_accel_x100;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 返回最新的 IMU 横向加速度，单位为 0.01 g。 */
int16_t obd_data_get_lat_accel_imu_x100(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_lat_accel_imu_x100;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 返回最新的 IMU 纵向加速度，单位为 0.01 g。 */
int16_t obd_data_get_lon_accel_imu_x100(void)
{
    int16_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_lon_accel_imu_x100;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 返回最新的 RS485 刹车温度传输状态。 */
brake_rs485_status_t obd_data_get_brake_rs485_status(void)
{
    brake_rs485_status_t val;
    portENTER_CRITICAL(&s_mux);
    val = s_brake_rs485_status;
    portEXIT_CRITICAL(&s_mux);
    return val;
}

/** 返回当前有效的实际档位。 */
bool obd_data_get_actual_gear(enGear *gear_out)
{
    bool valid;

    portENTER_CRITICAL(&s_mux);
    valid = s_actual_gear_valid;
    if (gear_out != NULL) {
        *gear_out = s_actual_gear;
    }
    portEXIT_CRITICAL(&s_mux);

    return valid;
}

/**
 * @brief 根据转速和车速计算并判断档位
 * @param rpm 发动机转速 (RPM)
 * @param speed 车速 (km/h)
 * @return 计算出的档位
 */
/**
 * 根据转速和车速推断当前档位
 *
 * 核心职责：结合车型传动比范围，给 UI 和业务逻辑提供稳定档位。
 */
static enGear calculate_gear_legacy(float rpm, float speed) {
    static enGear s_last_gear = GEAR_NEUTRAL;
    // ========== 输入校验 ==========
    // 任一信号缺失时直接回到空挡，避免沿用上一次档位造成误判。
    // 1. 检查输入数据有效性
    if (rpm <= 0 || speed <= 0) {
        s_last_gear = GEAR_NEUTRAL;
        return GEAR_NEUTRAL;
    }

    // 2. 使用当前车辆配置计算总传动比
    const vehicle_profile_t *profile = vehicle_profile_get_active();
    // ========== 传动比匹配 ==========
    // 必须使用当前车型配置，否则不同终传比会让档位判断整体偏移。
    float calc_const = vehicle_profile_calc_constant(profile);
    float total_ratio = rpm / (speed * calc_const);
    ESP_LOGD("gear", "RPM=%.0f Speed=%.1f ratio=%.2f", rpm, speed, total_ratio);

    // 3. 与各档位范围进行比较
    uint8_t range_count = 0;
    const gear_ratio_range_t *ranges = vehicle_profile_get_gear_ranges(&range_count);
    for (int i = 0; i < range_count; i++) {
        if (total_ratio >= ranges[i].min_ratio &&
            total_ratio <= ranges[i].max_ratio) {
            s_last_gear = ranges[i].gear;
            return ranges[i].gear;
        }
    }
    
    // 4. 如果在所有范围外，检查是否可能为空档（转速高车速为零）
    if (rpm > 800 && speed < 5) { // 怠速以上且几乎静止
        s_last_gear = GEAR_NEUTRAL;
        return GEAR_NEUTRAL;
    }
    
    // 5. 无法识别的传动比 返回上一次档位
    return s_last_gear;
}


/**
 * @brief 里程统计任务
 * @param pvParameter 参数
 * @return 无
 * @note  
 * @note 里程统计任务
 */
/**
 * 里程统计定时器回调
 *
 * 每秒刷新一次里程相关统计，并按节流频率输出调试日志。
 */
enGear calculate_gear(float rpm, float speed)
{
    static enGear s_last_gear = GEAR_NEUTRAL;
    static int64_t s_last_gear_change_us = 0;
    const vehicle_profile_t *profile;
    const gear_ratio_range_t *ranges;
    float calc_const;
    float total_ratio;
    float best_error = 1000.0f;
    float last_error = 1000.0f;
    enGear best_gear = GEAR_NEUTRAL;
    uint8_t range_count = 0;
    int64_t now_us = esp_timer_get_time();

    if (rpm < 450.0f || speed < 2.0f) {
        s_last_gear = GEAR_NEUTRAL;
        s_last_gear_change_us = now_us;
        return GEAR_NEUTRAL;
    }

    profile = vehicle_profile_get_active();
    calc_const = vehicle_profile_calc_constant(profile);
    if (profile == NULL || calc_const <= 0.0f) {
        s_last_gear = GEAR_NEUTRAL;
        s_last_gear_change_us = now_us;
        return GEAR_NEUTRAL;
    }

    total_ratio = rpm / (speed * calc_const);
    ranges = vehicle_profile_get_gear_ranges(&range_count);
    ESP_LOGD("gear", "GEAR-DERIVED rpm=%.0f speed=%.1f ratio=%.3f", rpm, speed, total_ratio);

    for (uint8_t i = 0; i < range_count; ++i) {
        float center_ratio;
        float error;

        if (ranges[i].max_ratio <= 0.0f || ranges[i].min_ratio <= 0.0f) {
            continue;
        }

        center_ratio = (ranges[i].min_ratio + ranges[i].max_ratio) * 0.5f;
        error = fabsf(total_ratio - center_ratio) / center_ratio;

        if (ranges[i].gear == s_last_gear) {
            last_error = error;
        }

        if (total_ratio >= ranges[i].min_ratio &&
            total_ratio <= ranges[i].max_ratio &&
            error < best_error) {
            best_error = error;
            best_gear = ranges[i].gear;
        }
    }

    if (best_gear != GEAR_NEUTRAL) {
        if (s_last_gear != GEAR_NEUTRAL &&
            best_gear != s_last_gear &&
            last_error <= (profile->gear_tolerance + 0.03f) &&
            (now_us - s_last_gear_change_us) < 400000LL) {
            return s_last_gear;
        }

        if (best_gear != s_last_gear) {
            s_last_gear = best_gear;
            s_last_gear_change_us = now_us;
        }
        return best_gear;
    }

    if (rpm > 800.0f && speed < 5.0f) {
        s_last_gear = GEAR_NEUTRAL;
        s_last_gear_change_us = now_us;
        return GEAR_NEUTRAL;
    }

    if (s_last_gear != GEAR_NEUTRAL &&
        (now_us - s_last_gear_change_us) < 800000LL) {
        return s_last_gear;
    }

    if (speed >= 5.0f) {
        enGear fallback = calculate_gear_legacy(rpm, speed);
        if (fallback != GEAR_NEUTRAL) {
            s_last_gear = fallback;
            s_last_gear_change_us = now_us;
            return fallback;
        }
    }

    s_last_gear = GEAR_NEUTRAL;
    s_last_gear_change_us = now_us;
    return GEAR_NEUTRAL;
}

static void mileage_timer_cb(void* arg)
{
    static uint16_t usPrintCnt = 0;
    (void)arg;
    nvs_stat_update_speed(obd_data_get_speed(), 1000);

    // 仅在车辆移动时打印，避免静止阶段持续刷日志。
    if(obd_data_get_speed() > 0){
        usPrintCnt++;
        if(usPrintCnt >= 20){
            usPrintCnt = 0;
            nvs_stat_t stat = nvs_stat_get_mileage();
            ESP_LOGI("MileageStat", " odometer: %" PRIu64 ", trip: %" PRIu64 ", run_time: %" PRIu64 ", max_speed: %d, avg_speed: %d, speed: %d", stat.odometer_m, stat.trip_m, stat.run_time_s, stat.max_speed_kmh, stat.avg_speed_kmh, obd_data_get_speed());
        }
    }
}

/**
 * @brief 初始化里程统计任务
 * @return 无
 * @note  
 * @note 初始化里程统计任务
 */
/**
 * 初始化里程统计任务
 *
 * 创建周期定时器，让里程和 trip 统计持续更新。
 */
void vMileageDataStatisticTask(void)
{
    ESP_LOGI("MileageStat", "MileageStatTask Init Start");
    static esp_timer_handle_t s_timer = NULL;
    if(!s_timer){
        const esp_timer_create_args_t args={
            .callback = mileage_timer_cb,
            .arg = NULL,
            .name = "mile_stat"
        };
        if(esp_timer_create(&args,&s_timer)==ESP_OK){
            // 固定 1 秒采样一次，方便距离和运行时长按统一节奏累加。
            esp_timer_start_periodic(s_timer, 1000000); //1s
        }
    }
}
  
