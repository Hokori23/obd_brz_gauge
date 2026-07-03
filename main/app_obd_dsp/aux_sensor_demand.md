# `aux_sensor_demand` 说明

这个模块负责“按当前页面需求决定该采哪些数据”。
它不真正采集数据，而是把 UI 侧的展示需求收敛成底层采集开关。

## 核心职责

1. 计算当前应启用的 OBD PID 需求掩码。
2. 判断是否需要打开 OBD G-force 监控流。
3. 判断是否需要打开 ZC6 档位监控流。
4. 决定刹车温度、油压、IMU 等附加采集模块是否应该运行。

## 为什么它重要

如果没有这一层，首页、设置页、扫描页和各类驱动模块都可能各自判断“要不要采”。
那样很容易出现状态漂移、重复采集或带宽浪费。

这个模块的价值就在于把“当前页面真正在显示什么”翻译成一份统一的采集需求。

## 主要入口

- `aux_sensor_demand_refresh()`
- `aux_sensor_demand_get_obd_mask()`
- `aux_sensor_demand_is_gforce_obd_enabled()`
- `aux_sensor_demand_is_zc6_gear_obd_enabled()`

## 典型调用时机

- 首页切页后
- 页面类型切换后
- 蓝牙扫描页等特殊页面进入或退出时
- 仪表配置变更后
