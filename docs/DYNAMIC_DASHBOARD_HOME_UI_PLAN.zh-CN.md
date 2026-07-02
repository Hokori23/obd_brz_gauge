# 动态仪表首页技术方案

## 1. 目标

本文档定义 WS175 圆屏首页的最终信息架构、交互规则、数据模型、代码边界与实施顺序。目标是把当前已经跑通的动态首页基线，收敛为一套后续可持续迭代的实现方案。

本文档是该功能的唯一实现规格。后续涉及首页重构、编辑态、仪表页新增/删除、配置页改造时，以本文档为准。

## 2. 评审结论

### 2.0 当前代码对照结论（基于 2026-07-01 仓库）

本次已按当前仓库真实代码复核本文档，主要结论如下：

1. 方案不是停留在设计阶段，动态首页主干已经落地；以当前仓库代码看，阶段 B、C、D 的软件侧目标已经收口完成。
2. `ui_home_runtime.c` 已经成为动态首页的主实现文件，承担首页 tile 构建、编辑态、删除确认和首页刷新。
3. 仪表页配置 screen 已经拆到 [ui_dashboard_config.c](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_dashboard_config.c:1) / [ui_dashboard_config.h](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_dashboard_config.h:1)，不再继续内嵌在 `ui_home_runtime.c`。
4. `my_timerMain()` 仍然存在，但动态首页刷新已经下沉到 [ui_home_runtime.c](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_home_runtime.c:1043) 自己管理的 LVGL timer，旧总控对首页的直接周期刷新耦合已被移除；随着旧固定页退场，`ui_legacy_runtime.c` 当前已进一步收敛为数据项元信息与 sweep 状态管理。
5. `ui_home_runtime_page_from_default()` 已经恢复为基于动态首页 tile 的映射，`Settings` 页里的 `BOOT PAGE` 也已经能真正落到 `MENU / GAUGE n`。
6. 本轮复核还已完成软件侧验证：`tools/run_ui_platform_static_tests.ps1` 通过，`idf.py -B build_default -D SDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.esp32s3' reconfigure build` 与 `idf.py -B build_ws175 -D SDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175' reconfigure build` 均已成功产出 `.bin`。
7. 本轮还顺手修正了两个与方案状态判断直接相关的仓库侧问题：`main/bsp_obd_dsp/rs485_brake_temp.c` 现已兼容 `CONFIG_OBD_RS485_DE_RE_GPIO = -1` 的板型构建路径；`sdkconfig.defaults` 中此前遗留的无效 capability 赋值与旧 Kconfig 名称迁移项也已清理，当前剩余提示以外部环境级 warning 为主，不再是本仓库配置噪声。

### 2.1 当前实现里已经成立的部分

以下能力已经在当前分支存在，方向正确，不需要推翻：

1. 首页结构已经从固定 6 页改成 `MENU + N 个仪表页 + ADD`。
2. `N` 的持久化已经走 `dashboard_cfg`，支持 `0..8` 页。
3. 单个仪表页已经支持 `1..6` 个 slot。
4. 新增页、删除页、配置页返回后，首页会整体重建并重新落到目标页。
5. `ui_round_shell_create_ring()` 已被改成透明，不再主动绘制白色环。
6. 长按当前仪表页进入编辑态、删除确认、配置页 roller 编辑，这条主流程已经可用。
7. `MENU` / 仪表页 / `ADD` 三类 tile 已经由 [ui_home_runtime.c](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_home_runtime.c:156) 按描述动态构建，不再依赖旧固定首页布局。
8. 首页返回入口已经收敛到 `ui_home_runtime_show_page()`；当前主流程中的二级页主要是 `BLE Scan`、`Settings` 和 `ODBProtocal`。

### 2.2 当前实现和目标方案的差距

当前剩余项已经不再是首页主体结构缺口；按当前仓库代码看，本方案范围内的软件主实现已基本收口，剩余重点主要是真机验证：

