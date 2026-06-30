# `touch_driver`

这是触摸芯片 `CST816` 的驱动层。

## 它做的事

- 初始化触摸控制器
- 读取当前触摸点坐标
- 把触摸数据交给 LVGL 输入设备驱动

## 在主流程里的位置

`LCD_Init()` 内部会把触摸也一起带起来。之后 `app_main.c` 里通过：

- `indev_drv.read_cb = lvgl_touch_cb`
- `indev_drv.user_data = tp`

把触摸设备注册给 LVGL。

## 你需要建立的概念

在嵌入式 GUI 里，显示和触摸通常是两条链路：

- 显示链路：LVGL -> LCD panel
- 输入链路：Touch 芯片 -> LVGL input device

这个模块属于第二条链路。

## 就近说明

- [CST816.md](CST816.md)
