# Waveshare ESP32-S3-Touch-AMOLED-1.75 适配方案

## 1. 文档目的

本文档用于同步当前仓库对 `Waveshare ESP32-S3-Touch-AMOLED-1.75` 的适配状态，并明确：

- 哪些能力已经落地
- 哪些结论已经被代码、测试或构建验证
- 哪些部分仍然必须依赖真实硬件验收
- 后续开发应继续沿着什么分层方式推进

这份文档描述的是“当前仓库真实状态”，不是纯方案草稿。

## 2. 当前结论

截至 `2026-06-29`，仓库已经具备可持续演进的多板型结构，`WS175 AMOLED` 也已经接入到实际代码路径中。

当前可以确认：

- 仓库已经不是“是否要支持 1.75”的讨论阶段，而是“已经开始支持并持续补齐”的阶段
- 板级兼容层已经落地，`1.85` 和 `1.75` 走统一 `board API`
- UI 兼容层已经落地，分辨率差异开始通过 `ui_platform + ui_layout` 处理
- “兼容层”当前应明确分成两部分：`board` 硬件兼容层 + `ui_platform/ui_layout` UI 兼容层
- 默认构建通过
- `WS175` 专用构建通过
- 静态测试通过
- 真实 `1.75` 板卡的点亮、触摸方向、亮度体感、UI 观感仍未完成最终实板验收

因此，当前最准确的表述是：

- 已支持 `ESP32-S3-Touch-AMOLED-1.75` 的代码接入、编译与专用构建
- 尚未完成真实硬件闭环验证，不能宣称“已完全兼容”

## 3. 已确认的 1.75 硬件事实

基于 Waveshare 官方资料以及当前仓库已接入的兼容组件，`ESP32-S3-Touch-AMOLED-1.75` 的关键规格如下：

- 显示控制器：`CO5300`
- 分辨率：`466x466`
- 触摸控制器：`CST9217`
- I2C：
  - `SCL = GPIO14`
  - `SDA = GPIO15`
- LCD QSPI：
  - `CS = GPIO12`
  - `PCLK = GPIO38`
  - `DATA0 = GPIO4`
  - `DATA1 = GPIO5`
  - `DATA2 = GPIO6`
  - `DATA3 = GPIO7`
  - `LCD_RST = GPIO39`
- Touch：
  - `TOUCH_RST = GPIO40`
  - `TOUCH_INT = GPIO11`
- 当前采用的触摸坐标变换默认值：
  - `swap_xy = 0`
  - `mirror_x = 1`
  - `mirror_y = 1`

这些常量已经集中在：

- `main/bsp_obd_dsp/boards/board_ws_175_amoled_spec.h`

## 4. 依赖策略

当前没有直接切换到 Waveshare 的完整新 BSP，原因是：

- 上游较新的 `1.75` BSP 依赖 `ESP-IDF >= 5.5`
- 本仓库当前仍以 `ESP-IDF 5.4` 为基线

所以当前采用的是“沿官方器件路线，但不强绑新 BSP”的方案：

- 复用官方硬件事实和初始化顺序
- 在本仓库内维护 `WS175` 的板级 glue code
- 使用与 `ESP-IDF 5.4` 兼容的组件

当前实际依赖：

- `espressif/esp_lcd_co5300`
- `espressif/esp_lcd_panel_io_additions`
- `waveshare/esp_lcd_touch_cst9217`

这个选择的好处：

- 不需要为了单板适配把整个项目立刻升级到 `ESP-IDF 5.5+`
- 仍然沿着官方支持路径前进，而不是自写整套私有驱动
- 后续如果项目整体升级 IDF，切回更完整 BSP 的成本更低

## 5. 适配分层

当前推荐并已落地的分层如下：

```text
app / business logic
    |
    v
board API + board_profile
    |
    +-- board_ws_185.c
    +-- board_ws_175_amoled.c
    |
    v
display / touch component drivers
    |
    v
ui_platform + ui_layout
    |
    v
UI pages / generated SquareLine code
```

职责边界：

- `board` 层负责
  - 板级初始化
  - LCD 初始化
  - touch 初始化
  - 亮度控制
  - 对上层暴露板级显示能力、`board_profile` 与硬件事实
- `ui_platform` 层负责
  - 屏幕宽高
  - 中心点
  - 圆屏半径
  - 安全边距
  - 基于 `360x360` 的统一缩放换算
- `ui_layout` 层负责
  - 页面级布局参数
  - 页面控件尺寸、偏移、留白
  - 把页面中的硬编码坐标从 `ui_ScreenPage*.c` 中抽出来

这里需要明确：

