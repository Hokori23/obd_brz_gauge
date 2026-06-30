# `main` 模块总览

`main` 是这个 ESP-IDF 工程的主组件。你可以把它理解成一个“单体应用”，里面同时放了：

- 启动入口
- 业务状态
- 硬件驱动适配
- UI 导出代码

## 目录职责

```text
main/
|-- app_main.c         启动入口，负责装配所有模块
|-- app_obd_dsp/       业务层：共享数据、车型配置、里程统计
|-- bsp_obd_dsp/       基础设施层：BLE、NVS、LCD、触摸、传感器驱动
`-- export_path/       UI 代码和资源
```

## 启动链路

`app_main()` 的主流程接近后端服务的 bootstrap：

1. `nvs_storage_init()`：初始化本地持久化。
2. `vehicle_profile_set_active()`：加载当前车型。
3. `I2C_Init()` / `EXIO_Init()`：初始化板级总线和 IO 扩展。
4. `LCD_Init()`：初始化屏幕、背光、触摸。
5. `lv_init()`：初始化 LVGL。
6. 创建 `lvgl_port_task`：持续调用 `lv_timer_handler()`。
7. `ui_init()`：构建页面树。
8. `elm327_ble_start_default()`：启动 BLE OBD 客户端。
9. `racechrono_ble_diy_start()`：启动对手机广播的 BLE 服务。
10. `rs485_brake_temp_start()` / `oil_pressure_start()`：启动附加传感器任务。
11. `vMileageDataStatisticTask()`：启动里程统计定时器。

## 线程模型

这里不是单线程事件循环，而是多个常驻任务并行：

- `LVGL task`：负责 UI 刷新
- `OBD poll task`：负责按周期发 OBD 命令
- `NVS flush task`：负责周期性落盘统计数据
- `RS485 task`：负责刹车温度采集
- `Oil pressure task`：负责机油压力采集

共享状态主要通过全局变量 + 锁保护，而不是消息总线。

## 建议阅读顺序

1. [app_main.c](app_main.c)
2. [app_main.md](app_main.md)
3. [app_obd_dsp/README.md](app_obd_dsp/README.md)
4. [bsp_obd_dsp/README.md](bsp_obd_dsp/README.md)
5. [export_path/README.md](export_path/README.md)

## 你需要适应的嵌入式差异

- 生命周期是“上电后长期运行”，不是单次请求。
- 很多模块启动后不会退出。
- `extern` 全局变量和回调函数比 Web 项目更常见。
- UI 和驱动通常强耦合于硬件时序。
