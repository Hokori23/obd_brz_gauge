# `bsp_obd_dsp` 模块说明

这一层是基础设施和板级适配层。
它负责把 ESP-IDF、板卡外设和各类协议设备封装成上层能直接使用的模块。

## 当前核心模块

- `nvs_storage.*`：配置与统计持久化
- `elm327_ble_client.*`：OBD BLE 客户端
- `racechrono_ble_diy.*`：对外 BLE GATT 服务
- `rs485_brake_temp.*`：RS485 刹车温度采集
- `ads1115_oil_pressure.*`：机油压力采样
- `qmi8658_gforce.*`：板载 IMU G-force 采样
- `boards/`：板型分发、显示和触摸适配

## `nvs_storage`

这个模块负责持久化用户配置和运行统计。
它不仅保存设置项，也负责控制脏数据何时真正落盘，避免频繁擦写 Flash。

## `elm327_ble_client`

这是最关键的设备接入模块，负责：

1. 扫描并连接 ELM327 类 BLE 设备。
2. 发现服务和特征值。
3. 发送 ASCII OBD 指令。
4. 解析 `Mode 01`、`Mode 21`、`Mode 22` 等返回。
5. 周期轮询所需 PID，并把结果写入共享缓存。

如果要理解整条 OBD 数据链路，这个模块必须重点看。

## `racechrono_ble_diy`

这是对外暴露的 BLE GATT Server。
它会把本机已经采集到的数据重新组织后广播给手机或外部应用。

## `boards`

这里收敛不同板型的初始化差异，包括：

- 显示控制器
- 背光
- 触摸
- 共享 I2C 总线
- 板型名称和分辨率信息

如果项目要移植到新板卡，这里通常是第一落点。

## 建议阅读顺序

1. [nvs_storage.md](nvs_storage.md)
2. [elm327_ble_client.md](elm327_ble_client.md)
3. [racechrono_ble_diy.md](racechrono_ble_diy.md)
4. [boards/README.md](boards/README.md)
5. [rs485_brake_temp.md](rs485_brake_temp.md)
6. [ads1115_oil_pressure.md](ads1115_oil_pressure.md)

## 相关说明

- [bsp_board.md](bsp_board.md)
- [nvs_storage.md](nvs_storage.md)
- [elm327_ble_client.md](elm327_ble_client.md)
- [racechrono_ble_diy.md](racechrono_ble_diy.md)
- [rs485_brake_temp.md](rs485_brake_temp.md)
- [ads1115_oil_pressure.md](ads1115_oil_pressure.md)
