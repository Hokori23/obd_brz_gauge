# `tools`

这里放项目辅助脚本，不参与主运行链路。

## 当前脚本

- `png_to_lvgl.py`
- `backup_factory_firmware.ps1`
- `run_ui_platform_static_tests.ps1`

## 作用

- 把图片资源转换成适合 `LVGL` 使用的 C 代码或资源格式
- 在烧录前备份 `ESP32-S3` 闪存整片镜像，支持分块读取
- 运行当前仓库的静态编译型兼容性测试

## 常用命令

- `powershell -ExecutionPolicy Bypass -File .\tools\run_ui_platform_static_tests.ps1`
- `powershell -ExecutionPolicy Bypass -File .\tools\backup_factory_firmware.ps1 -Port COM12`
- `powershell -ExecutionPolicy Bypass -File .\tools\backup_factory_firmware.ps1 -Port COM12 -Mode chunked -ChunkSizeBytes 0x10000`
- `powershell -ExecutionPolicy Bypass -File .\tools\backup_factory_firmware.ps1 -Port COM12 -Mode chunked -ChunkSizeBytes 0x1000 -ResumeBaseName factory-backup-esp32s3-COM12-YYYYMMDD-HHMMSS`
- `powershell -ExecutionPolicy Bypass -File .\tools\check_ws175_debug.ps1 -Port COM12`
- `powershell -ExecutionPolicy Bypass -File .\tools\check_ws175_debug.ps1 -Port COM12 -SmokeTestJtag`
- `powershell -ExecutionPolicy Bypass -File .\tools\clean_default_build.ps1`
- `powershell -ExecutionPolicy Bypass -File .\tools\flash_ws175.ps1 -DiagnoseOnly -Port COM12`
- `powershell -ExecutionPolicy Bypass -File .\tools\flash_ws175.ps1 -ProbeOnly -Port COM12`
- `powershell -ExecutionPolicy Bypass -File .\tools\flash_ws175.ps1 -ProbeOnly -Port COM12 -ProbeTimeoutSec 45`
- `powershell -ExecutionPolicy Bypass -File .\tools\flash_ws175.ps1 -Port COM12 -NoMonitor`

## WS175 调试与备用链路

- `check_ws175_debug.ps1` 会同时报告：
  - 原厂备份是否存在
  - `COM12` 是否只是枚举成功，还是实际可打开
  - `USB JTAG/serial debug unit` 与 `OpenOCD` 状态
  - 本机 `dfu-util` 是否存在
- 对当前这块 `ESP32-S3` 板子，`DFU` 不是默认优先方案。
  - 按 Espressif 官方说明，若走内部 `USB Serial/JTAG` 路径，切到 `USB OTG DFU` 可能需要永久性的 `USB_PHY_SEL` eFuse 或外部 USB PHY。
  - 在没有确认硬件前，不要把 `DFU` 当成无风险替代刷机通道。

## WS175 备份与恢复

- 当前主备份目录：`backups/factory-backup-esp32s3-COM12-20260630-101134.parts`
- 只有在生成完整合并镜像后，才允许烧录自定义固件
- 完整镜像校验条件：
  - 生成 `.bin`
  - 文件大小精确等于 `16777216` 字节
  - 对应 `.log` 存在

恢复原厂固件模板命令：

- `python -m esptool --chip esp32s3 --port COM12 --baud 115200 write_flash 0x0 <factory-backup.bin>`

WS175 当前自定义固件烧录命令：

- `idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175" build`
- `idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175" -p COM12 flash monitor`

## 就近说明

- [png_to_lvgl.md](png_to_lvgl.md)
