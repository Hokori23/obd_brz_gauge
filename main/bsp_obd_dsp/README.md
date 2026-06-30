# `bsp_obd_dsp`

这是板级支持包和驱动层。`BSP` 可以理解成“把通用 ESP-IDF 能力适配到这块板子和这套外设上”。

如果类比 Web，这里更像 infrastructure 层：

- BLE 客户端
- 本地存储
- 传感器采集
- 屏幕与触摸驱动
- 总线与 IO 扩展

## 子模块

```text
bsp_obd_dsp/
|-- nvs_storage.*            本地持久化
|-- elm327_ble_client.*      OBD 蓝牙客户端
|-- racechrono_ble_diy.*     对外 BLE GATT 服务
|-- rs485_brake_temp.*       RS485 刹车温度采集
|-- ads1115_oil_pressure.*   机油压力采集
|-- bsp_board.h              板级常量
|-- i2c_driver/              I2C 总线
|-- exio/                    TCA9554 IO 扩展
|-- lcd_driver/              屏幕驱动
`-- touch_driver/            触摸驱动
```

## 1. `nvs_storage`

`NVS` 是 ESP32 的非易失键值存储，可以类比设备上的 `localStorage + EEPROM`。

它保存两类数据：

- 用户配置：协议、主题、蓝牙设备名、默认页面、亮度、车型、告警阈值、页面布局
- 运行统计：总里程、小计里程、运行时间、最高时速、平均时速

关键点：

- 启动时加载 blob，不存在就写默认值。
- 配置修改立即写入。
- 统计数据不是每次都写，而是定时落盘，减少 flash 擦写。

## 2. `elm327_ble_client`

这是最关键的驱动模块，负责和 OBD 适配器通信。

### 它解决的问题

1. 扫描并连接 ELM327 BLE 设备。
2. 发现服务和特征值。
3. 发送 ASCII OBD 指令，例如 `01 0C`。
4. 聚合并解析 ELM327 返回的文本响应。
5. 周期轮询多个 PID。
6. 把解析结果通过回调写入 `obd_data_cache`。

### 你可以把它理解成

- BLE GATT client
- OBD 协议适配层
- 轮询调度器
- 文本协议解析器

### 重点能力

- 自动/手动协议选择
- 多车型机油温查询策略
- `Mode 01` / `Mode 21` / `Mode 22` 解析
- 扫描模式与连接模式切换

### 先读哪些函数

- `elm327_ble_start_default()`
- `obd_poll_task()`
- `elm327_auto_detect_protocol()`
- GATT notify 处理分支

## 3. `racechrono_ble_diy`

这是一个 BLE GATT Server，不是客户端。它把本机采集到的传感器数据向手机侧应用广播。

理解重点：

- 同一个芯片既能作为 BLE client 连 OBD，也能作为 BLE server 给手机提供服务。
- 这相当于“上游拉数据，下游再发布数据”。

## 4. `rs485_brake_temp`

通过 `RS485 / Modbus RTU` 读取刹车温度。

对 Web 开发者来说，可以把它理解成另一路串口传感器服务。它和 OBD 不共享协议，只是最后都写入同一个数据缓存。

## 5. `ads1115_oil_pressure`

名字里有 `ADS1115`，但当前头文件注释已经说明实现偏向 `ESP32 ADC 直连` 方案，保留旧命名主要是兼容。

它负责：

- 周期读取机油压力传感器模拟量
- 换算成 `0.1 bar`
- 写入 `obd_data_cache`

## 6. `bsp_board.h`

集中放板级宏定义，比如背光引脚。这是硬件常量入口。

## 继续往下读

- [i2c_driver/README.md](i2c_driver/README.md)
- [exio/README.md](exio/README.md)
- [lcd_driver/README.md](lcd_driver/README.md)
- [touch_driver/README.md](touch_driver/README.md)

## 核心实现就近说明

- [nvs_storage.md](nvs_storage.md)
- [elm327_ble_client.md](elm327_ble_client.md)
- [rs485_brake_temp.md](rs485_brake_temp.md)
- [ads1115_oil_pressure.md](ads1115_oil_pressure.md)
- [racechrono_ble_diy.md](racechrono_ble_diy.md)
- [bsp_board.md](bsp_board.md)