1. 动态首页运行时与配置页已经分别落在 `ui_home_runtime.c` 和 `ui_dashboard_config.c`，旧功能页运行时也已拆到 `ui_legacy_runtime.c`；历史固定页已经不再保留在主流程代码路径中。
2. `Temp / Info / Needle / OilPressure / BrakeTemp / BrakeWarn / OilWarn / EasterEgg` 及 `TempCustom / InfoCustom` 等衍生页已从工程主线移除，不再作为兼容页面继续留在 `screens/` 和 `ui.c/ui.h` 中。
3. `ui_init()` 只保留当前真实需要的页面状态；`ODBProtocal` 继续按需初始化，不在启动时预创建。
4. `BLE Scan`、`Settings`、`ODBProtocal` 三个当前主流程二级页都已经补上 `LV_EVENT_SCREEN_UNLOADED` / `LV_EVENT_DELETE` 生命周期清理，重复打开会重建新 screen；其中 `BLE Scan` 页面销毁时也已兜底调用 `elm327_ble_scan_only_stop()`，避免后台扫描残留。
5. 后续仍需按第 10 节做真机交互与视觉验收，这部分不能由静态检查或构建结果替代。

### 2.3 最终判断

结论不是“重做”，而是“收口”：

1. 现有 `dashboard_cfg` 数据模型保留。
2. 现有 `MENU + N + ADD` 首页结构保留。
3. 现有“整体重建首页”策略保留。
4. 后续要在现有结构上补一个 `page type` 维度：`指标仪表页` 与 `G-force 页`。
5. `ZC6` 的横纵向加速度表适合优先落地，因为公开资料已能支撑其被动 CAN 数据来源。
6. 后续工作重点已经从首页主体实现转为 `页面类型扩展 + 真机交互与视觉验收`。

## 3. 最终产品定义

### 3.1 首页结构

首页横向滑动顺序固定为：

1. 第 0 页：`MENU`
2. 第 `1..N` 页：可编辑仪表页
3. 最后一页：`ADD`

固定规则如下：

1. `MENU` 永远存在。
2. `ADD` 永远存在。
3. `N` 取值范围固定为 `0..8`。
4. 当 `N = 0` 时：首页只剩 `MENU` 和 `ADD`。
5. 当 `N = 8` 时，`ADD` 仍显示，但点击后只弹提示，不创建新页。

### 3.2 页面角色

`MENU` 页职责：

1. 展示当前 vehicle profile 名称。
2. 展示 BLE 设备名摘要。
3. 当前代码里通过上下滑手势提供进入二级页入口：上滑进入 `BLE Scan`，下滑进入 `Settings`。
4. 不参与编辑态。

可编辑仪表页职责：

1. 仅展示仪表数据卡片。
2. 接受长按进入编辑态。
3. 接受顶部 `EDIT` 进入配置页。
4. 支持删除当前页。

在本方案里，可编辑仪表页从这一版开始细分为两类：

1. `METRIC`
   - 现有指标卡片页
   - 使用 `slot_count + slot_items[]` 驱动布局与内容
2. `G_FORCE`
   - 仅优先支持 `ZC6`
   - 展示横向 / 纵向加速度，不走多 slot 卡片布局

`ADD` 页职责：

1. 仅负责新建仪表页。
2. 不参与编辑态。
3. 不承载任何业务数据展示。

### 3.3 黑色背景约束

首页、配置页、相关 drill-in 页统一使用黑色背景基线。

落地规则固定为：

1. 不允许再出现视觉上的白色环。
2. `ui_round_shell_create_ring()` 只允许作为过渡兼容层存在，不允许恢复可见白环。
3. 新增页面若需要圆屏外框，只能使用黑底或透明实现，不得再引入白色描边视觉语言。

## 4. 数据模型

### 4.1 保留当前 schema

当前持久化结构体大小继续沿用，但 `page` 级别需要补充 `page_type` 语义。建议优先复用 `ui_dashboard_page_cfg_t.rsv` 的高位，不新增 struct 字段，避免 NVS 体积和迁移复杂度继续膨胀：

```c
#define UI_DASHBOARD_CFG_VERSION 1u
#define UI_DASHBOARD_MAX_PAGES   8u
#define UI_DASHBOARD_MAX_SLOTS   6u

typedef struct {
    uint8_t slot_count;
    uint8_t slot_items[UI_DASHBOARD_MAX_SLOTS];
    uint8_t rsv;
} ui_dashboard_page_cfg_t;

typedef struct {
    uint8_t version;
    uint8_t gauge_page_count;
    uint8_t rsv[2];
    ui_dashboard_page_cfg_t pages[UI_DASHBOARD_MAX_PAGES];
} ui_dashboard_cfg_t;
```

建议位分配：

1. `page.rsv bit0..bit5`
   - 延续当前已存在的 `unsupported slot mask` 语义
