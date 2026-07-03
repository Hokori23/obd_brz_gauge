# `app_lvgl_port` 说明

这个模块负责项目里的 LVGL 运行时接入。
它是 UI 层和底层显示驱动之间最关键的一层胶水代码。

## 核心职责

1. 初始化 LVGL 运行环境。
2. 接入显示和触摸驱动。
3. 提供全局锁，保证多任务环境下的 LVGL 调用安全。
4. 驱动 LVGL 定时处理循环。
5. 记录页面切换和渲染相关的性能采样信息。

## 为什么它重要

在这个项目里，UI 不是单线程页面渲染，而是和 BLE、传感器、统计任务并行运行。
如果 LVGL 锁、刷屏回调或缓冲区策略有问题，很容易出现：

- 花屏
- 触摸异常
- 死锁
- 页面卡顿

所以这个模块虽然不是业务逻辑，但实际非常关键。

## 建议优先关注的点

- 显示缓冲区怎么分配
- 刷屏完成回调怎么接回 LVGL
- `app_lvgl_lock()` / `app_lvgl_unlock()` 在哪里被使用
- 性能采样点是怎么埋的

## 建议阅读顺序

1. [app_lvgl_port.h](app_lvgl_port.h)
2. [app_lvgl_port.c](app_lvgl_port.c)
3. [export_path/ui_home_runtime.c](export_path/ui_home_runtime.c)