- 兼容层不只是硬件兼容，也包括 UI 兼容
- 但 UI 兼容不应该塞进 LCD/Touch 驱动里
- UI 兼容应该通过 `ui_platform + ui_layout` 消费板级分辨率与圆屏几何信息

## 6. 已落地能力

### 6.1 多板型 board API

当前统一入口已经形成：

- `main/bsp_obd_dsp/boards/board_api.h`
- `main/bsp_obd_dsp/boards/board_dispatch.c`

板级实现包括：

- `main/bsp_obd_dsp/boards/board_ws_185.c`
- `main/bsp_obd_dsp/boards/board_ws_175_amoled.c`

这意味着应用层不再直接依赖具体 LCD / Touch 芯片型号。

### 6.2 WS175 板级实现已参与真实构建

`main/bsp_obd_dsp/boards/board_ws_175_amoled.c` 当前已包含并参与构建：

- I2C master 初始化
- CO5300 QSPI 面板初始化
- 面板初始化命令序列
- CST9217 touch 初始化
- touch transform 配置
- 亮度命令 `0x51` 写入路径
- LVGL flush ready callback 对接

它不是占位 stub，而是已经在专用构建链路中被编译和链接。

### 6.3 板级 profile / 自描述能力已落地

当前已经存在板级自描述接口：

- `board_profile_t`
- `const board_profile_t *board_profile(void);`

位置：

- `main/bsp_obd_dsp/boards/board_api.h`
- `main/bsp_obd_dsp/boards/board_dispatch.c`

`app_main.c` 当前启动时会打印：

- board name
- 分辨率
- color depth
- draw buffer lines
- 是否有 touch
- touch transform 标志

这项能力已经从“建议”变成“已实现”，是后续实板 bring-up 的核心观察面。

### 6.4 板级显示参数已下沉

`board_display_context_t` 当前已承担关键显示参数的输出职责，包括：

- `hor_res`
- `ver_res`
- `draw_buffer_lines`
- `color_bits`
- `has_touch`

`app_main.c` 已基于板级参数初始化：

- `ui_platform_init(board_ctx.hor_res, board_ctx.ver_res)`
- LVGL draw buffer 分配
- LVGL 显示驱动尺寸
- touch 注册

这说明不同板型已经不再共享一份写死的显示假设。

### 6.5 UI 平台与布局兼容层已落地

当前仓库已经具备：

- `main/export_path/ui_platform.h`
- `main/export_path/ui_platform.c`
- `main/export_path/ui_layout.h`

当前这层已经不是“只有一个缩放函数”的状态，而是已经具备两种职责：

- `ui_platform`
  - 管理运行时屏幕参数
  - 提供 `ui_platform_init()`、`ui_screen_width()`、`ui_screen_height()`、`ui_layout_px()` 等基础能力
- `ui_layout`
  - 管理页面级布局结构体
  - 把原先散落在各页面里的尺寸、偏移、圆屏半径、留白等参数集中输出

当前已经沉淀出的公共布局基元包括：

- `ui_round_shell_layout_t`
- `ui_round_shell_layout_get()`

这意味着后续如果切换到别的圆屏板型，优先改的是：

- `board` 层的硬件事实
- `ui_platform` 的屏幕输入
- 必要时补充/调整 `ui_layout` 页面参数

而不是把分辨率差异重新散回各个 `ui_ScreenPage*.c`。

当前代码里仍然存在的布局 getter 包括：

- 主线仍在使用的：
  - `ui_round_shell_layout_get`
  - `ui_ble_scan_layout_get`
  - `ui_settings_layout_get`
  - `ui_logo_layout_get`
  - `ui_obd_protocol_layout_get`
- 仍保留在仓库里的历史/兼容布局 helper：
  - `ui_brake_temp_layout_get`
  - `ui_oil_pressure_layout_get`
  - `ui_info_layout_get`
  - `ui_temp_layout_get`
  - `ui_needle_layout_get`
  - `ui_needle_config_layout_get`
  - `ui_warn_layout_get`
  - `ui_info_custom_layout_get`
  - `ui_temp_custom_layout_get`
  - `ui_easter_egg_layout_get`

这里要特别说明：

- 这些历史 getter 仍在 `ui_platform.c` / `ui_layout.h` 中保留，并不等于对应页面仍然是当前主线 UI 的一部分。
- 截至当前仓库状态，动态首页主线已经移除了旧固定页及其衍生页；这些 getter 更多体现为历史兼容资产，而不是当前页面流转入口。

### 6.6 已迁移到 `ui_layout` 的页面

当前主线仍在使用并已接入布局层的页面包括：

