# OBD BRZ Gauge 开机动画技术方案 2026-07-05

## Context

- 当前启动体验仍使用 `SKY / GAUGE` 静态文字首屏，入口在 `main/export_path/ui.c` 的 `ui_init()` 与 `my_timerMain()`。
- 现有工程已经具备 GIF 资源链路：
  - `main/export_path/ui.h` 已声明多个 `LV_IMG_DECLARE(...)`
  - `main/export_path/screens/ui_ScreenPageLogo.c` 已支持 `USE_GIF_LOGO == 1` 分支
  - `main/export_path/filelist.txt` / `main/export_path/CMakeLists.txt` 已将多个 GIF 资源编译进固件
- 当前工程还没有成熟的音频播放栈，因此本阶段音频先留空，但整体方案必须为后续音频接入预留稳定接口。

## 目标

在不破坏当前启动主流程、首页切换、BLE 首次连通与扫表体验的前提下，把现有 `SKY GAUGE` 开机页升级为可量产测试的 GIF 开机动画能力，并为后续音频文件接入预留接口。

本方案的目标不是做一个临时 demo，而是给后续“带音频的正式开机体验”打基础，所以必须满足：

1. 启动页代码边界清晰，不把逻辑继续堆回 `ui.c`
2. 优先复用现有 GIF 资源链路，避免先引入 SPIFFS / FATFS 复杂度
3. 动画结束后与现有首页、扫表、等待首包亮度逻辑无缝衔接
4. 后续接音频时只需要补底层播放实现和资源接入，不需要重做启动状态机

## 当前代码现状

### 启动链路

1. `main/app_main.c`
   - `app_bootstrap_init_storage_and_profile()`
   - `app_bootstrap_init_board_and_display()`
   - `app_lvgl_port_init()`
   - `app_lvgl_port_start_task()`
   - `ui_init()`
   - `app_bootstrap_start_runtime_services()`
2. `main/export_path/ui.c`
   - `ui_init()` 创建并加载 `ui_ScreenPageLogo`
   - `my_timerMain()` 每 `200 ms` 轮询
   - `my_timerMain()` 约 `3 s` 后切到首页
3. `main/export_path/ui_legacy_runtime.c`
   - `ui_legacy_runtime_tick()` 负责 BLE 连上后扫表触发
   - `ui_legacy_runtime_on_logo_exit()` 负责 logo 退场后承接挂起的扫表

### 当前启动页

- 页面构建函数：
  - `main/export_path/screens/ui_ScreenPageLogo.c`
- 当前默认实现：
  - 黑底
  - `SKY`
  - `GAUGE`
  - 圆环装饰
- 已有 GIF 分支：
  - `#if USE_GIF_LOGO == 1`
  - `lv_gif_create()`
  - `lv_gif_set_src(imageLogo, &gifSnake400)`

### 已有 GIF 资源

当前已编入工程的 GIF 资源：

1. `gifBlackLeopard`
2. `gifBlueLightLogo`
3. `gifCuteCatZip`
4. `gifKaBiBaLaZip`
5. `gifSnake400`

这意味着第一版完全可以不引入新资源格式，直接选一支现有 GIF 跑通启动体验。

## 最终推荐架构

### 核心决策

采用“启动体验状态机 + 启动页独立模块 + 音频空实现接口”的三层架构。

不建议继续把启动动画控制直接写在 `ui.c` 的 `my_timerMain()` 里，否则后面一旦加上：

- 音频同步
- 用户跳过
- 最短展示时长
- BLE 提前连上后的扫表衔接
- 动画失败兜底

`ui.c` 会迅速重新变成总控垃圾场。

### 模块拆分

建议新增以下文件：

1. `main/export_path/ui_boot_experience.h`
2. `main/export_path/ui_boot_experience.c`
3. `main/export_path/screens/ui_ScreenPageBootAnim.c`
4. `main/export_path/screens/ui_ScreenPageBootAnim.h`
5. `main/app_obd_dsp/boot_audio_stub.h`
6. `main/app_obd_dsp/boot_audio_stub.c`

职责划分如下：

#### `ui_boot_experience.*`

负责启动状态机和对外统一入口：

1. 初始化启动体验上下文
2. 记录启动开始时间
3. 管理动画页退出条件
4. 决定何时切首页
5. 预留音频启动、停止、完成查询接口
6. 向 `ui.c` 暴露最小 API

建议对外接口：

```c
void ui_boot_experience_init(void);
void ui_boot_experience_tick(void);
bool ui_boot_experience_is_active(void);
void ui_boot_experience_force_finish(const char *reason);
```

#### `ui_ScreenPageBootAnim.*`

只负责页面创建和对象持有，不负责状态判断：

1. 创建 boot screen
2. 创建 GIF 对象
3. 创建可选的品牌文字 / 版本号 / 黑色遮罩
4. 暴露获取根对象和 GIF 对象的接口

建议保留 `ui_ScreenPageLogo` 不直接删掉，分两步走：

