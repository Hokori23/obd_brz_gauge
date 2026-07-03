# `app_obd_dsp` 模块说明

这一层是业务层，不直接操作 BLE、LCD 或触摸硬件。
它负责维护“车况数据”和“车型规则”，给 UI 和驱动层提供稳定的业务语义。

## 当前核心模块

- `obd_data_cache.*`
- `vehicle_profiles.*`
- `aux_sensor_demand.*`

## `obd_data_cache`

这是全项目最核心的共享状态缓存，保存实时车况数据，例如：

- 转速、车速
- 冷却液温度、机油温度、进气温度
- 节气门、负载、电压
- 机油压力、刹车温度、增压值
- OBD/IMU G-force
- 实际档位

它主要负责：

1. 提供统一的 `set/get` 接口给驱动层和 UI 层使用。
2. 对共享数据做基本并发保护。
3. 对 RPM、车速等高频数据做平滑处理。
4. 对温度、压力、档位等值做范围校验或推导。

## `vehicle_profiles`

这是车型抽象层，避免把所有逻辑硬编码成单一车型。
它定义每种车型的关键规则，例如：

- 主减速比和各档齿比
- 轮胎滚动半径
- 档位容差
- 机油温度查询策略
- 是否支持增压显示

UI、档位推导和 OBD 查询策略都会依赖这里的配置。

## `aux_sensor_demand`

这是“按需采集”的状态汇总模块。
它会根据当前页面和显示项，统一推导：

- 当前需要轮询哪些 OBD PID
- 是否需要启用 OBD G-force 流
- 是否需要启用 ZC6 档位监控流
- 是否需要打开 RS485 刹车温度、油压、IMU 等附加采集

如果你在改首页显示逻辑，这个模块通常也要一起看。

## 建议阅读顺序

1. [obd_data_cache.h](obd_data_cache.h)
2. [obd_data_cache.c](obd_data_cache.c)
3. [vehicle_profiles.h](vehicle_profiles.h)
4. [vehicle_profiles.c](vehicle_profiles.c)
5. [aux_sensor_demand.h](aux_sensor_demand.h)
6. [aux_sensor_demand.c](aux_sensor_demand.c)

## 就近说明

- [obd_data_cache.md](obd_data_cache.md)
- [vehicle_profiles.md](vehicle_profiles.md)
- [aux_sensor_demand.md](aux_sensor_demand.md)
