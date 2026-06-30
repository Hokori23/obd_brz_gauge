# `esp_lcd_st77916`

这是更底层的 `ST77916` panel 驱动。

如果 [../ST77916.c](../ST77916.c) 是“项目封装层”，这里就是“面板驱动层”。

## 它做什么

- 实现 `esp_lcd_panel_t` 接口
- 提供 `reset/init/draw_bitmap/mirror/swap_xy/disp_on_off` 等操作
- 处理命令和像素数据如何通过 QSPI/SPI 发给屏幕

## 你可以怎么理解

这一层相当于“标准驱动适配接口”：

- 上层只知道自己拿到一个 panel handle
- 底层负责把这些统一操作翻译成 ST77916 能懂的命令

## 阅读建议

如果你第一次看嵌入式显示驱动，只需要先理解：

- `esp_lcd_new_panel_st77916()`
- `panel_st77916_init()`
- `panel_st77916_draw_bitmap()`

别一开始就试图吃透全部命令细节。