1. 第一版先把 `ui_ScreenPageLogo` 的实现替换成“启动动画页”
2. 第二版若确认稳定，再考虑重命名为 `ui_ScreenPageBootAnim`

这样可以显著减少第一次改动对现有 UI 导出代码的冲击。

#### `boot_audio_stub.*`

这一层本阶段只提供空实现，保证未来接音频不改状态机接口。

建议接口：

```c
void boot_audio_stub_init(void);
void boot_audio_stub_start(void);
void boot_audio_stub_stop(void);
bool boot_audio_stub_is_finished(void);
bool boot_audio_stub_is_available(void);
```

当前全部返回“不可用 / 已结束”即可。

## 启动状态机设计

### 状态定义

```c
typedef enum {
    UI_BOOT_STATE_IDLE = 0,
    UI_BOOT_STATE_ANIM_PREPARE,
    UI_BOOT_STATE_ANIM_PLAYING,
    UI_BOOT_STATE_WAIT_MIN_DURATION,
    UI_BOOT_STATE_EXITING,
    UI_BOOT_STATE_DONE,
} ui_boot_state_t;
```

### 状态行为

#### `IDLE`

- 尚未进入启动动画
- 仅在 `ui_init()` 之前存在

#### `ANIM_PREPARE`

- 创建启动页
- 创建 GIF 控件
- 记录 `boot_start_ms`
- 启动音频 stub

#### `ANIM_PLAYING`

- GIF 持续播放
- 每个 `tick` 检查：
  - 是否达到最短展示时间
  - 是否用户请求跳过
  - 是否动画资源创建失败

#### `WAIT_MIN_DURATION`

- 用于约束视觉观感
- 避免资源加载极快时开机页一闪而过

#### `EXITING`

- 提前准备首页
- 调用现有首页初始化逻辑
- 通过现有 `_ui_screen_change(...)` 离开启动页
- 退出时调用 `ui_legacy_runtime_on_logo_exit()`

#### `DONE`

- 启动体验完成
- 后续 `my_timerMain()` 不再参与启动页控制

## 时序方案

### 推荐时序

第一版推荐固定以下时序：

1. `0 ms`
   - LVGL ready
   - 进入启动动画页
2. `0 ~ 2200 ms`
   - GIF 播放
3. `2200 ms`
   - 若首页未准备则开始准备
4. `2400 ms`
   - 触发页面切换
5. `切页完成后`
   - 若 BLE 已连上且扫表挂起，调用现有逻辑承接

### 为什么不建议继续用当前固定 3 秒

当前 `3 s` 固定超时对静态 logo 合理，但对 GIF 来说不够精细：

1. 有些 GIF 实际主观节奏在 `2.0 ~ 2.5 s`
2. 强制拖到 `3 s` 容易显得黏滞
3. 后续加音频后还要再改一次

因此第一版建议把总时长收敛到：

- 目标值：`2400 ms`
- 可配置范围：`1800 ~ 3200 ms`

## GIF 资源策略

### 第一阶段

直接复用已编译进固件的 GIF 资源，不新增文件系统。

建议在 `ui_boot_experience.c` 内部维护一个资源描述：

```c
typedef struct {
    const lv_img_dsc_t *gif_src;
    const char *name;
    uint16_t min_show_ms;
} ui_boot_anim_profile_t;
```

初版先固定一支，例如：

- `gifBlueLightLogo`
或
- `gifSnake400`

### 第二阶段

如果你后面给的正式动画文件尺寸较大，再决定是否升级资源方式：

1. 仍然编译进固件
   - 适合小于 `300 ~ 500 KB` 的 GIF
   - 启动最简单
2. 单独文件系统分区
   - 适合更大的资源
   - 但当前并不推荐作为第一版前置条件

### 当前分区约束

当前 `partitions.csv` 只有：

- `nvs`
- `phy_init`
- `factory 8M`

本地构建结果显示：

- app binary size `0x67f2c0`
- 剩余约 `19%`

所以第一版继续内嵌 GIF 是合理的，但要控制资源体积，避免把可执行空间快速吃光。

## 与现有逻辑的衔接规则

### 与 `ui.c` 的关系

建议把 `my_timerMain()` 中这段职责迁出去：

1. 启动页超时计数
2. 首页切换判定
3. 启动页退场回调

`my_timerMain()` 只保留：

1. 亮度延迟初始化
2. 等待首包亮度 pulse
3. 调用 `ui_boot_experience_tick()`
4. 调用 `ui_legacy_runtime_tick()`

### 与扫表动画的关系

必须保持以下行为不变：

1. BLE 若在启动页期间连上
   - 不立即在启动页上扫表
   - 只记录 `s_sweep_pending`
2. 启动页退出后
   - 调用 `ui_legacy_runtime_on_logo_exit()`
   - 由现有逻辑触发扫表

这能保证：

- 启动动画和扫表不互相抢视觉焦点
- 不破坏你现有的连接即扫表体验

### 与等待首包亮度逻辑的关系

