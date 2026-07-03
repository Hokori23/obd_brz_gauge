#include "qmi8658_gforce.h"

#include <math.h>

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_obd_dsp/obd_data_cache.h"
#include "bsp_obd_dsp/boards/board_api.h"
#include "bsp_obd_dsp/nvs_storage.h"
#if CONFIG_OBD_BOARD_WS_175_AMOLED
#include "bsp_obd_dsp/boards/board_ws_175_amoled_spec.h"
#endif

static const char *TAG = "qmi8658_g";

#define QMI8658_ADDR_LOW             0x6Bu
#define QMI8658_ADDR_HIGH            0x6Au
#define QMI8658_REG_WHOAMI           0x00u
#define QMI8658_REG_CTRL1            0x02u
#define QMI8658_REG_CTRL2            0x03u
#define QMI8658_REG_CTRL5            0x06u
#define QMI8658_REG_CTRL7            0x08u
#define QMI8658_REG_AX_L             0x35u
#define QMI8658_REG_RESET            0x60u
#define QMI8658_WHOAMI_EXPECTED      0x05u
#define QMI8658_RESET_VALUE          0xB0u
#define QMI8658_CTRL1_ADDR_AI        0x40u
#define QMI8658_CTRL2_ACCEL_4G_125HZ 0x16u
#define QMI8658_CTRL5_ACCEL_LPF      0x01u
#define QMI8658_CTRL7_ACCEL_ENABLE   0x01u
#define QMI8658_ACCEL_SCALE_G        (4.0f / 32768.0f)
#define QMI8658_SAMPLE_PERIOD_MS     20u
#define QMI8658_IDLE_PERIOD_MS       250u
#define QMI8658_RETRY_PERIOD_MS      1000u
#define QMI8658_STABILIZE_MS         60u
#define QMI8658_DEADZONE_G           0.015f
#define QMI8658_AXIS_ALPHA           0.18f

typedef struct {
    i2c_master_dev_handle_t dev;
    bool ready;
    bool bias_ready;
    float bias_x;
    float bias_y;
    float bias_z;
    float filtered_x;
    float filtered_y;
} qmi8658_state_t;

static volatile bool s_enabled;
static qmi8658_state_t s_state = {0};
static int64_t s_qmi8658_last_log_us = 0;

/** 读取 QMI8658 指定寄存器区间。 */
static esp_err_t qmi8658_read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, data, len, -1);
}

/** 向 QMI8658 写入单个寄存器值。 */
static esp_err_t qmi8658_write_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t value)
{
    uint8_t payload[2] = {reg, value};
    return i2c_master_transmit(dev, payload, sizeof(payload), -1);
}

/**
 * 尝试在指定 I2C 地址打开 QMI8658
 *
 * 核心职责：挂载设备、验证 WHOAMI、返回可用句柄
 */
static esp_err_t qmi8658_try_open(i2c_master_bus_handle_t bus,
                                  uint8_t addr,
                                  i2c_master_dev_handle_t *out_dev)
{
    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 400000,
    };
    i2c_master_dev_handle_t dev = NULL;
    uint8_t whoami = 0;
    esp_err_t err;

    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &dev), TAG, "add device failed");
    err = qmi8658_read_reg(dev, QMI8658_REG_WHOAMI, &whoami, 1);
    if (err != ESP_OK || whoami != QMI8658_WHOAMI_EXPECTED) {
        i2c_master_bus_rm_device(dev);
        return (err == ESP_OK) ? ESP_ERR_NOT_FOUND : err;
    }

    *out_dev = dev;
    return ESP_OK;
}

/**
 * 准备 QMI8658 运行环境
 *
 * 核心职责：探测器件、完成寄存器初始化、重置滤波状态
 */
static esp_err_t qmi8658_prepare(qmi8658_state_t *state)
{
    i2c_master_bus_handle_t bus = NULL;
    esp_err_t err;

    if (state->ready) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(board_get_shared_i2c_bus(&bus), TAG, "shared i2c unavailable");

    err = qmi8658_try_open(bus, QMI8658_ADDR_HIGH, &state->dev);
    if (err != ESP_OK) {
        err = qmi8658_try_open(bus, QMI8658_ADDR_LOW, &state->dev);
    }
    ESP_RETURN_ON_ERROR(err, TAG, "probe failed");

    ESP_RETURN_ON_ERROR(qmi8658_write_reg(state->dev, QMI8658_REG_RESET, QMI8658_RESET_VALUE),
                        TAG, "reset failed");
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_RETURN_ON_ERROR(qmi8658_write_reg(state->dev, QMI8658_REG_CTRL1, QMI8658_CTRL1_ADDR_AI),
                        TAG, "ctrl1 failed");
    ESP_RETURN_ON_ERROR(qmi8658_write_reg(state->dev, QMI8658_REG_CTRL2, QMI8658_CTRL2_ACCEL_4G_125HZ),
                        TAG, "ctrl2 failed");
    ESP_RETURN_ON_ERROR(qmi8658_write_reg(state->dev, QMI8658_REG_CTRL5, QMI8658_CTRL5_ACCEL_LPF),
                        TAG, "ctrl5 failed");
    ESP_RETURN_ON_ERROR(qmi8658_write_reg(state->dev, QMI8658_REG_CTRL7, QMI8658_CTRL7_ACCEL_ENABLE),
                        TAG, "ctrl7 failed");

    state->ready = true;
    state->bias_ready = false;
    state->filtered_x = 0.0f;
    state->filtered_y = 0.0f;
    ESP_LOGI(TAG, "QMI8658 ready");
    return ESP_OK;
}

