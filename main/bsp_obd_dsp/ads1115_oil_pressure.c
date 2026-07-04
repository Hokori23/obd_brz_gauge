#include "ads1115_oil_pressure.h"

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "bsp_obd_dsp/boards/board_api.h"
#include "bsp_obd_dsp/nvs_storage.h"
#include "app_obd_dsp/obd_data_cache.h"

#define TAG "oil_press"

// ADS1115 的 I2C 地址。
// 当 ADDR 引脚接地时，地址固定为 0x48。
#define ADS1115_ADDR            0x48
#define ADS1115_REG_CONV        0x00
#define ADS1115_REG_CONFIG      0x01

#define OIL_PRESS_POLL_MS       100
#define OIL_PRESS_IDLE_MS       250

// 标定参数：3.3V 供电传感器输出 0.5~2.5V，
// 对应 0.0~10.0 bar，信号线接在 ADS1115 的 AIN0。
#define OIL_PRESS_ADC_MIN_MV    500
#define OIL_PRESS_ADC_MAX_MV    2500
#define OIL_PRESS_MIN_BAR_X10   0
#define OIL_PRESS_MAX_BAR_X10   100

static bool s_started = false;
static volatile bool s_enabled = true;
static int64_t s_last_oil_read_fail_log_us = 0;

/**
 * 读取 ADS1115 的 AIN0 电压
 *
 * 核心职责：触发一次单次转换、等待完成、把原始结果换算成毫伏
 */
static esp_err_t ads1115_read_ain0_mv(int32_t *out_mv)
{
    if (out_mv == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // OS=1（启动单次转换）, MUX=AIN0/GND(100), PGA=±4.096V(001),
    // MODE=single-shot(1), DR=860SPS(111), COMP 禁用(11)
    // Config = 0x8000|0x4000|0x0200|0x0100|0x00E0|0x0003 = 0xC3E3
    uint16_t cfg = 0xC3E3u;
    uint8_t cfg_bytes[2] = {(uint8_t)(cfg >> 8), (uint8_t)(cfg & 0xFF)};

    esp_err_t err = board_i2c_reg_write(ADS1115_ADDR, ADS1115_REG_CONFIG, cfg_bytes, sizeof(cfg_bytes));
    if (err != ESP_OK) {
        return err;
    }

    // 860 SPS 的单次转换时间约为 1.2ms，这里留 3ms 余量。
    vTaskDelay(pdMS_TO_TICKS(3));

    uint8_t conv[2] = {0};
    err = board_i2c_reg_read(ADS1115_ADDR, ADS1115_REG_CONV, conv, sizeof(conv));
    if (err != ESP_OK) {
        return err;
    }

    int16_t raw = (int16_t)(((uint16_t)conv[0] << 8) | conv[1]);

    // PGA=±4.096V 时，LSB = 125uV = 0.125mV，因此毫伏值约等于 raw / 8。
    int32_t mv = (int32_t)raw / 8;
    if (mv < 0) {
        mv = 0;
    }

    *out_mv = mv;
    return ESP_OK;
}

/** 把传感器输出毫伏值映射成 0.1 bar 单位的油压。 */
static int16_t mv_to_oil_pressure_x10(int32_t mv)
{
    int32_t mv_clamped = mv;
    if (mv_clamped < OIL_PRESS_ADC_MIN_MV) {
        mv_clamped = OIL_PRESS_ADC_MIN_MV;
    }
    if (mv_clamped > OIL_PRESS_ADC_MAX_MV) {
        mv_clamped = OIL_PRESS_ADC_MAX_MV;
    }

    const int32_t in_span  = (OIL_PRESS_ADC_MAX_MV - OIL_PRESS_ADC_MIN_MV);
    const int32_t out_span = (OIL_PRESS_MAX_BAR_X10 - OIL_PRESS_MIN_BAR_X10);
    if (in_span <= 0 || out_span <= 0) {
        return OIL_PRESS_MIN_BAR_X10;
    }

    int32_t x10 = OIL_PRESS_MIN_BAR_X10 +
                  ((mv_clamped - OIL_PRESS_ADC_MIN_MV) * out_span) / in_span;

    if (x10 < OIL_PRESS_MIN_BAR_X10) {
        x10 = OIL_PRESS_MIN_BAR_X10;
    }
    if (x10 > OIL_PRESS_MAX_BAR_X10) {
        x10 = OIL_PRESS_MAX_BAR_X10;
    }

    return (int16_t)x10;
}

/**
 * 机油压力后台采样任务
 *
 * 核心职责：周期读取 ADS1115、电压转油压、同步到共享数据缓存
 */
static void oil_pressure_task(void *arg)
{
    (void)arg;

    uint32_t log_count = 0;

    while (1) {
        if (!s_enabled) {
            vTaskDelay(pdMS_TO_TICKS(OIL_PRESS_IDLE_MS));
            continue;
        }

        int32_t mv = 0;
        esp_err_t err = ads1115_read_ain0_mv(&mv);
        if (err == ESP_OK) {
            int16_t pressure_x10 = mv_to_oil_pressure_x10(mv);
            obd_data_set_oil_pressure_x10(pressure_x10);

            // 诊断模式下每 10 次输出一次原始电压和换算后的油压。
            if ((log_count++ % 10) == 0) {
                ESP_LOGI(TAG, "[OIL] AIN0=%ldmV => pressure_x10=%d (%.1fbar) [range: %d-%dmV -> 0.0-10.0bar]",
                         (long)mv, pressure_x10, (float)pressure_x10 / 10.0f,
                         OIL_PRESS_ADC_MIN_MV, OIL_PRESS_ADC_MAX_MV);
            }
        } else {
            ESP_LOGW(TAG, "[OIL] ADS1115 read failed: %s (addr=0x48)", esp_err_to_name(err));
            if ((esp_timer_get_time() - s_last_oil_read_fail_log_us) > 10000000LL) {
                s_last_oil_read_fail_log_us = esp_timer_get_time();
                nvs_error_log_recordf(TAG, err, "ADS1115 read failed addr=0x%02X", ADS1115_ADDR);
            }
            obd_data_set_oil_pressure_x10(-1);
        }

        vTaskDelay(pdMS_TO_TICKS(OIL_PRESS_POLL_MS));
    }
}

/** 启动机油压力采样任务。 */
void oil_pressure_start(void)
{
    if (s_started) {
        return;
    }

    s_started = true;
    xTaskCreate(oil_pressure_task, "oil_press", 3072, NULL, 4, NULL);
    ESP_LOGI(TAG, "=== ADS1115 Oil Pressure Init ===");
    ESP_LOGI(TAG, "I2C Addr: 0x%02X | Channel: AIN0", ADS1115_ADDR);
    ESP_LOGI(TAG, "Voltage Range: %dmV ~ %dmV", OIL_PRESS_ADC_MIN_MV, OIL_PRESS_ADC_MAX_MV);
    ESP_LOGI(TAG, "Pressure Range: 0.0 ~ 10.0 bar");
    ESP_LOGI(TAG, "Poll Interval: %dms", OIL_PRESS_POLL_MS);
    ESP_LOGI(TAG, "Task started. Check [OIL] logs for readings.");
}

/** 切换机油压力采样开关。 */
void oil_pressure_set_enabled(bool enabled)
{
    s_enabled = enabled;
}

/** 兼容旧接口名，转发到机油压力采样启动入口。 */
void ads1115_oil_pressure_start(void)
{
    oil_pressure_start();
}