- `main/export_path/screens/ui_ScreenPageBLEScan.c`
- `main/export_path/screens/ui_ScreenPageSettings.c`
- `main/export_path/screens/ui_ScreenPageLogo.c`
- `main/export_path/screens/ui_ScreenPageODBProtocal.c`

另外，动态首页主实现 `main/export_path/ui_home_runtime.c` 也已经统一使用：

- `ui_round_screen_apply_base()`
- `ui_layout_px()`

它不再依赖旧固定页的逐页布局 getter，而是直接按动态 tile 结构组织首页内容。

说明：

- 仍然能看到部分 `360` 字样时，需要区分它是否只是角度值、注释或非分辨率语义
- 当前真正需要继续清理的，是仍然和分辨率/几何布局强绑定的硬编码坐标
- 历史旧页面相关的布局 getter 虽仍在代码中，但对应页面文件已不再位于当前主线路径

## 7. 当前验证状态

### 7.1 静态测试

当前测试覆盖包括：

- `tests/ui_platform/test_ui_platform.c`
- `tests/ui_layout/test_ui_layout.c`
- `tests/board_ws_175_amoled/test_board_ws_175_amoled_spec.c`
- `tests/board_display/test_board_display_profiles.c`

静态测试已经覆盖：

- UI 缩放规则
- 多页面布局缩放规则
- `WS175` 规格常量
- `WS185` / `WS175` 的显示 profile 约束
- `466x466` 下圆屏直径与针表配置页外围几何参数约束

执行命令：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_ui_platform_static_tests.ps1
```

最近已验证结果：

- `static tests passed`

### 7.2 构建验证

最近已验证通过：

- 默认构建：`idf.py build`
- `WS175` 专用构建：

```bash
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175" build
```

本地构建环境需要先激活 `ESP-IDF 5.4`，可复用以下方式：

```powershell
$env:PATH='C:\Users\Hokori\.espressif\python_env\idf5.4_py3.11_env\Scripts;' + $env:PATH
$idf_exports = python 'D:\esp\v5.4\esp-idf\tools\activate.py' --export
. $idf_exports
```

最近一次验证结果：

- 默认构建通过
- `WS175` 专用构建通过
- 生成固件体积约为 `0x15cb30`

这说明以下链路已经打通：

- 依赖接入
- 代码编译
- 板型分发
- `WS175` 默认配置集成

当前仍存在但未阻断构建的告警，后续可单独清理：

- `main/bsp_obd_dsp/rs485_brake_temp.c` 左移位宽告警
- `main/bsp_obd_dsp/lcd_driver/ST77916.c` 未使用函数告警
- `main/export_path/ui.c` 若干未使用变量告警
- `sdkconfig.defaults*` 中若干 unknown symbol 告警

### 7.3 尚未完成的验证

以下内容目前仍必须在真实 `ESP32-S3-Touch-AMOLED-1.75` 板上确认：

- AMOLED 是否正常点亮
- LVGL 刷新是否稳定
- touch 坐标方向与镜像是否完全正确
- `board_set_brightness()` 在实板上的表现是否符合预期
- 关键页面在 `466x466` 下是否仍有错位、遮挡、越界或比例失真
- BLE / OBD 主链路在 `WS175` 上是否存在时序副作用

在这些项目完成前，不能把当前状态描述成“1.75 已完全适配完成”。

## 8. 当前设计原则

### 8.1 板级差异留在 board 层

不要把以下内容再次扩散回页面层或业务层：

- GPIO 常量
- 面板型号判断
- touch 芯片差异
- 初始化顺序
- draw buffer 行数
- 亮度控制实现细节

### 8.2 UI 兼容独立于驱动兼容

不要在显示或触摸驱动里偷偷改页面坐标。

正确做法是：

- 驱动层输出硬件事实
- board 层输出板级 profile
- UI 层根据 profile 和屏幕指标做布局换算

### 8.3 以可验证的小步推进

建议保持以下优先级顺序：

1. 构建通过
2. 板级初始化成功
3. LVGL 正常刷新
4. touch 可用
5. 关键页面无明显错位或越界
6. 亮度与视觉细节再优化

## 9. 下一步执行计划

### 9.1 第一优先级：真实硬件 bring-up

必须做的动作：

- 烧录默认 `build` 固件
- 打开串口日志
- 核对启动日志中的 board profile
- 检查屏幕点亮与刷新
- 检查 touch 方向、点击区域与边界精度
- 检查亮度调节是否可用

推荐命令：

```bash
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175" -p <PORT> flash monitor
```

建议重点观察启动日志中这些信息是否正确：

- board name
- `466x466`
- `16-bit`
- `buffer_lines=20`
- `touch=yes`
- `swap_xy=0 mirror_x=1 mirror_y=1`

### 9.2 第二优先级：继续清理剩余 UI 几何硬编码

方向不是重写 UI，而是继续把仍与 `360x360` 基准强耦合的页面参数抽到 `ui_layout`。

建议流程：

1. 先用 `rg` 盘点页面中的剩余坐标硬编码
2. 先补布局 getter 和静态断言
3. 再迁移页面实现
4. 每迁移一页就保持默认构建和 `WS175` 构建可通过

建议直接复用以下排查命令：

```powershell
rg -n "ui_layout_px\\(" .\main\export_path\screens
rg -n "lv_obj_align\\([^\\n]*-?[0-9]+,\\s*-?[0-9]+\\)" .\main\export_path\screens
rg -n "lv_obj_set_x|lv_obj_set_y|lv_obj_set_width\\([^\\n]*[0-9]|lv_obj_set_height\\([^\\n]*[0-9]" .\main\export_path\screens
```

排查时要注意区分：

- 真正的分辨率/几何硬编码
- 合法的角度值
- 仅用于样式或注释的常量

### 9.3 第三优先级：实板验证后的细修

只有在实板已经稳定点亮并可交互之后，再处理这些问题：

- 视觉留白是否协调
- 文字大小是否需要微调
- 边缘安全区是否需要额外收缩
- 某些页面是否要为 `466x466` 做非线性布局修正

### 9.4 中期选项

如果后续项目统一升级到 `ESP-IDF 5.5+`，可以重新评估是否切到更完整的 Waveshare 官方 BSP。

在那之前，当前路线仍然是最稳妥的：

- 沿官方器件栈走
- 在仓库内维护板级适配层
- 保持业务逻辑和 UI 逻辑可复用

## 10. 最终判断

当前仓库已经做到：

- 能为 `ESP32-S3-Touch-AMOLED-1.75` 生成专用固件
- 能在结构上隔离 `1.85` 与 `1.75` 的底层差异
- 能通过 `ui_platform + ui_layout` 开始承接不同分辨率与圆屏几何差异
- 能通过静态测试和双构建验证持续保护这条适配路径

当前仓库还没有做到：

- 用真实 `1.75` 板卡完成最终点亮、触摸、亮度和整页观感验收

所以现在最准确的结论是：

- 架构方向已经正确
- 代码已经进入可持续迭代阶段
- 适配层已经不是概念，而是已落地并已通过构建/静态验证
- 剩余风险主要集中在真实硬件 bring-up 和少量 UI 细修
## 11. Flash Backup Before First WS175 Bring-up

Before flashing any custom firmware to the real `ESP32-S3-Touch-AMOLED-1.75` board, back up the factory flash image.

Script already available in this repo:

- `tools/backup_factory_firmware.ps1`

Recommended command for the current board on `COM12`:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\backup_factory_firmware.ps1 -Port COM12 -Mode chunked -Baud 115200 -ChunkSizeBytes 0x1000
```

