# Settings / OBD 统一交互重构方案

## 1. 目标

本方案定义 `Settings` 与 `OBD Protocol` 的最终交互模型、页面结构、状态机、代码改造边界与验收标准。

本方案的唯一目标是把当前三套不一致的交互收敛为一套稳定模型：

1. 仪表主页继续保持当前体验不变。
2. `OBD Protocol` 不再作为独立页面存在，改为 `Settings` 内的一个分类页。
3. `Settings` 从“根页 + 详情页 + 局部纵向滚动”改为“横向分类页 + 控件内部操作”。
4. 不使用说明性交互提示文案，不依赖“小字告诉用户怎么滑”。

本方案不包含视觉主题大改，不改变仪表页逻辑，不重构底层数据采集链路。

## 2. 最终交互模型

### 2.1 用户心智模型

系统统一为以下规则：

1. `Home / Gauge` 页：
   - 左右滑：切换仪表页
   - 上滑：进入 `BLE Scan`
   - 下滑：进入 `Settings`
2. `Settings` 体系：
   - 左右滑：切换分类页
   - 上滑：返回上一层
3. 控件操作：
   - `roller` 自身上下滚动只负责改值
   - 页面容器不承担纵向滚动

用户只需要记住：

1. 主页左右翻数据
2. 下滑进设置
3. 设置里左右换分类
4. 上滑返回

### 2.2 交互不变项

以下行为必须保持不变：

1. `Home` 仪表页左右切换的手感不变。
2. `Home` 页上滑进入 `BLE Scan`、下滑进入 `Settings` 的入口不变。
3. 仪表页长按进入编辑模式的行为不变。
4. 仪表页与菜单页的切换逻辑不变。

### 2.3 交互变更项

以下行为必须废弃：

1. `Settings` 页面容器纵向滚动。
2. `Settings` 根页卡片点击后进入单独“详情页”的二级信息架构。
3. 独立 `OBD Protocol` 页面。
4. `OBD Protocol` 页“长按保存并立即重启”的隐式确认模型。
5. 任何“Swipe up to go back”之类的交互提示小字。

## 3. 页面信息架构

### 3.1 新的 Settings 顶层结构

`Settings` 改为单一 screen，内部有固定顺序的横向分类页：

1. `DISPLAY`
2. `DASHBOARD`
3. `VEHICLE`
4. `OBD`

这四页都是同级页，不再存在 “root category list -> detail page” 两级跳转。

### 3.2 分类页职责

#### `DISPLAY`

包含：

1. `Brightness`
2. `Rotation`（仅在 `CONFIG_OBD_BOARD_WS_175_AMOLED` 下显示）

约束：

1. 所有控件必须在一屏内完成布局。
2. 不允许通过纵向滚动才能看到完整内容。

#### `DASHBOARD`

包含：

1. `Boot Page`
2. `OBD Poll`
3. `RaceChrono BLE`
4. `Oil Pressure`

约束：

1. 所有控件必须在一屏内完成布局。
2. 若一屏空间不足，只允许通过压缩卡片排版或拆成两个同级分类页解决，不允许重新引入纵向滚动。

#### `VEHICLE`

包含：

1. `Vehicle Profile`

约束：

1. 保持单页简洁布局。
2. 车型切换后的“主页需重建”行为保留。

#### `OBD`

包含：

1. `Protocol`

预留：

1. 后续若新增 `OBD adapter mode`、`CAN monitor mode`、`protocol fallback strategy`，统一并入该页。

约束：

1. `OBD` 页只负责 OBD 连接策略和协议相关设置。
2. 不允许再出现独立 OBD 设置 screen。

## 4. 页面状态机

### 4.1 Screen 级状态

运行态 screen 只保留：

1. `ui_ScreenPageHome`
2. `ui_ScreenPageBLEScan`
3. `ui_ScreenPageSettings`

`ui_ScreenPageODBProtocal` 从主流程中移除。

### 4.2 Settings 内部状态

`Settings` 内部只保留一个横向页索引，不再保留“section root / section detail”双层状态。

建议状态定义：

