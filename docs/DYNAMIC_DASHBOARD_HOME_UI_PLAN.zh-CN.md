# 动态仪表首页技术方案

## 1. 目标

本文档定义 WS175 圆屏首页的最终信息架构、交互规则、数据模型、代码边界与实施顺序。目标是把当前已经跑通的动态首页基线，收敛为一套后续可持续迭代的实现方案。

本文档是该功能的唯一实现规格。后续涉及首页重构、编辑态、仪表页新增/删除、配置页改造时，以本文档为准。

## 2. 评审结论

### 2.1 当前实现里已经成立的部分

以下能力已经在当前分支存在，方向正确，不需要推翻：

1. 首页结构已经从固定 6 页改成 `MENU + N 个仪表页 + ADD`。
2. `N` 的持久化已经走 `dashboard_cfg`，支持 `0..8` 页。
3. 单个仪表页已经支持 `1..6` 个 slot。
4. 新增页、删除页、配置页返回后，首页会整体重建并重新落到目标页。
5. `ui_round_shell_create_ring()` 已被改成透明，不再主动绘制白色环。
6. 长按当前仪表页进入编辑态、删除确认、配置页 roller 编辑，这条主流程已经可用。

### 2.2 当前实现和目标方案的差距

以下三点仍然需要继续收口：

1. 动态首页运行时、配置页、历史兼容逻辑仍然主要堆在 [ui.c](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui.c)。
2. 编辑态虽然已经接近目标形态，但还缺少正式的模块边界和验收口径，后续继续往 `ui.c` 里补功能风险很高。
3. 旧固定首页残留逻辑仍在 `my_timerMain()` 和若干老页面文件周围存在兼容痕迹，需要按阶段清理，而不是继续长期共存。

### 2.3 最终判断

结论不是“重做”，而是“收口”：

1. 现有 `dashboard_cfg` 数据模型保留。
2. 现有 `MENU + N + ADD` 首页结构保留。
3. 现有“整体重建首页”策略保留。
4. 后续工作重点转为模块拆分、交互细节固化、历史逻辑下线。

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
3. 提供进入 `BLE Scan`、`Settings` 等二级页入口。
4. 不参与编辑态。

可编辑仪表页职责：

1. 仅展示仪表数据卡片。
2. 接受长按进入编辑态。
3. 接受顶部 `EDIT` 进入配置页。
4. 支持删除当前页。

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

当前持久化模型已经满足需求，继续沿用：

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

定义位置：

1. [nvs_storage.h](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/bsp_obd_dsp/nvs_storage.h:16)
2. [nvs_storage.c](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/bsp_obd_dsp/nvs_storage.c:219)

### 4.2 固定约束

1. `gauge_page_count` 只能是 `0..8`。
2. `slot_count` 只能是 `1..6`。
3. 不允许存在“空仪表页”，删除最后一个 slot 的语义等同于删除整页。
4. 新建页默认值固定为：
   `slot_count = 1`，`slot_items[0] = DISP_ITEM_RPM`
5. 历史数据迁移默认页固定为：
   `RPM / SPEED / OIL`

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

### 6.2 固定行为

1. 标题固定为 `DASHBOARD`
2. `slot_count` roller 范围固定为 `1..6`
3. 每个 slot 一组 item roller
4. 当 `slot_count` 变小时，仅隐藏超出行，不强制清空缓存值
5. 当 `slot_count` 变大时，新暴露 slot 默认值写成 `RPM`
6. 上滑返回当前仪表页
7. roller 修改即时写入 NVS，不新增单独 Save 按钮

### 6.3 与首页的数据同步策略

配置页关闭后，不做局部热更新，固定走：

1. 重新读取 NVS
2. 重建首页 tile 描述和挂载状态
3. 回到刚才编辑的仪表页

这条策略已经被验证可用，继续保留。

## 7. 代码落地边界

### 7.1 模块划分

后续不再把动态首页相关能力继续堆回 `ui.c`。固定拆成三层：

1. `ui.c`
   职责：全局页面路由、logo 后首屏进入、旧入口兼容转发
2. `ui_home_runtime.c`
   职责：动态首页 tile 描述、页面挂载、编辑态、首页刷新
3. `ui_dashboard_config.c`
   职责：仪表页配置 screen

对应头文件：

1. [ui_home_runtime.h](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_home_runtime.h:1)
2. 新增 `main/export_path/ui_dashboard_config.h`

### 7.2 对外接口

首页运行时稳定接口固定为：

```c
void ui_home_runtime_show_page(uint8_t page_id, lv_scr_load_anim_t anim);
void ui_home_runtime_refresh_active_tile(void);
uint8_t ui_home_runtime_page_from_default(uint8_t default_page);
```

后续允许补充但不允许反向扩散回 `ui.h` 的接口：

```c
void ui_home_runtime_rebuild_and_load(uint8_t page_id, lv_scr_load_anim_t anim);
void ui_dashboard_config_open(uint8_t gauge_index);
```

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

## 8. 历史固定页的处理原则

以下页面不再是首页主路径的一部分：

1. `ui_ScreenPageTemp.c`
2. `ui_ScreenPageInfo.c`
3. `ui_ScreenPageNeedle.c`
4. `ui_ScreenPageOilPressure.c`
5. `ui_ScreenPageBrakeTemp.c`
6. `ui_ScreenPageEasterEgg.c`

处理原则固定为：

1. 有独立功能价值的，允许保留为功能页。
2. 仅服务旧首页刷新的逻辑，要从首页主刷新链路中移除。
3. `my_timerMain()` 不再感知旧首页页面类型。
4. 动态首页成为唯一首页刷新主路径。

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

### 阶段 D：旧首页逻辑退场

目标：

1. 从 `my_timerMain()` 移除旧固定首页刷新分支
2. 清理无效缓存变量、无效枚举依赖和历史注释
3. 保留必要兼容入口，不保留死路径

完成标准：

1. 动态首页成为唯一首页刷新入口
2. `my_timerMain()` 不再依赖旧首页页面类型
3. 构建和静态测试通过

## 10. 验收口径

### 10.1 构建验收

每阶段至少执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_ui_platform_static_tests.ps1
idf.py reconfigure build
idf.py build
```

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

### 10.3 视觉验收

必须确认：

1. 首页和配置页都没有白色环
2. 编辑态左右操作区颜色区分清晰，但底层仪表仍然可见
3. 删除确认框显示正常，不被圆屏裁切异常遮挡

## 11. 本轮冻结的技术决策

本轮正式冻结以下结论，后续默认不再重复讨论：

1. `dashboard_cfg` schema 不改。
2. 首页结构固定为 `MENU + N gauge pages + ADD`。
3. `N` 范围固定为 `0..8`。
4. 编辑态固定为“左半区 BACK、右半区 DELETE、顶部小 EDIT”。
5. 配置能力固定为独立 screen。
6. 新增、删除、配置返回后固定整体重建首页。
7. 动态首页运行时要从 `ui.c` 中拆出。
8. 旧固定首页逻辑最终要退出首页主路径。
