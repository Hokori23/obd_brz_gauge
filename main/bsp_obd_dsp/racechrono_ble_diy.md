# `racechrono_ble_diy.c/.h` 阅读说明

这个模块和 `elm327_ble_client` 正好相反。

- `elm327_ble_client` 是去连别人的 BLE 设备
- `racechrono_ble_diy` 是把本机变成 BLE 设备给别人连

## 它做什么

向手机侧应用暴露一个自定义 BLE GATT 服务，把当前车况数据持续 notify 出去。

## 数据来源

它并不直接采集数据，而是从 [obd_data_cache.c](../app_obd_dsp/obd_data_cache.c) 读取：

- RPM
- 速度
- 水温
- 进气温
- 机油温
- 负载
- TPS
- 电压
- 刹车温度

## 你最值得看的部分

### 1. GATT 属性表

`s_gatt_db` 定义了 service、characteristic、CCCD。

这相当于这个 BLE 服务对外暴露的“接口定义”。

### 2. `read_*` 系列函数

把共享缓存里的值读取出来，按 RaceChrono 期待的格式缩放。

### 3. 发送规则

代码里有：

- `RC_PID_*`
- `s_rules`
- `interval_ms`

说明它不是盲目全量推送，而是按规则和频率发送。

## 你可以怎么理解

它更像一个“小型数据发布服务”，上游来自本地传感器缓存，下游是手机 App。
