# `main` 模块总览

`main` 是这个 ESP-IDF 工程的主组件，可以把它理解成完整应用本体。
这里同时放了启动装配、业务状态、板级适配、UI 运行时和导出的界面代码。

## 目录职责

```text
main/
|-- app_main.c         启动入口，负责串起所有子系统
|-- app_bootstrap.*    启动装配逻辑，收敛初始化顺序
|-- app_lvgl_port.*    LVGL 运行时、锁、刷屏与性能采样
|-- app_obd_dsp/       业务层：共享数据、车型规则、按需采集状态
|-- bsp_obd_dsp/       基础设施层：BLE、NVS、板级、传感器驱动
`-- export_path/       UI 页面、布局和运行时交互代码
```

## 启动链路

启动主线可以按下面顺序理解：

1. `app_main()` 进入后先做最外层启动编排。
2. `app_bootstrap` 初始化 NVS、车型配置、板级资源和显示环境。
3. `app_lvgl_port` 接管 LVGL 的任务循环、锁和刷屏回调。
4. `ui_init()` 或页面初始化逻辑创建首页、设置页、扫描页等 UI。
5. `elm327_ble_client` 启动默认 BLE OBD 连接流程。
6. `racechrono_ble_diy` 启动对手机暴露的 BLE GATT 服务。
7. `rs485_brake_temp`、`ads1115_oil_pressure`、`qmi8658_gforce` 等附加采集任务按需进入后台循环。

## 线程模型

这里不是单线程事件循环，而是多个常驻任务并行：

- `LVGL task`：负责 UI 刷新与动画推进
- `OBD poll task`：负责 OBD 协议初始化和 PID 轮询
- `NVS flush task`：负责脏数据落盘
- `RS485 task`：负责刹车温度查询
- `Oil pressure task`：负责机油压力采样
- `IMU task`：负责 ESP32 板载 G-force 采集

模块之间主要通过共享缓存、配置快照和少量锁同步协作，而不是消息总线架构。

## 建议阅读顺序

1. [app_main.c](app_main.c)
2. [app_bootstrap.c](app_bootstrap.c)
3. [app_lvgl_port.c](app_lvgl_port.c)
4. [app_obd_dsp/README.md](app_obd_dsp/README.md)
5. [bsp_obd_dsp/README.md](bsp_obd_dsp/README.md)
6. [export_path/README.md](export_path/README.md)

## 维护时要注意

- 很多模块启动后不会退出，代码风格更偏“长期运行的设备程序”。
- UI、驱动和协议轮询之间会共享状态，修改顺序时要特别注意初始化时机。
- 板级差异不只影响引脚，也影响显示旋转、触摸映射和可用外设。
- 如果只看 `main()` 会觉得逻辑很长，实际职责已经开始向 `app_bootstrap`、`app_lvgl_port` 和各业务模块下沉。