2. `page.rsv bit6..bit7`
   - 新增 `page_type`
   - `0 = METRIC`
   - `1 = G_FORCE`
   - `2 / 3` 预留

这个调整属于“扩展既有 schema 语义”，不是新增 struct 字段或重做整套持久化格式。

定义位置：

1. [nvs_storage.h](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/bsp_obd_dsp/nvs_storage.h:16)
2. [nvs_storage.c](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/bsp_obd_dsp/nvs_storage.c:219)

### 4.2 固定约束

1. `gauge_page_count` 只能是 `0..8`。
2. `page_type` 默认值固定为 `METRIC`。
3. `slot_count` 只能是 `1..6`，但仅对 `METRIC` 页生效。
4. 不允许存在“空仪表页”，删除最后一个 slot 的语义等同于删除整页。
5. `G_FORCE` 页在 v1 中忽略 `slot_count` 和 `slot_items[]` 的显示语义，但仍允许沿用现有存储结构。
6. 新建页默认值固定为：
   `slot_count = 1`，`slot_items[0] = DISP_ITEM_RPM`
7. 历史数据迁移默认页固定为：
   `MENU`
8. 若 `default_page` 因删页等原因超出当前 `gauge_page_count`，统一回退为：
   `MENU`

补充约束：

1. `G_FORCE` 类型在第一阶段只对 `ZC6` 暴露。
2. 非 `ZC6` 车型不显示 `G_FORCE` 选项，避免用户配出无法显示的页面。
3. 已存量 `METRIC` 页在旧版本升级后默认无行为变化。

### 4.3 首页页号映射

首页 tile id 固定定义为：

1. `0` 对应 `MENU`
2. `1..N` 对应 `gauge_index = tile_id - 1`
3. `N + 1` 对应 `ADD`

这个映射关系是后续所有新增、删除、回落、配置返回逻辑的统一基础，不再引入第二套映射。

## 5. 编辑态规格

### 5.1 进入条件

只有“当前激活的仪表页”允许长按进入编辑态。

明确禁止进入编辑态的页面：

1. `MENU`
2. `ADD`
3. 非当前激活页
4. 任意二级功能页

### 5.2 进入效果

长按命中后，立即执行以下动作：

1. 当前仪表页内容根节点缩放到 `1.2x`
2. 禁止 `tileview` 横向滚动
3. 在当前 tile 上创建半透明黑色遮罩层
4. 遮罩透明度固定为 `opa = 96`

缩放对象固定为“当前仪表页内容根节点”，不是整个 screen，也不是整个 tileview。

### 5.3 编辑态操作区

编辑态固定保留三个操作对象：

1. 左半圆：`BACK`
2. 右半圆：`DELETE`
3. 顶部中间胶囊按钮：`EDIT`

这里做一个正式决策：

1. 用户原始需求明确要求左右半圆承担主操作。
2. 但页面仍然需要一个进入配置页的入口。
3. 因此 `EDIT` 保留，但降级为顶部次级入口，不再和左右主操作并列竞争。

### 5.4 左右半圆的精确定义

命中区域按整屏左半区和右半区实现，依靠圆屏裁切获得最终半圆视觉。

固定实现规则：

1. 左区尺寸：`50%` 宽，`100%` 高，左对齐。
2. 右区尺寸：`50%` 宽，`100%` 高，右对齐。
3. 父层为全屏遮罩，背景半透明黑。
4. 左区背景色固定为 `#27AE60`，透明度 `160`。
5. 右区背景色固定为 `#EB5757`，透明度 `160`。
6. 文案固定为 `BACK` 和 `DELETE`。
7. 左区点击直接退出编辑态。
8. 右区点击只弹删除确认框，不直接删。

说明：

1. 这里不是“数学意义上的半圆按钮对象”。
2. 落地方式是“半屏点击区 + 圆屏裁切”。
3. 这是当前 LVGL + 圆屏场景下最稳的实现，不再引入自绘 hit-test。

### 5.5 EDIT 入口

`EDIT` 的存在是刚需，不再悬而未决。

固定规则：

1. 位置：顶部中间。
2. 形态：胶囊按钮。
3. 文案：`EDIT`
4. 颜色：`#2F80ED`
5. 透明度：`160`
6. 点击后流程：
   先退出编辑遮罩，再进入独立配置页。

### 5.6 删除确认

