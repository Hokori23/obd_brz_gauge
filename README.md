# OBD BRZ Gauge

这是一个运行在 `ESP32-S3` 圆屏设备上的车载仪表项目。它通过 `BLE -> ELM327 -> OBD` 读取车辆数据，再用 `LVGL` 渲染首页、设置页和各类仪表页。

## 当前重点

- 目标硬件：`Waveshare ESP32-S3-Touch-LCD-1.75 / 1.85` 圆屏方案
- 当前主车型：`Subaru BRZ ZC6`
- 首页架构：`MENU + N 个仪表页 + ADD`
- 首页刷新策略：只对当前激活页做实时刷新；翻页后立即刷新新页
- 设置页架构：两级结构
  - 一级分类：`DISPLAY` / `DASHBOARD` / `VEHICLE`
  - 二级详情：具体 roller / slider 配置

## 目录速览

```text
.
|-- README.md
|-- docs/
|-- firmware/
|-- tools/
`-- main/
    |-- app_main.c
    |-- README.md
    |-- app_obd_dsp/
    |-- bsp_obd_dsp/
    `-- export_path/
```

## 核心模块

- `main/app_main.c`
  系统启动入口，负责板级初始化、配置加载、UI 启动。
- `main/app_obd_dsp/`
  业务层，包括 OBD 数据缓存、轮询调度、车型能力和辅助传感器需求控制。
- `main/bsp_obd_dsp/`
  板级与外设适配，包括 BLE、NVS、显示和各类驱动。
- `main/export_path/`
  UI 相关代码，当前动态首页运行时、设置页、BLE 扫描页、仪表编辑页都在这里。

## 当前 UI 约定

- 首页横向滑动：`MENU -> gauge pages -> ADD`
- 首页上滑：进入 `BLE Scan`
- 首页下滑：进入 `Settings`
- `OBD Protocol` 与 `BLE Scan`、`Settings` 保持首页平级入口，不并入 `Settings`
- 仪表页长按：进入编辑态
- 编辑态 `EDIT`：进入仪表配置页
- `No page config` 保留为异常兜底页，但必须可返回，不允许出现死路

## 构建

```bash
idf.py set-target esp32s3
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175" build
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175" -p PORT flash monitor
```

## 推荐阅读顺序

1. [main/README.md](main/README.md)
2. [main/app_main.c](main/app_main.c)
3. [main/app_obd_dsp/README.md](main/app_obd_dsp/README.md)
4. [main/bsp_obd_dsp/README.md](main/bsp_obd_dsp/README.md)
5. [main/export_path/README.md](main/export_path/README.md)
6. [docs/DYNAMIC_DASHBOARD_HOME_UI_PLAN.zh-CN.md](docs/DYNAMIC_DASHBOARD_HOME_UI_PLAN.zh-CN.md)

## 相关文档

- [docs/README.zh-CN.md](docs/README.zh-CN.md)
- [docs/README.en.md](docs/README.en.md)
- [docs/PROJECT_STRUCTURE.md](docs/PROJECT_STRUCTURE.md)
- [docs/WS175_DASHBOARD_POLISH_2026-07-02.md](docs/WS175_DASHBOARD_POLISH_2026-07-02.md)
