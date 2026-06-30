# `esp_lcd_st77916.c/.h` 阅读说明

这是 ST77916 的底层 panel 驱动实现。

## 核心角色

它实现的是 `esp_lcd_panel_t` 这一套标准接口，也就是把：

- reset
- init
- draw bitmap
- mirror / swap xy
- display on/off

这些标准动作映射到 ST77916 的寄存器命令。

## 为什么要有这一层

这样上层 LVGL 或项目代码就不需要知道：

- 命令码是多少
- QSPI 包格式是什么
- 像素数据怎么发

## 关键函数

### `esp_lcd_new_panel_st77916()`

构造 panel 对象，填充函数指针。

### `tx_param()` / `tx_color()`

把普通命令或像素数据封装成底层总线需要的格式。

### `panel_st77916_reset()`

执行硬件复位或软件复位。

### `panel_st77916_draw_bitmap()`

这是最关键的显示输出函数。LVGL 最终就是通过它把像素块刷到屏幕。

## 对 Web 开发者最重要的理解

UI 框架并不是直接画到屏幕，它只是不断调用一个“把这块像素贴上去”的底层函数。

这里的 `draw_bitmap()` 就是那个最终落地点。