删除确认统一使用 LVGL 标准 `lv_msgbox`。

固定文案：

1. 标题：`Delete Page`
2. 内容：`Delete this dashboard page?`
3. 按钮：`Cancel` / `Delete`

删除成功后的回落规则固定为：

1. 若删除页左侧仍有仪表页，则落到左侧相邻仪表页。
2. 否则若右侧仍有仪表页，则落到右侧相邻仪表页。
3. 否则落到 `MENU`。

不允许删除后停留在不存在的 tile id。

## 6. 配置页规格

### 6.1 页面形态

配置能力继续使用独立 screen，不改回首页内弹层。

原因固定为：

1. 圆屏空间不足，不适合在编辑态里再叠多组 roller。
2. 当前已有可用实现，复用成本最低。
3. 后续若扩展主题、模板、布局风格，独立 screen 可扩展性更高。

当前代码现状：

1. 配置页已经是独立 screen，实现位于 [ui_dashboard_config.c](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_dashboard_config.c:1)。
2. 配置页通过 `ui_dashboard_config_open()` 打开，并通过 `ui_home_runtime_rebuild_and_load()` 返回首页。

### 6.2 固定行为

1. 标题固定为 `DASHBOARD`
2. 页面顶部新增 `TYPE` 选择器
3. `TYPE` 选项在 v1 固定为：
   - `METRIC`
   - `G-FORCE`
4. `G-FORCE` 选项仅在当前 vehicle profile = `ZC6` 时显示
5. 当 `TYPE = METRIC` 时：
   - 显示 `slot_count` roller，范围固定为 `1..6`
   - 显示每个 slot 对应的一组 item roller
   - 当 `slot_count` 变小时，仅隐藏超出行，不强制清空缓存值
   - 当 `slot_count` 变大时，新暴露 slot 默认值写成 `RPM`
6. 当 `TYPE = G-FORCE` 时：
   - 隐藏 `slot_count` 和全部 slot item rollers
   - v1 不增加额外次级选项
   - 后续如需扩展，再考虑 `max hold`、`符号翻转`、`数值叠加` 等配置
7. 上滑返回当前仪表页
8. 选择器 / roller 修改即时写入 NVS，不新增单独 Save 按钮

### 6.3 G-force 页设计

`G_FORCE` 页优先只服务 `ZC6`，并按“先做稳、再做花”的原则落地。

数据来源建议：

1. 以公开 FT86 Gen1 文档中的 `0x0D0 (208)` 为准。
2. 首版只消费：
   - `lateral acceleration`
   - `longitudinal acceleration`
3. 先不把 `yaw rate` 和 `steering angle` 混入同一页，避免首版信息过满。

界面布局建议：

1. 中心做一个尽量吃满圆屏的十字坐标区。
2. 用一个实时移动的小圆点表示当前 `lat/lon` 合成位置。
3. 四个象限直接在圆盘内部表达语义，分别对应：
   - `ACC + LEFT`
   - `ACC + RIGHT`
   - `BRAKE + LEFT`
   - `BRAKE + RIGHT`
4. 页面中央保留细十字线与圆形边界，不叠太多说明文字。
5. 页面要有黄色历史峰值包络，不再退回成左右上下四组大数字主导。
6. v1 不再以单独数值块作为主视觉，页面主语义固定交给中心圆盘坐标图。

数值与显示规则建议：

1. 页面主语义固定为 `g`，不是 `m/s²`。
2. 数值建议显示到 `0.01g`。
3. 横向与纵向数值统一使用同一字体规格，避免再次出现不同仪表页左上角字体不一致的问题。
4. 如果当前车型不支持该页类型，首页直接不创建该 tile，配置页也不暴露该类型入口。

### 6.4 与首页的数据同步策略

配置页关闭后，不做局部热更新，固定走：

1. 重新读取 NVS
2. 重建首页 tile 描述和挂载状态
3. 回到刚才编辑的仪表页

这条策略已经被验证可用，继续保留。

## 7. 代码落地边界

### 7.1 模块划分

目标边界仍然是不再把动态首页相关能力继续堆回 `ui.c`。结合当前代码，现状与目标如下：

1. `ui.c`
   现状：已经主要收敛为 logo 后首屏进入、顶层定时入口、文本 helper、协议页事件以及少量告警/兼容入口。
2. `ui_home_runtime.c`
   现状：已经承载动态首页 tile 描述、页面挂载、编辑态和首页刷新。
