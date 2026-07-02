#include "bsp_obd_dsp/boards/board_api.h"

esp_err_t board_ws_185_init(void);
esp_err_t board_ws_185_register_display_flush_ready_callback(board_display_flush_ready_cb_t cb, void *user_ctx);
esp_err_t board_ws_185_display_init(board_display_context_t *ctx);
esp_err_t board_ws_185_set_brightness(uint8_t percent);
const board_profile_t *board_ws_185_profile(void);
const char *board_ws_185_name(void);
bool board_ws_185_has_touch(void);

esp_err_t board_ws_175_amoled_init(void);
esp_err_t board_ws_175_amoled_register_display_flush_ready_callback(board_display_flush_ready_cb_t cb, void *user_ctx);
esp_err_t board_ws_175_amoled_display_init(board_display_context_t *ctx);
esp_err_t board_ws_175_amoled_set_brightness(uint8_t percent);
const board_profile_t *board_ws_175_amoled_profile(void);
const char *board_ws_175_amoled_name(void);
bool board_ws_175_amoled_has_touch(void);

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
