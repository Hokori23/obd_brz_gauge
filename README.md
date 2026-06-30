# OBD BRZ Gauge

这是一个运行在 `ESP32-S3` 上的车载圆形仪表项目。它通过 `BLE -> ELM327 -> OBD` 读取车辆数据，再用 `LVGL` 渲染触摸界面。

如果你是 Web 全栈开发，可以先这样映射：

- `app_main.c` 类似应用启动入口
- `app_obd_dsp` 类似业务层 / domain 层
- `bsp_obd_dsp` 类似基础设施层 / 硬件适配层
- `export_path` 类似设计工具导出的 UI 代码
- `NVS` 类似设备本地 KV 存储
- `FreeRTOS task` 类似常驻后台 worker

演示视频：
`https://www.douyin.com/video/7614174567678984187`

## 项目状态

- 硬件平台：`Waveshare ESP32-S3-Touch-LCD-1.85`
- 软件栈：`ESP-IDF 5.1+`、`LVGL 8`
- 通信链路：`BLE + ELM327`
- 当前验证重点：`Subaru BRZ ZC6`

## 系统在做什么

1. 初始化板级外设：I2C、IO 扩展、LCD、背光、触摸。
2. 初始化 `LVGL`，启动 UI 刷新任务。
3. 从 `NVS` 读取用户配置和里程统计。
4. 连接蓝牙 OBD 设备，轮询 OBD PID。
5. 把解析后的车况数据写入共享缓存。
6. UI 页面从共享缓存读取数据并显示。
7. 额外采集刹车温度、机油压力，并通过 BLE 服务对外广播。

## 学习顺序

1. [main/README.md](main/README.md)
2. [main/app_main.c](main/app_main.c)
3. [main/app_obd_dsp/README.md](main/app_obd_dsp/README.md)
4. [main/bsp_obd_dsp/README.md](main/bsp_obd_dsp/README.md)
5. [main/export_path/README.md](main/export_path/README.md)

## 目录树

```text
.
|-- README.md
|-- docs/                     项目文档
|-- firmware/                 预编译固件
|-- tools/                    辅助脚本
`-- main/                     主组件
    |-- app_main.c            入口
    |-- README.md             主流程说明
    |-- app_obd_dsp/          业务层
    |-- bsp_obd_dsp/          板级与驱动层
    `-- export_path/          UI 导出代码
```

## 编译烧录

```bash
idf.py set-target esp32s3
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175" build
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175" -p PORT flash monitor
```

## 学习时重点关注

- `app_main.c`：系统启动顺序
- `elm327_ble_client.c`：OBD 通信状态机
- `obd_data_cache.c`：共享数据与平滑处理
- `nvs_storage.c`：配置和里程持久化
- `vehicle_profiles.c`：车型差异抽象

## 相关文档

- [docs/README.zh-CN.md](docs/README.zh-CN.md)
- [docs/README.en.md](docs/README.en.md)
- [docs/PROJECT_STRUCTURE.md](docs/PROJECT_STRUCTURE.md)