```c
typedef enum {
    UI_SETTINGS_PAGE_DISPLAY = 0,
    UI_SETTINGS_PAGE_DASHBOARD,
    UI_SETTINGS_PAGE_VEHICLE,
    UI_SETTINGS_PAGE_OBD,
    UI_SETTINGS_PAGE_COUNT
} ui_settings_page_t;
```

必须满足：

1. 任一时刻只存在一个 active settings page。
2. 页面切换由横向手势或显式切页控件驱动。
3. `上滑` 永远表示离开当前层级。

### 4.3 返回规则

返回语义固定如下：

1. `Settings` 任一分类页 `上滑`：
   - 直接返回 `Home`
   - 默认落点：`UI_HOME_PAGE_MENU_ID`
2. `BLE Scan` `上滑`：
   - 返回 `Home`
   - 默认落点：`UI_HOME_PAGE_MENU_ID`
3. 不保留 `Settings` 内部“返回 root 分类页”的概念，因为分类本身已经是同级横向页。

## 5. 交互与控件规则

### 5.1 roller 规则

所有 `roller` 控件必须满足：

1. 触控高度不小于 `ui_layout_px(58)`。
2. 文本必须在单行内可见，不允许选中态文字溢出容器。
3. `roller` 的上下拖动不向页面容器传播成页面纵向切换。
4. `roller` 的左右拖动不应导致误切页。

### 5.2 页面切换承载方式

`Settings` 内部页面切换建议使用与 `Home` 一致的横向 tile 容器思路，但不要求与 `Home` 共用完全相同的业务实现。

必须满足：

1. 切页方向一致：左右切换同级页。
2. 生命周期一致：页面切换后只刷新当前活跃页需要的 UI。
3. 手势优先级一致：横向切页优先，控件操作不抢返回手势。

不强制要求：

1. `Settings` 与 `Home` 共用同一个 `tileview` 实例。
2. `Settings` 与 `Home` 共用同一个 descriptor 数据结构。

### 5.3 Modal 规则

所有设置类确认弹窗统一使用 `ui_round_shell_apply_modal_theme()`。

Modal 行为固定为：

1. 普通设置项：
   - 改值后立即保存
   - 如需提醒，弹 `Saved` modal
2. 高影响设置项：
   - 改值后先弹确认 modal
   - 用户确认后再写入配置并执行副作用

### 5.4 OBD Protocol 的新确认模型

`OBD Protocol` 改为 `Settings -> OBD -> Protocol`。

规则固定如下：

1. 用户滚动 `Protocol` roller 只改变当前候选值。
2. 若候选值与已持久化值相同，不弹确认，不重启。
3. 若候选值与已持久化值不同，弹统一确认 modal。
4. Modal 文案固定表达“应用该协议需要重启”。
5. 用户点击确认后：
   - 写入 `cfg.protocol`
   - 调用 `nvs_cfg_set(&cfg)`
   - 调用 `esp_restart()`
6. 用户点击取消后：
   - 不写入 NVS
   - `roller` 恢复到当前已持久化值

此处不允许保留“长按保存”作为后门交互。

## 6. 代码改造边界

### 6.1 必改文件

#### [main/export_path/screens/ui_ScreenPageSettings.c](/D:/Program Files (x86)/Code Projects/obd_brz_gauge/main/export_path/screens/ui_ScreenPageSettings.c)

必须完成：

1. 删除现有 `UI_SETTINGS_SECTION_ROOT / DISPLAY / DASHBOARD / VEHICLE` 二级结构。
2. 引入新的同级页枚举 `DISPLAY / DASHBOARD / VEHICLE / OBD`。
3. 去掉 `s_settings_content` 的页面级纵向滚动。
4. 引入横向页容器与 active-page 管理。
5. 将 `Protocol` roller 迁入该文件内的 `OBD` 分类页。
6. 实现 `Protocol` 改值确认 modal 和取消回滚逻辑。

#### [main/export_path/ui.c](/D:/Program Files (x86)/Code Projects/obd_brz_gauge/main/export_path/ui.c)

必须完成：

