# `app_obd_dsp`

这是业务层。它不直接操作屏幕，也不直接控制 BLE 硬件，而是维护“车况数据”和“车型规则”。

如果类比 Web：

- `obd_data_cache.*` 类似运行时 store
- `vehicle_profiles.*` 类似多租户配置 / 策略表

## 文件

- `obd_data_cache.c/.h`
- `vehicle_profiles.c/.h`

## 1. `obd_data_cache`

这是项目的共享状态中心，保存当前实时数据：

- RPM
- 车速
- 水温
- 机油温
- 进气温
- 负载
- 节气门开度
- 电瓶电压
- 机油压力
- 刹车温度
- 增压值

### 它做的事

1. 提供 `set/get` API 给驱动层和 UI 使用。
2. 用 `portMUX_TYPE` 保护共享变量，避免并发读写冲突。
3. 对 `rpm`、`speed` 做平滑处理，避免指针或数字跳变。
4. 对机油温、刹车温、机油压力做有效范围校验。
5. 根据 `rpm + speed + 当前车型传动比` 估算档位。
6. 启动里程统计定时器，每秒把车速换算成里程并写入 `NVS` 统计。

### 重点函数

- `obd_data_set_*` / `obd_data_get_*`
- `calculate_gear()`
- `vMileageDataStatisticTask()`

### 你阅读时要注意

- 这里的 `get_rpm()` / `get_speed()` 不一定返回原始值，返回的是平滑后的值。
- `calculate_gear()` 不是从 ECU 直接读档位，而是推导出来的。

## 2. `vehicle_profiles`

这是多车型抽象层。项目当前不是把所有逻辑写死给 BRZ，而是抽象出：

- 主减速比
- 各档传动比
- 轮胎滚动半径
- 档位容差
- 机油温查询策略
- 是否支持增压显示

### 当前内置车型

- `BRZ ZC6`
- `BRZ ZD8`
- `MX-5 ND`
- `BMW G`

### 它做的事

1. 返回车型列表。
2. 保存当前激活车型。
3. 根据车型重建档位判定区间。
4. 为 OBD 层提供机油温查询策略。
5. 在切换车型时把索引写回 `NVS`。

### 重点函数

- `vehicle_profile_get_all()`
- `vehicle_profile_get_active()`
- `vehicle_profile_set_active()`
- `vehicle_profile_get_gear_ranges()`
- `vehicle_profile_get_oil_temp_strategy()`

## 建议阅读顺序

1. [obd_data_cache.h](obd_data_cache.h)
2. [obd_data_cache.c](obd_data_cache.c)
3. [vehicle_profiles.h](vehicle_profiles.h)
4. [vehicle_profiles.c](vehicle_profiles.c)

## 就近说明

- [obd_data_cache.md](obd_data_cache.md)
- [vehicle_profiles.md](vehicle_profiles.md)