3. `ui_dashboard_config.c`
   现状：已经承载仪表页配置 screen，并通过独立头文件暴露入口。
4. `ui_legacy_runtime.c`
   现状：已经收敛为数据项元信息、显示格式化与 sweep 状态机，不再继续承载旧固定页的页面级刷新细节。

对应头文件：

1. [ui_home_runtime.h](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_home_runtime.h:1)
2. [ui_dashboard_config.h](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_dashboard_config.h:1)
3. [ui_runtime_common.h](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_runtime_common.h:1)

### 7.2 对外接口

当前已经存在并可对外使用的首页运行时接口为：

```c
void ui_home_runtime_show_page(uint8_t page_id, lv_scr_load_anim_t anim);
void ui_home_runtime_refresh_active_tile(void);
uint8_t ui_home_runtime_page_from_default(uint8_t default_page);
void ui_home_runtime_reset(uint8_t initial_page_id);
void ui_home_runtime_screen_init(void);
void ui_home_runtime_rebuild_and_load(uint8_t page_id, lv_scr_load_anim_t anim);
```

当前已经存在的配置页接口为：

```c
void ui_dashboard_config_open(uint8_t gauge_index);
void ui_dashboard_config_reset(void);
```

说明：

1. `ui_home_runtime_rebuild_and_load()` 已作为 sibling UI 模块可调用接口暴露给 `ui_dashboard_config.c`。
2. `ui_dashboard_config_open()` 已是独立模块接口，不再是 `ui_home_runtime.c` 内部 `static` 函数。
3. `ui_dashboard_config_reset()` 当前作为顶层 UI reset 路径使用的生命周期接口保留，对普通页面流转不直接暴露交互语义。

### 7.3 私有内部职责

以下逻辑属于 `ui_home_runtime.c` 私有实现，不对外暴露：

1. tile 描述重建
2. tile mount / unmount
3. 活动页跟踪
4. 编辑态进入/退出
5. 删除确认
6. 新增页逻辑
7. notice / msgbox 管理

以下逻辑属于 `ui_dashboard_config.c` 私有实现，不对外暴露：

1. roller 行显隐
2. slot_count 变更落盘
3. slot item 变更落盘
4. 配置页返回首页
5. page type 切换后的控件显隐与落盘

## 8. 历史固定页的处理原则

以下页面已经不再是首页主路径的一部分，并已从工程主线移除：

1. `ui_ScreenPageTemp.c`
2. `ui_ScreenPageInfo.c`
3. `ui_ScreenPageNeedle.c`
4. `ui_ScreenPageOilPressure.c`
5. `ui_ScreenPageBrakeTemp.c`
6. `ui_ScreenPageBrakeWarn.c`
7. `ui_ScreenPageOilWarn.c`
8. `ui_ScreenPageTempCustom.c`
9. `ui_ScreenPageInfoCustom.c`
10. `ui_ScreenPageEasterEgg.c`

处理原则固定为：

1. 有独立功能价值的，允许保留为功能页。
2. 仅服务旧首页刷新的逻辑，要从首页主刷新链路中移除。
3. `my_timerMain()` 不再按旧首页“当前可见页类型”做分支刷新判断。
4. 动态首页成为唯一首页刷新主路径。

当前代码现状补充：

1. 上述旧页面文件已经从 `main/export_path/screens/` 主线移除。
2. `ui.c` / `ui.h` 里对应的历史 screen 指针、控件导出和事件入口也已同步移除。
3. `my_timerMain()` 已不再直接驱动动态首页 tile 刷新，也不再承载旧功能页的数据更新细节；它当前主要只保留顶层时序入口，并转调已经收敛后的 `ui_legacy_runtime.c`。

## 9. 分阶段实施顺序

### 阶段 A：方案冻结

目标：

1. 本文档落库并作为唯一规格。
2. 明确不再改 `dashboard_cfg` schema。
3. 明确保留顶部小 `EDIT`。

完成标准：

1. 文档评审通过。

### 阶段 B：模块拆分

目标：

1. 把动态首页逻辑从 `ui.c` 拆到 `ui_home_runtime.c`
2. 把配置页逻辑从 `ui.c` 拆到 `ui_dashboard_config.c`
3. `ui.c` 只保留页面级路由和少量兼容入口

完成标准：

1. `idf.py build` 通过
2. 静态脚本测试通过
3. 行为与当前分支一致，不引入新交互变化