/** 读取一次三轴加速度，并换算成 g 值。 */
static esp_err_t qmi8658_read_xyz_g(qmi8658_state_t *state, float *x_g, float *y_g, float *z_g)
{
    uint8_t raw[6] = {0};
    int16_t x_raw;
    int16_t y_raw;
    int16_t z_raw;

    ESP_RETURN_ON_ERROR(qmi8658_read_reg(state->dev, QMI8658_REG_AX_L, raw, sizeof(raw)), TAG, "read failed");
    x_raw = (int16_t)(((uint16_t)raw[1] << 8) | raw[0]);
    y_raw = (int16_t)(((uint16_t)raw[3] << 8) | raw[2]);
    z_raw = (int16_t)(((uint16_t)raw[5] << 8) | raw[4]);
    *x_g = (float)x_raw * QMI8658_ACCEL_SCALE_G;
    *y_g = (float)y_raw * QMI8658_ACCEL_SCALE_G;
    *z_g = (float)z_raw * QMI8658_ACCEL_SCALE_G;
    return ESP_OK;
}

/**
 * 采样静止偏置
 *
 * 核心职责：在设备稳定时多次取样，生成后续映射要减掉的零偏
 */
static void qmi8658_capture_bias(qmi8658_state_t *state)
{
    float sum_x = 0.0f;
    float sum_y = 0.0f;
    float sum_z = 0.0f;
    int samples = 0;

    for (int i = 0; i < 32; ++i) {
        float x_g = 0.0f;
        float y_g = 0.0f;
        float z_g = 0.0f;
        if (qmi8658_read_xyz_g(state, &x_g, &y_g, &z_g) == ESP_OK) {
            sum_x += x_g;
            sum_y += y_g;
            sum_z += z_g;
            ++samples;
        }
        vTaskDelay(pdMS_TO_TICKS(8));
    }

    if (samples > 0) {
        state->bias_x = sum_x / (float)samples;
        state->bias_y = sum_y / (float)samples;
        state->bias_z = sum_z / (float)samples;
        state->bias_ready = true;
        state->filtered_x = 0.0f;
        state->filtered_y = 0.0f;
        ESP_LOGI(TAG, "QMI8658 bias x=%.4fg y=%.4fg z=%.4fg",
                 state->bias_x, state->bias_y, state->bias_z);
    }
}

/** 把 g 值转换成 0.01g 定点整数，并完成死区与限幅。 */
static int16_t qmi8658_to_x100(float axis_g)
{
    if (fabsf(axis_g) < QMI8658_DEADZONE_G) {
        axis_g = 0.0f;
    }
    if (axis_g > 4.99f) {
        axis_g = 4.99f;
    } else if (axis_g < -4.99f) {
        axis_g = -4.99f;
    }
    return (int16_t)lrintf(axis_g * 100.0f);
}

/** 判断当前显示旋转是否需要翻转 G-force 轨迹方向。 */
static bool qmi8658_display_rotation_flips_plot(void)
{
#if CONFIG_OBD_BOARD_WS_175_AMOLED
    return nvs_cfg_get_display_rotation_degrees(nvs_cfg_get(),
                                                BOARD_WS_175_AMOLED_DISPLAY_ROTATION) == 180u;
#else
    return false;
#endif
}

/**
 * 把芯片坐标映射成车辆横纵向坐标
 *
 * 核心职责：扣除静态偏置、适配安装方向、输出车辆语义下的 G-force
 */
static void qmi8658_map_vehicle_axes(float raw_x_g,
                                     float raw_y_g,
                                     float raw_z_g,
                                     float bias_x_g,
                                     float bias_y_g,
                                     float bias_z_g,
                                     float *lat_g,
                                     float *lon_g)
{
    float corrected_x_g = raw_x_g - bias_x_g;
    float corrected_y_g = raw_y_g - bias_y_g;
    float corrected_z_g = raw_z_g - bias_z_g;

    if (lat_g == NULL || lon_g == NULL) {
        return;
    }

    /* Waveshare 的演示代码按平放桌面的姿态理解 X/Y。
     * 这里的仪表是按车内仪表方向安装的，屏幕法线更接近车辆前后方向。
     * 因此横向加速度仍取板内平面的 Y 轴，纵向加速度改取板法线方向的 Z 轴，
     * 避免沿用演示坐标后把车辆横纵向含义映射反了。
     */
    *lat_g = -corrected_y_g;
    (void)corrected_x_g;
    *lon_g = corrected_z_g;

    if (qmi8658_display_rotation_flips_plot()) {
        *lat_g = -*lat_g;
        *lon_g = -*lon_g;
    }
}