1. 删除 `ui_ScreenPageODBProtocal` 相关全局对象声明。
2. 删除 `ui_event_obd_prot_background()` 事件入口。
3. 删除 logo 页双击进入 OBD page 的入口逻辑。
4. 删除 `SAVE_PROTOCOL_TIME`、`usSaveProtTimeCnt` 及相关长按状态变量。

#### [main/export_path/ui.h](/D:/Program Files (x86)/Code Projects/obd_brz_gauge/main/export_path/ui.h)

必须完成：

1. 删除 `ui_ScreenPageODBProtocal` 的 screen 声明和 extern。

#### [main/export_path/screens/ui_ScreenPageODBProtocal.c](/D:/Program Files (x86)/Code Projects/obd_brz_gauge/main/export_path/screens/ui_ScreenPageODBProtocal.c)

处理策略必须二选一：

1. 若本轮允许删文件：直接移除该 screen 源文件与引用。
2. 若本轮暂不删文件：保留文件但必须确保主流程无入口、无引用、无对外声明。

推荐方案：直接移除。

### 6.2 可选抽取文件

若 `Settings` 横向页逻辑明显变长，允许新增：

1. `main/export_path/ui_settings_runtime.h`
2. `main/export_path/ui_settings_runtime.c`

但必须满足：

1. `ui_ScreenPageSettings_screen_init()` 仍然是唯一 screen 初始化入口。
2. 不引入第二套和 `Home` 页相互复制但无法复用的布局常量体系。

## 7. 布局约束

### 7.1 一屏约束

每个 `Settings` 分类页必须满足：

1. 不依赖页面容器纵向滚动即可看到全部内容。
2. 控件在圆屏安全区内可完整点击。
3. 标题、控件值、选中态文本不溢出。

### 7.2 圆屏安全区约束

布局计算必须继续基于当前工程的圆屏安全区方法：

1. `ui_safe_margin()`
2. `ui_settings_safe_span_for_band()` 或其等价几何方法
3. `ui_layout_px()`

不允许重新引入依赖绝对像素魔法数的整页布局。

### 7.3 分类切换视觉约束

分类页切换时必须可被用户感知，但不需要额外交互说明。

至少满足其一：

1. 顶部标题随 active page 更新。
2. 页面主体左右切换时有明确位置变化。
3. 存在页序指示器或分类标签高亮。

推荐同时具备 1 和 3。

## 8. 数据与持久化规则

### 8.1 即时保存项

以下项保持即时保存：

1. `Brightness`
2. `Rotation`
3. `Boot Page`
4. `OBD Poll`
5. `RaceChrono BLE`
6. `Oil Pressure`
7. `Vehicle Profile`

### 8.2 需要确认后执行项

以下项必须确认后执行：

1. `OBD Protocol`

原因固定为：

1. 修改后需要重启
2. 副作用明显大于普通配置项

### 8.3 Vehicle Profile 的副作用

`Vehicle Profile` 改动后：

1. 立即写入 NVS
2. 立即调用 `vehicle_profile_set_active(selected)`
3. 标记 `s_settings_requires_home_rebuild = true`
4. 退出 `Settings` 返回 `Home` 时重建主页

此规则保留不变。

## 9. 迁移步骤

实施顺序固定如下：

1. 重构 `Settings` 数据结构与内部枚举。
2. 引入 `Settings` 横向分类页容器。
3. 把 `DISPLAY / DASHBOARD / VEHICLE` 三页迁移到新容器。
4. 将 `OBD Protocol` roller 迁移进 `Settings -> OBD`。
5. 用确认 modal 替换 `OBD Protocol` 长按保存逻辑。
6. 删除 `ui.c / ui.h` 中 OBD 独立页入口和状态。
7. 处理 `ui_ScreenPageODBProtocal.c` 文件删除或退役。
8. 跑 UI 静态测试。
9. 跑完整编译。

不允许颠倒为“先删 OBD 页，再想办法把协议设置补回去”，否则中间态会产生功能缺口。

## 9.1 开发前已知问题

以下问题已经在当前代码中确认存在，本轮开发前只做记录，不做过渡态修补，统一在本方案落地后回归验收：

