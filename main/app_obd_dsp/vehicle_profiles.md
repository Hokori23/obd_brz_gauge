# `vehicle_profiles.c/.h` 阅读说明

这组文件定义“不同车型的差异化参数”。

如果你从 Web 视角看，它像一个内置的策略表和配置中心。

## 为什么需要它

这个项目不只是显示通用 OBD 数值，还做了两件和车型强相关的事：

1. 估算档位
2. 选择机油温查询策略

这两件事如果写死，就只能跑在一个车型上。

## `vehicle_profile_t` 里最重要的字段

- `name`：车型名
- `final_drive_ratio`：主减速比
- `tire_rolling_radius_m`：轮胎滚动半径
- `gear_count`：档位数
- `gear_ratios[]`：各档传动比
- `gear_tolerance`：档位匹配容差
- `oil_temp_strategy`：机油温查询优先级
- `has_boost`：是否显示增压

## 机油温策略是什么意思

不同车厂的 OBD 暴露方式不一样，所以项目会按车型切换：

- 标准 `PID 0x5C`
- Toyota 风格 `21 01`
- UDS 风格 `22 10 17`
- Mazda 风格 `22 11 1F` / `22 13 10`

这就是为什么 [elm327_ble_client.c](../bsp_obd_dsp/elm327_ble_client.c) 里会有多套解析逻辑。

## 关键函数

### `vehicle_profile_set_active()`

切换当前车型，并把索引写回 `NVS`。

### `vehicle_profile_calc_constant()`

把轮胎半径和主减速比换算成档位计算常量，供 `calculate_gear()` 使用。

### `vehicle_profile_get_gear_ranges()`

根据各档传动比动态生成每档的容差区间。

它不是把区间硬编码死，而是启动后根据配置重建。

## 你读这份代码时要注意

- “车型支持”不只是 UI 文案变化，而是协议和算法变化。
- 这个文件本身不做 OBD 通信，它只是给通信层和业务层提供规则。