当前判断：主体已完成。
说明：`ui.c -> ui_home_runtime.c` 与 `ui.c -> ui_dashboard_config.c` 的主拆分已经完成，旧固定页兼容链路也已从主线退场；当前剩余主要是第 10 节定义的真机交互与视觉验收，而不是新的模块边界改造。

### 阶段 C：编辑态收口

目标：

1. 固化左右半区 overlay、删除确认、顶部小 `EDIT`
2. 固化长按进入、退出恢复、滚动锁定
3. 清理旧三按钮语义残留

完成标准：

1. 长按稳定进入编辑态
2. `BACK` 稳定退出
3. `DELETE` 稳定弹确认框
4. `EDIT` 稳定进入配置页并返回

当前判断：主流程已完成，且已有静态契约测试覆盖。

### 阶段 D：旧首页逻辑退场

目标：

1. 从 `my_timerMain()` 移除旧固定首页刷新分支
2. 清理无效缓存变量、无效枚举依赖和历史注释
3. 保留必要兼容入口，不保留死路径

完成标准：

1. 动态首页成为唯一首页刷新入口
2. `my_timerMain()` 不再依赖旧首页页面类型做页面可见性分支
3. 构建和静态测试通过

当前判断：主体已完成。
说明：动态首页自身刷新已从 `my_timerMain()` 移出，旧首页按“当前可见页类型”分支刷新的代码已去掉；旧固定页及其衍生页也已从工程主线删除，对应历史全局指针和页面级刷新逻辑同步收缩。当前剩余的是第 10 节定义的真机交互与视觉验收，而不是新的软件结构改造。

## 10. 验收口径

### 10.1 构建验收

每阶段至少执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_ui_platform_static_tests.ps1
idf.py reconfigure build
idf.py build
```

本次复核的实际结果（2026-07-01）：

1. `powershell -ExecutionPolicy Bypass -File .\tools\run_ui_platform_static_tests.ps1` 已通过。
2. `idf.py -B build_default -D SDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.esp32s3' reconfigure build` 已通过。
3. `idf.py -B build_ws175 -D SDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.defaults.esp32s3;sdkconfig.defaults.ws175' reconfigure build` 已通过。
4. 本次两套构建复跑后，`rs485_brake_temp.c` 相关编译失败已消失，`sdkconfig.defaults` 中 `SOC_SDM_GROUPS` / `CONFIG_NEWLIB_*` / `CONFIG_PERIPH_CTRL_FUNC_IN_IRAM` 等仓库侧迁移告警也已消失。
5. 当前仍可见的提示主要是环境侧项，例如 `ESP_ROM_ELF_DIR` 未设置导致的 gdbinit warning、`idf.py` 启动时的 Python venv 路径提示，以及依赖可升级 notice；这些不阻塞 `.bin` 产出，也不属于本方案实现偏差。

### 10.2 交互验收

必须人工验证：

1. `N = 0`
2. `N = 1`
3. `N = 8`
4. 新增页
5. 删除第一页
6. 删除中间页
7. 删除最后一页
8. 长按进入编辑态
9. 配置页把 `slot_count` 从 `1` 调到 `6`
10. 编辑态下横向滑动被锁定
11. 退出编辑态后横向滑动恢复
12. `ZC6` 下配置页可见 `G-FORCE` 类型
13. 非 `ZC6` 下配置页不可见 `G-FORCE` 类型
14. `METRIC -> G-FORCE -> METRIC` 来回切换时，配置页控件显隐与首页回落正确
15. `G-FORCE` 页返回首页后仍能稳定落回原页
16. `G-force-OBD` 在目标 `ZC6 + ELM327` 组合上应能持续收到有效变化，而不是只刷新首个值
17. `G-force-esp32` 静止时应能回到圆心附近，重新进入页面后 bias 应重新建立

### 10.3 视觉验收

必须确认：

1. 首页和配置页都没有白色环
2. 编辑态左右操作区颜色区分清晰，但底层仪表仍然可见
3. 删除确认框显示正常，不被圆屏裁切异常遮挡
4. `G-FORCE` 页中心坐标区能尽量吃满圆屏，不出现明显大块闲置底部空间
5. `G-FORCE` 页横纵数值字号统一，不出现单边视觉偏重
6. 当前红点阻尼既不能抖，也不能拖尾过重
7. 黄色历史峰值包络在静止后会缓慢回落，不会永久挂住

