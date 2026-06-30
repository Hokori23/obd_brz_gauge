# `lcd_driver`

这是显示驱动层，核心芯片是 `ST77916`。

## 模块组成

- `ST77916.c/.h`：项目侧封装
- `esp_lcd_st77916/`：更底层的 panel 驱动

## 它做的事

1. 初始化显示总线和 panel。
2. 管理背光 PWM。
3. 暴露 `panel_handle` 给 LVGL 使用。
4. 提供 flush callback 接口，让 LVGL 把像素刷到屏幕。

## Web 开发者可以这样理解

- `LVGL` 类似 React/Vue 的渲染层
- `lcd_driver` 类似浏览器渲染后端或 native canvas backend

UI 组件不会自己画到硬件上，而是：

1. LVGL 生成绘制区域
2. `lvgl_flush_cb` 调用 panel draw
3. `lcd_driver` 把像素送到屏幕控制器

## 阅读重点

- `LCD_Init()`
- 背光初始化
- `panel_handle` 如何提供给 `app_main.c`
- 与 `touch_driver`、`exio` 的依赖关系

## 就近说明

- [ST77916.md](ST77916.md)
- [esp_lcd_st77916/README.md](esp_lcd_st77916/README.md)