1. `Settings` 当前页面容器曾引入纵向滚动，同时保留 `上滑返回`，存在手势语义冲突。
2. `Dashboard Config` 当前也存在“容器纵向滚动 + 返回动作入口不统一”的同类问题，本轮需要一起回归验证。
3. 旧的独立 `OBD Protocol` 页仍保留独立入口与长按保存重启逻辑，和目标交互模型不一致。
4. 当前 `WS175` 构建存在与 `oil_pressure_set_enabled` 相关的链接阻塞，必须在本轮开发末尾一并收口。

## 9.2 本轮继续开发前补充问题记录

以下问题是在统一方案初步落地后，继续对照代码确认出的剩余收口项。本轮开发直接按这些点继续修复，并在完成后统一回归：

1. `Settings` 当前顶部分类名仍为 `DISPLAY / RUNTIME / SYSTEM / OBD`，和最终信息架构中的 `DISPLAY / DASHBOARD / VEHICLE / OBD` 不一致。
2. `BOOT PAGE` 当前仍和 `VEHICLE PROFILE` 放在同一分类页内，没有按最终方案拆分到 `DASHBOARD` 与 `VEHICLE` 两个独立分类页。
3. `Dashboard Config` 虽已去掉主要纵向滚动依赖，但页面结构、标题层级、返回路径和 `Settings` 体系仍未完全统一到同一套圆屏运行时交互语言。
4. `Dashboard Config` 的行式布局目前以“压缩显示全部项”为优先，仍需要继续验证长文案、长指标名和不同分辨率下的可触达性是否足够稳定。
5. 本轮开发完成后，必须按统一回归清单重新验证 `Settings` 与 `Dashboard Config`，不接受仅静态测试通过但真机交互仍别扭的结果。

这些问题在本轮中的处理原则固定如下：

1. 不做局部止血式交互补丁。
2. 直接按统一交互方案整体改造。
3. 改造完成后统一回归，不接受“交互结构仍冲突但靠提示文案规避”的结果。

## 10. 验收标准

### 10.1 交互验收

必须全部满足：

1. 从 `Home` 下滑进入 `Settings`。
2. `Settings` 内左右滑能切换 `DISPLAY / DASHBOARD / VEHICLE / OBD`。
3. `Settings` 任一分类页上滑直接返回 `Home`。
4. 用户不需要阅读说明文案即可完成分类切换与返回。
5. `Settings` 页面不存在页面级纵向滚动。
6. `Dashboard Config` 不再出现“纵向滚动”和“上滑返回”抢手势的情况。

### 10.2 OBD 验收

必须全部满足：

1. 不再存在独立 `OBD Protocol` 入口。
2. `Protocol` 配置只能在 `Settings -> OBD` 修改。
3. 修改 `Protocol` 时会弹确认 modal。
4. 点击确认后写入 NVS 并重启。
5. 点击取消后恢复原始选择，不写入 NVS，不重启。
6. 不再存在长按保存逻辑。

### 10.3 布局验收

必须全部满足：

1. 所有分类页一屏显示完整。
2. 选择器高度明显大于当前旧版二级设置页。
3. 选中项文字不溢出。
4. 圆屏边缘不存在难以点击的控件。

### 10.4 工程验收

必须全部满足：

1. `tools/run_ui_platform_static_tests.ps1` 通过。
2. `idf.py build` 通过。
3. 不新增未使用的全局 screen 状态。
4. 不保留死代码入口。
5. `WS175` 目标构建不再出现 `oil_pressure_set_enabled` 链接错误。

## 11. 非目标

本轮明确不做：

1. 改造 `Home` 页为纵向多层结构。
2. 改造 `BLE Scan` 信息架构。
3. 调整仪表页编辑模式交互。
4. 重做 UI 主题色系统。
5. 重构 OBD 后端协议检测算法。

## 12. 最终决策

本方案的最终决策如下：

1. `Settings` 与 `OBD` 必须并入统一 screen 体系。
2. `Settings` 必须采用横向分类页，而不是纵向长表单。
3. 页面切换规则必须向 `Home` 页靠拢，而不是继续扩大 `Settings` 的例外行为。
4. `OBD Protocol` 必须改为明确确认模型，不允许继续使用长按保存。
5. 交互模型必须自解释，不允许依赖说明性小字提示。