Why this exact mode is recommended:

- full-image single-shot reads were not stable enough on this board
- `chunked` mode has already been proven to keep progressing
- `0x1000` chunks are already verified as readable on the real hardware
- the board has already been confirmed as `ESP32-S3 + 16MB flash + 8MB PSRAM`

Evidence already confirmed on the current real board:

- `python -m esptool --chip esp32s3 --port COM12 --baud 115200 read_flash 0x0 0x1000 ...` succeeded
- this proves the board is readable and not blocked from all flash reads
- chunked backup has been running stably for the full flash range

Backup completion criteria:

- final merged image exists
- final image size is exactly `16777216` bytes
- the `.log` file is present
- the `.parts` directory is retained at least until the first custom bring-up succeeds

Restore command template:

```powershell
python -m esptool --chip esp32s3 --port COM12 --baud 115200 write_flash 0x0 "<backup-image>.bin"
```

Do not overwrite the board before the full backup image has been verified.

## 12. WS175 Real-Board Bring-up Checklist

After the backup is complete, the next direct bring-up command is:

```powershell
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175" -p COM12 flash monitor
```

Boot-log checks:

- board name should print as `Waveshare ESP32-S3-Touch-AMOLED-1.75`
- resolution should print as `466x466`
- color depth should print as `16-bit`
- draw buffer line count should print as `20`
- touch should print as enabled
- touch transform should print as `swap_xy=0 mirror_x=1 mirror_y=1`

First-pass hardware checks:

