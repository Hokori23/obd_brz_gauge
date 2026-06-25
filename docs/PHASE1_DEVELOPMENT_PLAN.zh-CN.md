# 第一阶段：开发验证与学习计划

## 目标

这一阶段的目标不是直接做可量产产品，而是基于你手头已有的开发板、屏幕和 OBD 设备，把项目完整跑通，并借这个过程建立最基本的单片机、ESP-IDF、LVGL、BLE 和 OBD 调试能力。

你最终应该做到：

- 能编译、烧录并运行本项目
- 能点亮屏幕并操作 UI
- 能通过 BLE 扫描并连接 OBD 设备
- 能读取并显示基础车辆数据
- 能理解项目的主流程、模块边界和常见排障方法

## 适用硬件

建议优先沿用仓库当前适配的硬件组合：

- `ESP32-S3` 开发板
- SPI 或 QSPI 屏幕
- 触摸屏控制器
- 兼容 `ELM327` 的 BLE OBD 设备
- USB 供电与串口连接线

说明：

- 本仓库当前目标板是 `Waveshare ESP32-S3-Touch-LCD-1.85`
- 如果你手头的 `ESP32-S3` 开发板和屏幕不是同一套硬件，需要你自己对照驱动和引脚定义做适配
- 如果硬件差异较大，建议先跑通最小功能，再考虑完全复用现有 UI

## 学习重点

这一阶段建议只学够用的知识，不追求系统化覆盖全部嵌入式内容。

建议优先掌握：

- `ESP-IDF` 基本开发流程：`idf.py build`、`flash`、`monitor`
- GPIO、I2C、UART 的基本作用
- `LVGL` 页面刷新与输入事件
- BLE 扫描、连接、通知的基本概念
- `ELM327` 和 OBD 基础命令路径
- 基本供电与接线安全

## 实施步骤

### 第 1 步：搭建环境

先准备好基础软件环境：

- 安装 `ESP-IDF 5.1+`
- 安装 Python 与依赖工具链
- 确认 `idf.py` 可执行
- 确认开发板能被电脑识别

建议先执行：

```bash
idf.py set-target esp32s3
idf.py build
```

如果编译不通过，先不要接硬件，先解决环境问题。

### 第 2 步：只跑屏幕和 UI

先不接车，不接 OBD，目标是确认显示链路通了：

- 屏幕能点亮
- 触摸可用
- 页面能初始化
- 页面切换正常

这一阶段主要涉及：

- [main/app_main.c](../main/app_main.c)
- [main/export_path](../main/export_path)
- [main/bsp_obd_dsp/lcd_driver](../main/bsp_obd_dsp/lcd_driver)
- [main/bsp_obd_dsp/touch_driver](../main/bsp_obd_dsp/touch_driver)

如果你用的不是仓库当前对应的屏幕和触摸芯片，这一步通常需要改引脚、初始化时序或驱动。

### 第 3 步：跑通 BLE 扫描和连接

在 UI 和显示稳定后，再开始验证 BLE OBD。

目标：

- 能扫描到你的 OBD 设备
- 能点击连接
- 日志中能看到 `ELM327` 初始化命令
- 设备连接状态能被正确识别

主要代码：

- [main/bsp_obd_dsp/elm327_ble_client.c](../main/bsp_obd_dsp/elm327_ble_client.c)
- [main/export_path/screens/ui_ScreenPageBLEScan.c](../main/export_path/screens/ui_ScreenPageBLEScan.c)

建议优先验证最常见名称如 `OBDII` 的设备，避免一开始就改很多 BLE 过滤逻辑。

### 第 4 步：跑通基础车辆数据

连接成功后，优先验证标准 PID，可先不追求所有数据都正确。

建议先确认这些值能稳定更新：

- RPM
- Speed
- Coolant Temp
- Intake Temp
- Battery Voltage

如果这些都通了，再继续看：

- Load
- TPS
- Oil Temp

### 第 5 步：针对 BRZ ZC6 做专项验证

这个仓库当前对 `BRZ ZC6` 是已有支持的，但你仍需要用自己的车和 OBD 盒子做实测。

重点验证：

- 自动协议检测是否成功
- 固定协议是否更稳定
- 油温查询是否稳定
- 档位识别是否准确
- UI 是否存在长时间运行卡顿或断连问题

相关代码：

- [main/app_obd_dsp/vehicle_profiles.c](../main/app_obd_dsp/vehicle_profiles.c)
- [main/app_obd_dsp/obd_data_cache.c](../main/app_obd_dsp/obd_data_cache.c)
- [main/bsp_obd_dsp/elm327_ble_client.c](../main/bsp_obd_dsp/elm327_ble_client.c)

### 第 6 步：最后再接扩展传感器

扩展传感器不是第一优先级。建议在基础 OBD 功能稳定后再接：

- `RS485` 刹车温度
- `ADS1115` 油压

对应代码：

- [main/bsp_obd_dsp/rs485_brake_temp.c](../main/bsp_obd_dsp/rs485_brake_temp.c)
- [main/bsp_obd_dsp/ads1115_oil_pressure.c](../main/bsp_obd_dsp/ads1115_oil_pressure.c)

## 第一阶段的产出

这一阶段做完后，建议至少沉淀这些结果：

- 一份你自己硬件的实际接线记录
- 一份开发环境安装记录
- 一份烧录和调试命令清单
- 一份 BRZ ZC6 的实测结果表
- 一份已知问题列表

## 风险与边界

这一阶段的方案只适合开发验证，不适合直接装车量产。

主要原因：

- 开发板体积大，接线多
- 抗震、抗温差和供电稳定性不足
- 没有完整车载电源保护
- 结构装配不适合长期使用

因此，第一阶段的成功标准应定义为“功能跑通、会调试、知道下一步要改什么”，而不是“已经是成品”。