补充说明：

更细的 `ZC6 G-force` 实机路测步骤、方向校验和故障分流见：

- [ZC6_GFORCE_ROAD_TEST_PLAN.zh-CN.md](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/docs/ZC6_GFORCE_ROAD_TEST_PLAN.zh-CN.md:1)

## 11. 本轮冻结的技术决策

本轮正式冻结以下结论，后续默认不再重复讨论：

1. `dashboard_cfg` struct 大小不改，但允许复用 `page.rsv` 高位扩展 `page_type` 语义。
2. 首页结构固定为 `MENU + N gauge pages + ADD`。
3. `N` 范围固定为 `0..8`。
4. 编辑态固定为“左半区 BACK、右半区 DELETE、顶部小 EDIT”。
5. 配置能力固定为独立 screen。
6. 新增、删除、配置返回后固定整体重建首页。
7. 动态首页运行时已从 `ui.c` 中拆出。
8. 旧固定首页逻辑已退出首页主路径。
9. 仪表页类型从本轮起分为 `METRIC` 与 `G_FORCE` 两类。
10. `G_FORCE` 首版优先只支持 `ZC6`。
11. `page_type` 优先复用 `ui_dashboard_page_cfg_t.rsv` 高位编码，不新增 struct 字段。

## 12. 2026-07-02 G-force 补充

1. `G-force-OBD` 页面激活时不再继续常规 PID 轮询，而是切到专用监听模式。
2. G-force 视觉形态已经固定为圆形十字坐标图，不再回退到双数值卡片。
3. 当前值使用红点表达，并增加轻量防抖与平滑跟随。
4. 历史最大 G 值使用黄色不规则包络线表达，并补一层低透明度描边提升层次。
5. `G-force-esp32` 与 `G-force-OBD` 共用同一套 UI，只区分数据来源。

## 13. G-force 参考图约束落地

这一节专门记录 2026-07-02 用户对参考图风格的约束，避免后续实现再次退回成“通用图表感”。

1. 页面主视觉必须是一个圆形十字坐标轴，而不是两个横纵数值卡片。
2. 四个象限语义固定为：
   - 加速向左
   - 加速向右
   - 减速向左
   - 减速向右
3. 当前 G 值只使用一个小红点表达，不再同时突出两组大数字。
4. 红点需要有防抖与补动画，视觉目标是“有惯性跟随感”，而不是原始值瞬移。
5. 历史最大 G 值需要有一块黄色不规则区域来表达，不能只有一圈单薄描边。
6. 黄色历史区应更接近“历史峰值包络面”的感觉，而不是标准折线图或雷达图。
7. 红点周围允许有轻量红色柔光或很弱的拖尾，但不能喧宾夺主成一条“红线仪表”。
8. 圆盘底应偏深色、十字轴偏浅色、历史区偏黄，层次要像仪表而不是默认图表控件。
9. 实现允许不和参考图逐像素一致，但视觉方向必须优先贴近该参考图，而不是走默认工程风仪表样式。

## 14. 2026-07-02 最新落地约束

这一节用于同步当前代码已经落地、后续实现默认不得回退的行为约束。

1. 首页刷新策略已经固定为“只刷新当前激活页”，不再后台低频刷新相邻页；切页后必须立即刷新新激活页。
2. `No page config` 不是可删除的占位页，而是保留的异常兜底页；该页必须提供 `BACK` 入口，并优先返回来源仪表页，其次回到 `MENU`。
3. `Settings` 已改成两级信息架构：
   - 一级分类固定为 `DISPLAY` / `DASHBOARD` / `VEHICLE`
   - 二级页承载具体 roller、slider 和动作项
4. `BLE Scan` 与 `OBD Protocol` 仍然和 `Settings` 保持首页平级入口，不并入 `Settings` 二级结构。
5. 设置页二级页上滑应先回到设置首页；设置首页上滑才返回首页，避免在深层配置时直接丢失上下文。
6. 动态首页运行时已经从 `ui.c` 中拆出，后续首页行为修改优先落在 `ui_home_runtime.c`；仪表编辑页行为优先落在 `ui_dashboard_config.c`。
7. 后续所有相关回归测试至少要覆盖：
   - active page only 刷新节奏
   - settings 两级导航
   - `No page config` 回退路径
   - 圆屏布局不溢出