- screen lights up reliably
- LVGL refresh is stable and not obviously corrupted
- touch input responds
- touch direction and mirroring are correct
- brightness control actually changes panel brightness

UI compatibility checks on `466x466`:

- no obvious clipping near the round-screen edges
- titles, hints, rollers and cards stay within the visible area
- dynamic home `MENU / GAUGE / ADD` flow can be entered, slid and returned correctly
- `Settings / BLE / ODBProtocal` pages can all be entered and exited
- typography still feels proportionate on the larger round panel

Current status after real-board acceptance:

- the WS175 code path, architecture and static validation are already in place
- the latest cleanup build has now also passed real-board validation
- the repository can treat the current WS175 compatibility baseline as working for:
  - boot stability
  - page flow
  - touch interaction
  - brightness control
  - the cleaned round-ring visuals on the affected pages

## 13. 2026-07-01 Follow-up

### Context

After commit `6f34889 feat: 兼容ws175`, a later `°C` text fix introduced a startup regression and several temporary UI/debug artifacts remained in the tree.

### Confirmed findings

- The `Guru Meditation Error: Cache error` seen right after `Start UI` was not a generic LVGL issue.
- Root cause was the custom-font fallback wiring added for `°C` support: code was writing `.fallback` into generated `const lv_font_t` objects stored in flash/rodata.
- The runtime-safe fix is to clone each custom font descriptor into writable RAM and set fallback on the clone instead of mutating the original font struct.
- The `D:/esp/v5.5/esp-idf` build failure around `tlsf_find_containing_block` was an environment problem, not a repository dependency problem.
- The broken tree was a mixed/corrupted ESP-IDF checkout; the clean working path is `D:\esp\v5.5.4-clean`.

### Page-switch and debug cleanup notes

- The temporary page-routing diagnostics in `ui_helpers.c` were added while debugging a WS175 page-switch failure.
- Final conclusion from that thread: the old refresh logic used by the original repository did not behave correctly on WS175.
- The working direction came from aligning the refresh path with the official WS175 demo behavior, after which the page-switch issue was resolved.
- Once the routing issue was understood, the temporary diagnostics were removed again.

### UI artifact cleanup

- `ui_img_pngblackear_png` is a real top overlay image (`306x38`) with two black corner blocks.
- On round-ring pages it visibly cuts into the white outer ring on WS175.
- The overlay was removed from:
  - `TEMP`
  - `INFO`
  - `OIL PRESS`
  - `BRAKE TEMP`
- The temporary `TEMP PAGE` banner and page-tag debug labels were also removed.

### Verification

- `powershell -ExecutionPolicy Bypass -File .\tools\run_ui_platform_static_tests.ps1`
  - passed
- `idf.py -B build -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175" build`
  - passed on `ESP-IDF v5.5.4-clean`

### Real-board acceptance result

Real hardware acceptance has now succeeded on the cleaned build.

Confirmed on board:

- no reboot loop after UI startup
- top white ring is no longer blocked on `TEMP / INFO / OIL PRESS / BRAKE TEMP`
- page transitions remain stable after removing the temporary debug diagnostics

### Remaining follow-up

- If page transitions are enhanced later, prefer extending the current `lv_scr_load_anim` path instead of introducing a second custom transition framework.

## 14. 2026-07-01 UI polish acceptance

### Context

After the cleanup build had already passed WS175 hardware acceptance, one more UI polish round was accepted on device.

### What changed

- Replaced the main page fade jumps with direction-aware `lv_scr_load_anim` transitions in `main/export_path/ui.c`.
- Kept the startup logo transition on the existing fade path, but moved the timing into shared constants.
- Removed the top black-ear overlay from `SETTINGS`.
- Adjusted the `SETTINGS` page rollers so the selected white background clips to rounded corners instead of showing square cut edges.
- Replaced the `EASTER EGG` heading with the active vehicle profile name and removed the `Up:BLE  Down:Settings` hint text.

### Verification

- `powershell -ExecutionPolicy Bypass -File .\tools\run_ui_platform_static_tests.ps1`
  - passed
- Real WS175 hardware acceptance
  - passed
  - transition behavior accepted
  - `SETTINGS` top black blocks gone
  - `SETTINGS` selected roller background no longer looks incorrectly cropped
  - `EASTER EGG` page content is clearer in the accepted build

### Implementation direction

- Preferred path for future page-switch animation work is still LVGL built-in screen transitions.
- Use directional pairs such as `MOVE_LEFT/RIGHT` for horizontal carousel pages and `MOVE_TOP/BOTTOM` for drill-in / back navigation.
- Only introduce custom animation containers if LVGL screen-load animation cannot cover a specific interaction requirement.
