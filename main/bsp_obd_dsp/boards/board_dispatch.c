#include "bsp_obd_dsp/boards/board_api.h"

esp_err_t board_ws_185_init(void);
esp_err_t board_ws_185_register_display_flush_ready_callback(board_display_flush_ready_cb_t cb, void *user_ctx);
esp_err_t board_ws_185_display_init(board_display_context_t *ctx);
esp_err_t board_ws_185_set_brightness(uint8_t percent);
esp_err_t board_ws_185_i2c_reg_write(uint8_t device_addr, uint8_t reg_addr, const uint8_t *data, size_t len);
esp_err_t board_ws_185_i2c_reg_read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t len);
const board_profile_t *board_ws_185_profile(void);
const char *board_ws_185_name(void);
bool board_ws_185_has_touch(void);

esp_err_t board_ws_175_amoled_init(void);
esp_err_t board_ws_175_amoled_register_display_flush_ready_callback(board_display_flush_ready_cb_t cb, void *user_ctx);
esp_err_t board_ws_175_amoled_display_init(board_display_context_t *ctx);
esp_err_t board_ws_175_amoled_set_brightness(uint8_t percent);
esp_err_t board_ws_175_amoled_i2c_reg_write(uint8_t device_addr, uint8_t reg_addr, const uint8_t *data, size_t len);
esp_err_t board_ws_175_amoled_i2c_reg_read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t len);
const board_profile_t *board_ws_175_amoled_profile(void);
const char *board_ws_175_amoled_name(void);
bool board_ws_175_amoled_has_touch(void);

/** 按当前编译目标分发到对应板级初始化入口。 */
esp_err_t board_init(void)
{
#if CONFIG_OBD_BOARD_WS_185
    return board_ws_185_init();
#elif CONFIG_OBD_BOARD_WS_175_AMOLED
    return board_ws_175_amoled_init();
#else
#error "No board selected"
#endif
}

/** 为当前板卡注册显示刷屏完成回调。 */
esp_err_t board_register_display_flush_ready_callback(board_display_flush_ready_cb_t cb, void *user_ctx)
{
#if CONFIG_OBD_BOARD_WS_185
    return board_ws_185_register_display_flush_ready_callback(cb, user_ctx);
#elif CONFIG_OBD_BOARD_WS_175_AMOLED
    return board_ws_175_amoled_register_display_flush_ready_callback(cb, user_ctx);
#else
#error "No board selected"
#endif
}

/** 初始化当前板卡的显示上下文。 */
esp_err_t board_display_init(board_display_context_t *ctx)
{
#if CONFIG_OBD_BOARD_WS_185
    return board_ws_185_display_init(ctx);
#elif CONFIG_OBD_BOARD_WS_175_AMOLED
    return board_ws_175_amoled_display_init(ctx);
#else
#error "No board selected"
#endif
}

/** 设置当前板卡的显示亮度百分比。 */
esp_err_t board_set_brightness(uint8_t percent)
{
#if CONFIG_OBD_BOARD_WS_185
    return board_ws_185_set_brightness(percent);
#elif CONFIG_OBD_BOARD_WS_175_AMOLED
    return board_ws_175_amoled_set_brightness(percent);
#else
#error "No board selected"
#endif
}

/** 返回当前板卡对外共享的 I2C 总线句柄。 */
esp_err_t board_get_shared_i2c_bus(i2c_master_bus_handle_t *out_bus)
{
#if CONFIG_OBD_BOARD_WS_185
    (void)out_bus;
    return ESP_ERR_NOT_SUPPORTED;
#elif CONFIG_OBD_BOARD_WS_175_AMOLED
    extern esp_err_t board_ws_175_amoled_get_shared_i2c_bus(i2c_master_bus_handle_t *out_bus);
    return board_ws_175_amoled_get_shared_i2c_bus(out_bus);
#else
#error "No board selected"
#endif
}

esp_err_t board_i2c_reg_write(uint8_t device_addr,
                              uint8_t reg_addr,
                              const uint8_t *data,
                              size_t len)
{
#if CONFIG_OBD_BOARD_WS_185
    return board_ws_185_i2c_reg_write(device_addr, reg_addr, data, len);
#elif CONFIG_OBD_BOARD_WS_175_AMOLED
    return board_ws_175_amoled_i2c_reg_write(device_addr, reg_addr, data, len);
#else
#error "No board selected"
#endif
}

esp_err_t board_i2c_reg_read(uint8_t device_addr,
                             uint8_t reg_addr,
                             uint8_t *data,
                             size_t len)
{
#if CONFIG_OBD_BOARD_WS_185
    return board_ws_185_i2c_reg_read(device_addr, reg_addr, data, len);
#elif CONFIG_OBD_BOARD_WS_175_AMOLED
    return board_ws_175_amoled_i2c_reg_read(device_addr, reg_addr, data, len);
#else
#error "No board selected"
#endif
}

/** 返回当前编译目标板卡的静态配置描述。 */
const board_profile_t *board_profile(void)
{
#if CONFIG_OBD_BOARD_WS_185
    return board_ws_185_profile();
#elif CONFIG_OBD_BOARD_WS_175_AMOLED
    return board_ws_175_amoled_profile();
#else
#error "No board selected"
#endif
}

/** 返回当前板卡名称。 */
const char *board_name(void)
{
#if CONFIG_OBD_BOARD_WS_185
    return board_ws_185_name();
#elif CONFIG_OBD_BOARD_WS_175_AMOLED
    return board_ws_175_amoled_name();
#else
#error "No board selected"
#endif
}

/** 返回当前板卡是否带触摸能力。 */
bool board_has_touch(void)
{
#if CONFIG_OBD_BOARD_WS_185
    return board_ws_185_has_touch();
#elif CONFIG_OBD_BOARD_WS_175_AMOLED
    return board_ws_175_amoled_has_touch();
#else
#error "No board selected"
#endif
}
