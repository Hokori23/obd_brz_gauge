# `nvs_storage.c/.h` 阅读说明

`NVS` 是 ESP32 的非易失键值存储。这里的实现同时承担：

- 用户配置持久化
- 运行统计持久化

你可以把它看成“设备本地配置仓库 + 统计仓库”。

## 它保存的两类数据

### 1. `nvs_user_cfg_t`

偏配置性质，类似用户设置：

- OBD 协议
- UI 主题
- 记住的蓝牙设备名
- 动态首页启动页
- 亮度
- 车型索引
- 刹车温度和机油压力告警阈值
- 页面显示项映射

### 2. `nvs_stat_t`

偏运行数据，类似设备统计：

- 总里程
- 小计里程
- 总运行时间
- 最高速度
- 平均速度

## 启动时发生什么

`nvs_storage_init()` 做几件事：

1. 初始化 `nvs_flash`
2. 读取配置 blob
3. 读取统计 blob
4. 对新增字段做默认值修正
5. 创建互斥锁
6. 启动后台刷盘任务

## 为什么统计数据不立刻写

Flash 擦写次数有限。

所以这里的设计是：

- 内存里先累计
- `s_stat_dirty = true`
- 后台 `stat_flush_task()` 每 30 秒刷一次

这是典型的“写回缓存”思路。

## 你应该重点看

### `load_blob()`

读取失败时，直接把结构体清零并写入默认值。这是一个很嵌入式的容错方式。

### `nvs_cfg_set()`

配置变化就立即写，适合低频设置项。

### `nvs_stat_update_speed()`

把 `km/h + dt_ms` 换算成米数和运行时间，是里程逻辑的核心。

### `nvs_stat_reset_trip()`

只重置 trip 维度的数据：

- `trip_m`
- `trip_run_time_s`
- `max_speed_kmh`
- `avg_speed_kmh`

不会清零总运行时间 `run_time_s`。

## 容易混淆的点

- `trip` 和 `odometer` 都在这里维护，不在 UI 层。
- `default_page`、`vehicle_profile_idx` 这些都不是 UI 私有状态，而是持久化用户配置。
- 其中 `default_page` 当前表示动态首页启动页：`0=MENU`，`1..8=GAUGE 1..8`。
- 如果保存的 `default_page` 因删页而超过当前 `dashboard_cfg.gauge_page_count`，会在配置清洗时自动回退到 `MENU`。
