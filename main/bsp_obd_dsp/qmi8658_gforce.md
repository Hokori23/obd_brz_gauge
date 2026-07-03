# `qmi8658_gforce` 说明

这个模块负责从板载 QMI8658 IMU 采样加速度，并换算成 UI 可直接显示的车辆横纵向 G-force。

## 核心职责

1. 通过共享 I2C 总线探测 QMI8658。
2. 初始化加速度量程、低通和采样配置。
3. 在设备静止时采集零偏。
4. 把芯片坐标映射成车辆语义下的横向和纵向加速度。
5. 对结果做简单滤波并写入共享缓存。

## 这个模块解决了什么问题

IMU 原始坐标和车辆显示坐标不是一回事。
如果直接把芯片的 X/Y/Z 往 UI 上丢，显示方向很容易反，数值也会因为安装姿态和零偏产生明显漂移。

这个模块把这些问题集中处理掉，让上层只关心：

- 横向 G-force
- 纵向 G-force
- 当前是否启用 IMU 采样

## 主要入口

- `qmi8658_gforce_start()`
- `qmi8658_gforce_set_enabled()`
- `qmi8658_gforce_is_enabled()`

## 阅读建议

如果要改 IMU 显示效果，建议按下面顺序看：

1. `qmi8658_prepare()`
2. `qmi8658_capture_bias()`
3. `qmi8658_map_vehicle_axes()`
4. `qmi8658_gforce_task()`
