#include <string.h>

#include "bsp_obd_dsp/boards/board_api.h"
#include "bsp_obd_dsp/boards/board_ws_185_spec.h"
#include "bsp_obd_dsp/exio/TCA9554PWR.h"
#include "bsp_obd_dsp/i2c_driver/I2C_Driver.h"
#include "bsp_obd_dsp/lcd_driver/ST77916.h"
#include "bsp_obd_dsp/touch_driver/CST816.h"
#include "esp_check.h"

static const board_profile_t s_board_profile = {
    .name = BOARD_WS_185_NAME,
    .hor_res = BOARD_WS_185_H_RES,
    .ver_res = BOARD_WS_185_V_RES,
    .draw_buffer_lines = BOARD_WS_185_DRAW_BUFFER_LINES,
    .color_bits = BOARD_WS_185_COLOR_BITS,
    .has_touch = BOARD_WS_185_HAS_TOUCH,
    .touch_swap_xy = BOARD_WS_185_TOUCH_SWAP_XY,
    .touch_mirror_x = BOARD_WS_185_TOUCH_MIRROR_X,
    .touch_mirror_y = BOARD_WS_185_TOUCH_MIRROR_Y,
};

/** 执行 WS185 板级基础初始化。 */
esp_err_t board_ws_185_init(void)
{
    I2C_Init();
    return EXIO_Init();
}

/** 为 WS185 显示驱动注册刷屏完成回调。 */
esp_err_t board_ws_185_register_display_flush_ready_callback(board_display_flush_ready_cb_t cb, void *user_ctx)
{
    LCD_SetFlushCallback(cb, user_ctx);
    return ESP_OK;
}

/**
 * 初始化 WS185 显示上下文
 *
 * 核心职责：启动 LCD 与触摸驱动，并把底层句柄封装成统一显示上下文
 */
esp_err_t board_ws_185_display_init(board_display_context_t *ctx)
{
    ESP_RETURN_ON_FALSE(ctx, ESP_ERR_INVALID_ARG, "board_ws_185", "display context is null");

    memset(ctx, 0, sizeof(*ctx));
    LCD_Backlight = 0;
    LCD_Init();

    ctx->hor_res = BOARD_WS_185_H_RES;
    ctx->ver_res = BOARD_WS_185_V_RES;
    ctx->draw_buffer_lines = BOARD_WS_185_DRAW_BUFFER_LINES;
    ctx->color_bits = BOARD_WS_185_COLOR_BITS;
    ctx->has_touch = s_board_profile.has_touch;
    ctx->panel = panel_handle;
    ctx->panel_io = NULL;
    ctx->touch = tp;

    return (ctx->panel != NULL && ctx->touch != NULL) ? ESP_OK : ESP_FAIL;
}

/** 设置 WS185 背光亮度百分比。 */
esp_err_t board_ws_185_set_brightness(uint8_t percent)
{
    Set_Backlight(percent);
    return ESP_OK;
}

/** 返回 WS185 板卡静态配置。 */
const board_profile_t *board_ws_185_profile(void)
{
    return &s_board_profile;
}

/** 返回 WS185 板卡名称。 */
const char *board_ws_185_name(void)
{
    return s_board_profile.name;
}

/** 返回 WS185 是否带触摸能力。 */
bool board_ws_185_has_touch(void)
{
    return s_board_profile.has_touch;
}