/** 按节流频率输出一次 IMU G-force 诊断日志。 */
static void qmi8658_log_sample(const qmi8658_state_t *state,
                               float raw_x_g,
                               float raw_y_g,
                               float raw_z_g,
                               float lat_g,
                               float lon_g)
{
    int64_t now_us;

    if (state == NULL) {
        return;
    }

    now_us = esp_timer_get_time();
    if ((now_us - s_qmi8658_last_log_us) < 1000000LL) {
        return;
    }

    s_qmi8658_last_log_us = now_us;
    ESP_LOGI(TAG,
             "GFORCE IMU raw=(%d,%d,%d)x100g bias=(%d,%d,%d)x100g mapped=(%d,%d)x100g filt=(%d,%d)x100g",
             (int)lrintf(raw_x_g * 100.0f),
             (int)lrintf(raw_y_g * 100.0f),
             (int)lrintf(raw_z_g * 100.0f),
             (int)lrintf(state->bias_x * 100.0f),
             (int)lrintf(state->bias_y * 100.0f),
             (int)lrintf(state->bias_z * 100.0f),
             (int)lrintf(lat_g * 100.0f),
             (int)lrintf(lon_g * 100.0f),
             (int)lrintf(state->filtered_x * 100.0f),
             (int)lrintf(state->filtered_y * 100.0f));
}

/**
 * QMI8658 G-force 后台任务
 *
 * 核心职责：按需启动 IMU、采样滤波、写入车辆横纵向加速度
 */
static void qmi8658_gforce_task(void *arg)
{
    qmi8658_state_t *state = (qmi8658_state_t *)arg;

    while (1) {
        if (!s_enabled) {
            obd_data_set_lat_accel_imu_x100(-32768);
            obd_data_set_lon_accel_imu_x100(-32768);
            vTaskDelay(pdMS_TO_TICKS(QMI8658_IDLE_PERIOD_MS));
            continue;
        }

        if (qmi8658_prepare(state) != ESP_OK) {
            obd_data_set_lat_accel_imu_x100(-32768);
            obd_data_set_lon_accel_imu_x100(-32768);
            vTaskDelay(pdMS_TO_TICKS(QMI8658_RETRY_PERIOD_MS));
            continue;
        }

        // ========== 静态校准 ==========
        // 每次重新启用都重新采一次零偏，避免上次安装姿态残留影响本次显示。
        if (!state->bias_ready) {
            vTaskDelay(pdMS_TO_TICKS(QMI8658_STABILIZE_MS));
            qmi8658_capture_bias(state);
            continue;
        }

        float x_g = 0.0f;
        float y_g = 0.0f;
        float z_g = 0.0f;
        if (qmi8658_read_xyz_g(state, &x_g, &y_g, &z_g) == ESP_OK) {
            float lat_g = 0.0f;
            float lon_g = 0.0f;

            qmi8658_map_vehicle_axes(x_g, y_g, z_g,
                                     state->bias_x, state->bias_y, state->bias_z,
                                     &lat_g, &lon_g);

            state->filtered_x += QMI8658_AXIS_ALPHA * (lat_g - state->filtered_x);
            state->filtered_y += QMI8658_AXIS_ALPHA * (lon_g - state->filtered_y);

            obd_data_set_lat_accel_imu_x100(qmi8658_to_x100(state->filtered_x));
            obd_data_set_lon_accel_imu_x100(qmi8658_to_x100(state->filtered_y));
            qmi8658_log_sample(state, x_g, y_g, z_g, lat_g, lon_g);
        }

        vTaskDelay(pdMS_TO_TICKS(QMI8658_SAMPLE_PERIOD_MS));
    }
}

/** 启动 QMI8658 G-force 采样任务。 */
void qmi8658_gforce_start(void)
{
    static bool started = false;

    if (started) {
        return;
    }

    started = true;
    xTaskCreate(qmi8658_gforce_task, "qmi8658_g", 4096, &s_state, 4, NULL);
}

/** 切换 QMI8658 G-force 采样开关。 */
void qmi8658_gforce_set_enabled(bool enabled)
{
    static bool last_enabled = false;

    if (enabled && !last_enabled) {
        s_state.bias_ready = false;
        s_state.filtered_x = 0.0f;
        s_state.filtered_y = 0.0f;
        s_qmi8658_last_log_us = 0;
    }

    s_enabled = enabled;
    last_enabled = enabled;
}

/** 返回 QMI8658 G-force 采样当前是否启用。 */
bool qmi8658_gforce_is_enabled(void)
{
    return s_enabled;
}
