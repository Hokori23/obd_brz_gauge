# `obd_data_cache.c/.h` 阅读说明

这组文件是项目的实时数据中心。你可以把它当成“单片机里的全局 store”。

## 它保存什么

- 发动机转速 `rpm`
- 车速 `speed`
- 水温 `coolant_temp`
- 机油温 `oil_temp`
- 进气温 `intake_temp`
- 发动机负载 `load_pct`
- 节气门开度 `tps`
- 电瓶电压 `bat_mv`
- 机油压力 `oil_pressure_x10`
- 刹车温度 `brake_temp_x10`
- 增压值 `boost_x10`
- MAP `map_kpa`
- 点火提前角 `ign_timing_x10`
- RS485 状态 `brake_rs485_status`

## 代码结构

1. 一组 `static volatile` 全局变量保存最新值。
2. `portMUX_TYPE s_mux` 负责临界区保护。
3. `set_*` 负责写入和基本校验。
4. `get_*` 负责读取，有些值会做平滑。
5. `calculate_gear()` 负责根据车型参数推导 `GEAR-DERIVED`。
6. `vMileageDataStatisticTask()` 负责启动里程统计定时器。

## 为什么这里不是队列或事件总线

因为这个项目的数据模型更像“仪表盘当前状态”，不是“事件历史”。

UI 只关心“现在的值是多少”，驱动层也只需要不断覆盖最新值，所以全局缓存是够用的。

## 你最值得看的 3 段逻辑

### 1. RPM / 速度平滑

`obd_data_get_rpm()` 和 `obd_data_get_speed()` 不是直接返回原值，而是用时间常数做一阶平滑。

这相当于：

- 后端收到原始数据
- 给前端展示前再做一个抗抖处理

目的很简单：UI 上的数字和指针不要跳得太凶。

### 2. 档位推导

`calculate_gear()` 不是直接读 ECU 档位，而是：

`rpm / speed -> 总传动比 -> 和车型配置里的各档范围做匹配`

当前实现还加入了两层稳定化：

- 先按各档位区间中心值选择最近匹配档位
- 对最近一次有效档位增加短时滞回，减少换挡边界抖动

所以它依赖 [vehicle_profiles.c](vehicle_profiles.c) 里的车型参数。

### 3. 里程统计

`mileage_timer_cb()` 每秒运行一次，用当前车速和时间换算距离，然后调用 `nvs_stat_update_speed()`。

也就是说：

- OBD 提供速度
- `obd_data_cache` 负责定时取速度
- `nvs_storage` 负责真正保存累计里程

## 你可以带着这些问题读

- 为什么部分默认值是 `-1`、`-100`、`-1000`？
- 哪些值是“无效值哨兵”？
- 平滑为什么放在 `get` 而不是 `set`？
- 哪些逻辑属于业务层，哪些应该留在驱动层？