当前等待首包亮度 pulse 的触发条件之一是：

- `ui_ScreenPageLogo == NULL`

如果启动页对象名变了，这段逻辑会失效。

因此第一版建议：

1. 不修改 `ui_ScreenPageLogo` 这个全局 screen 名称
2. 只替换它的内容和控制方式

这样能最大限度降低回归风险。

## 音频预留方案

本阶段不实现音频解码和播放，但要把接口位置定好。

### 预留策略

在 `ui_boot_experience.c` 中只通过 `boot_audio_stub_*()` 访问音频，不直接依赖具体驱动。

未来接音频时替换为：

1. `boot_audio_player.c`
2. `board_audio_*` 板级接口
3. `WAV PCM` 流式输出

### 未来推荐格式

等你给音频文件后，第一版正式播放建议优先支持：

1. `WAV`
2. `PCM 16-bit`
3. `mono`
4. `16 kHz` 或 `22.05 kHz`

不建议第一版直接上 MP3：

1. 解码复杂度更高
2. 启动时延更难控
3. 出问题时定位成本更高

## 配置项建议

建议在 `main/Kconfig.projbuild` 补以下配置：

1. `CONFIG_OBD_BOOT_ANIM_ENABLE`
2. `CONFIG_OBD_BOOT_ANIM_MIN_SHOW_MS`
3. `CONFIG_OBD_BOOT_ANIM_ALLOW_SKIP`
4. `CONFIG_OBD_BOOT_ANIM_USE_GIF`

第一版默认值建议：

1. `ENABLE = y`
2. `MIN_SHOW_MS = 2400`
3. `ALLOW_SKIP = n`
4. `USE_GIF = y`

原因：

- 上版测试阶段更需要稳定性，不需要一开始就开放点击跳过
- 不开放跳过可以先把时序和退场逻辑做扎实

## 具体实施步骤

### 阶段 A：安全重构

1. 新增 `ui_boot_experience.*`
2. 把 `logo timeout` 逻辑从 `my_timerMain()` 下沉到该模块
3. 保持视觉效果不变
4. 确认 build 通过

验收标准：

- 功能和现在一致
- 启动页仍能正常切首页
- BLE 连上后的扫表不回归

### 阶段 B：接入 GIF 开机页

1. 复用 `ui_ScreenPageLogo`
2. 打开 GIF 分支
3. 选择一支现有 GIF 作为正式启动页
4. 去掉旧的 `SKY / GAUGE` 静态文字实现
5. 微调最短展示时长

验收标准：

- GIF 正常显示
- 切页后无残影、无空白帧
- 构建空间仍有健康余量

### 阶段 C：音频占位接入

1. 新增 `boot_audio_stub.*`
2. 由 `ui_boot_experience` 统一调用
3. 当前全部空实现

验收标准：

- 编译链路稳定
- 后续替换为真实音频播放器时不需要改启动状态机

### 阶段 D：板上验证

1. 冷启动多轮验证
2. BLE 快速连上场景验证
3. 首页默认页不同配置验证
4. 长时间通电后的重复重启验证

验收标准：

1. 无黑屏卡死
2. 无切页闪白
3. 无扫表漏触发
4. 无亮度异常抖动

## 测试清单

上版前至少验证：

1. `display-only bootstrap` 关闭时正常启动
2. `display-only bootstrap` 打开时不会误进入完整启动动画状态机
3. 默认页为 `MENU`
4. 默认页为某个 gauge page
5. BLE 在启动页期间连上
6. BLE 在启动页结束后才连上
7. GIF 资源创建失败时自动退回纯黑底或静态 logo
8. 亮度延迟初始化与启动动画共存
9. 启动页销毁后对象指针被正确清空

## 风险与规避

### 风险 1：GIF 占用过大导致固件空间或内存压力上升

规避：

1. 第一版只用现有资源
2. 构建后检查 binary size
3. 若正式资源过大，再评估文件系统分区

### 风险 2：启动页对象名变化导致现有亮度 / 扫表逻辑失联

规避：

1. 第一版保留 `ui_ScreenPageLogo` 命名
2. 只替换内容，不急于大规模重命名

### 风险 3：把状态机继续塞进 `ui.c` 导致后续音频接入成本很高

规避：

1. 第一版就拆出 `ui_boot_experience.*`
2. 音频统一走 stub 接口

## 最终结论

当前最合适的路线不是直接“把某个 GIF 塞到 logo 页里”，而是：

1. 先把启动体验从 `ui.c` 中拆成独立状态机
2. 第一版复用现有 GIF 资源完成开机动画
3. 保持 `ui_ScreenPageLogo` 命名和扫表承接逻辑不变
4. 预留音频 stub，等你提供音频文件后再接入真实播放

这条路线的好处是：

1. 你可以很快拿到一个能上板测试的 GIF 开机动画版本
2. 后续接音频时不会推倒重来
3. 对现有首页、BLE、扫表和亮度等待逻辑的回归风险最低
