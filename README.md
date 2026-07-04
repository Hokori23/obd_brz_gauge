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

- 仓库约定的 ESP-IDF 版本是 `v5.5.4`
- 个人机器的实际安装路径不应写死进仓库；推荐通过 VS Code ESP-IDF 插件、`IDF_PATH` 环境变量，或 `tools/idf-env.local.ps1` 本地覆盖文件来指定
- 如果你直接在普通 `powershell` 里运行而不是在 ESP-IDF 插件终端里运行，请先确保当前 shell 已拿到正确的 `IDF_PATH` / Python 环境
- `tools/flash_ws175.ps1` 现在会优先读取：
  - `WS175_IDF_PATH`
  - `IDF_PATH`
  - `WS175_IDF_PYTHON_SCRIPTS`
  - `IDF_PYTHON_ENV_PATH`
- 本地可复制 `tools/idf-env.example.ps1` 为 `tools/idf-env.local.ps1`，填入你自己的路径；这个文件不需要提交

### 推荐使用方式

1. `VS Code ESP-IDF 插件终端`
   适合日常开发。插件会自动准备好 `IDF_PATH`、Python 环境和工具链，最不容易出现“`idf.py build` 不存在或环境不对”的误判。
2. `普通 PowerShell + 本机环境变量`
   适合你手动跑命令或给外部脚本复用。至少保证当前 shell 能看到正确的 `IDF_PATH`，必要时再补 `IDF_PYTHON_ENV_PATH`。
3. `tools/idf-env.local.ps1`
   适合个人机器有固定路径，但又不想把绝对路径写进仓库。`flash_ws175.ps1` 会自动读取这个本地覆盖文件。

### Agent / 终端常见差异

- `VS Code` 里能构建，不代表新开的 `powershell` 也能直接构建
- `agent` 如果不是从 ESP-IDF 插件终端启动，经常拿不到同一套环境变量
- 所以出现 `idf.py build` 失败时，先区分：
  - 是仓库代码编译失败
  - 还是当前 shell 没有进入 ESP-IDF 环境

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

## Update 2026-07-03

Current repo direction to remember:

- `Settings` now owns the `OBD` settings path as part of one unified IA.
- The old standalone OBD protocol screen is legacy debt and should not be treated as the target UX.
- Home horizontal page-switch performance has been instrumented with `ui_home_perf`; current evidence shows the bottleneck is the existing tile/scroll implementation path, not page-specific content.

## Update 2026-07-04

Current runtime changes to remember:

- Dashboard metric set now distinguishes `OIL-PID` and `OIL-CAN` as separate items.
- `MAP` and `IGN` are now first-class metrics sourced from standard OBD PIDs `01 0B` and `01 0E`.
- ZC6 now has two gear paths:
  - `GEAR-DERIVED`: computed from RPM and vehicle speed using profile gear-ratio ranges with hysteresis.
  - `GEAR-MON`: monitor-mode path for ZC6 CAN `0x141`.
- ZC6 `OIL-CAN` is implemented as a low-frequency ELM327 monitor-mode path for CAN `0x360`.
- ELM327 polling now includes stronger self-heal / reconnect behavior and weighted polling for `LOD`, `TPS`, and `Speed`.
- Debug toggles have been consolidated in `main/export_path/ui_debug_config.h`.
- Boot can print the persisted last-20-entry error log when `CONFIG_OBD_BOOT_PRINT_ERROR_LOG=y`.
