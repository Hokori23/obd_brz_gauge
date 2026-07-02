# 主页横向切页容器替代 `tileview` 技术方案

## 1. 目标

本方案只解决一件事：

1. 用一个自管的主页横向 pager 容器替代当前 `ui_home_runtime.c` 里的 `lv_tileview`

本轮明确不做：

1. 不改 `MENU` 页上滑进 `BLE Scan`、下滑进 `Settings` 的用户感知
2. 不改 `Settings` / `OBD` / `BLE Scan` 自己内部的交互实现
3. 不改仪表编辑态、删除确认、配置页入口的产品语义
4. 不改 `dashboard_cfg`、NVS、数据刷新 cadence、需求驱动采集策略
5. 不做“通用全站容器重构”，只覆盖主页横切

目标不是“换个 LVGL 组件试试”，而是把主页横切从 `tileview/scroll` 路径上拿下来，做一个最小但可控的专用容器。

## 2. 当前代码评审结论

基于当前仓库代码，主页横切性能问题确实集中在 `tileview` 路径：

1. `main/export_path/ui_home_runtime.c` 当前通过 `lv_tileview_create(ui_ScreenPageHome)` 创建主页横切容器
2. 页面切换依赖 `LV_EVENT_SCROLL_BEGIN` / `LV_EVENT_SCROLL_END` / `LV_EVENT_VALUE_CHANGED`
3. 激活页判定依赖 `lv_tileview_get_tile_act(...)`
4. 主动切页依赖 `lv_obj_set_tile_id(...)`
5. `ui_home_perf` 日志已经证明：
   - 当前主页切页稳定出现“两轮整屏 flush”
   - `rot=0` 和 `rot=180` 都存在
   - 问题不是单纯旋转，而是横向 tile/scroll 实现路径本身

这意味着：

1. 如果继续留在 `tileview`
2. 或者只是换成另一种基于 `scroll/snap` 的 LVGL 容器

大概率仍然会落在同一类重绘路径上，不能算真正针对性优化。

## 3. 设计结论

### 3.1 采用方案

主页横切改为：

1. 一个不可滚动的全屏根容器
2. 一个手动平移的横向 `track`
3. `MENU / gauge / ADD` 各页作为 `track` 的普通子对象
4. 通过按压、拖动、释放事件手动计算 `track.x`
5. 通过 `lv_anim` 只做“释放后的短收口动画”

这是一个“自管 pager”，不是 `tileview`，也不是 `tabview`，也不是普通 scroll 容器。

### 3.2 不采用方案

本轮明确不采用：

1. `lv_tabview`
2. 普通 `lv_obj` 横向滚动 + snap
3. 把 `Settings` 的 `tileview` 一起替换
4. 先做跨页面通用 pager 再落主页

原因：

1. `tabview` 和普通 scroll 容器仍然是 scroll 链路，不是这轮要打掉的核心瓶颈
2. 主页和 `Settings` 的约束不同；主页要求最强的 follow-hand 和最小副作用，先隔离主页更稳
3. 当前 `ui_home_runtime.c` 已经很大，再同时推通用化只会扩大 blast radius

## 4. 落地边界

### 4.1 允许改动的文件

本轮只允许改动这些主文件：

1. `main/export_path/ui_home_runtime.c`
2. `main/export_path/ui_home_runtime.h`
3. `main/export_path/CMakeLists.txt`
4. 新增 `main/export_path/ui_home_pager.c`
5. 新增 `main/export_path/ui_home_pager.h`
6. `tests/integration/test_home_navigation_workflow_contract.ps1`
7. 新增一个主页 pager 静态 contract
8. `tools/run_ui_platform_static_tests.ps1`

### 4.2 不应改动的文件

本轮不应为了“顺手统一”去改：

1. `main/export_path/screens/ui_ScreenPageSettings.c`
2. `main/export_path/screens/ui_ScreenPageBLEScan.c`
3. `main/export_path/ui_dashboard_config.c`
4. `main/export_path/ui_round_shell.c`
5. 任何 OBD / NVS / G-force 后端文件

如果中途发现这些文件必须一起动，说明方案边界失控，应停下来复核，不要继续堆改动。

## 5. 最小架构

### 5.1 新模块

新增私有模块：

1. `main/export_path/ui_home_pager.h`
2. `main/export_path/ui_home_pager.c`

职责固定为：

1. 只管理主页横向容器几何、拖动、释放收口和当前页索引
2. 不关心仪表页内容
3. 不关心 NVS
4. 不关心数据刷新
5. 不关心编辑态视觉，只接受“锁定/解锁 pager”的外部命令

### 5.2 `ui_home_runtime.c` 继续负责的内容

保留在 `ui_home_runtime.c`：

1. `s_home_tile_descs`
2. `s_home_tile_runtime`
3. `ui_home_sync_tile_mounts(...)`
4. `ui_home_runtime_refresh_tile(...)`
5. `ui_home_runtime_refresh_visible_tiles(...)`
6. `ui_home_open_menu_overlay(...)`
7. 编辑态进入/退出
8. `ui_home_perf` 统计

换句话说：

1. 页内容和业务状态不动
2. 只把“横向容器”抽掉

## 6. 精确数据结构

`ui_home_pager.h` 采用以下明确边界：

```c
typedef enum {
    UI_HOME_PAGER_AXIS_NONE = 0,
    UI_HOME_PAGER_AXIS_X,
    UI_HOME_PAGER_AXIS_Y
} ui_home_pager_axis_t;

typedef struct {
    uint8_t from_page;
    uint8_t to_page;
    bool changed;
} ui_home_pager_commit_t;

typedef struct ui_home_pager ui_home_pager_t;

typedef void (*ui_home_pager_commit_cb)(const ui_home_pager_commit_t *commit, void *user);
typedef void (*ui_home_pager_vertical_cb)(lv_dir_t dir, uint8_t active_page, void *user);

typedef struct {
    lv_obj_t *parent;
    lv_coord_t width;
    lv_coord_t height;
    uint8_t page_count;
    uint8_t initial_page;
    ui_home_pager_commit_cb commit_cb;
    ui_home_pager_vertical_cb vertical_cb;
    void *user;
} ui_home_pager_config_t;
```

`ui_home_pager_t` 私有 struct 固定包含：

1. `root`
2. `track`
3. `pages[UI_HOME_MAX_TILE_COUNT]`
4. `page_count`
5. `active_page`
6. `target_page`
7. `locked`
8. `settling`
9. `axis`
10. `press_start_x`
11. `press_start_y`
12. `track_start_x`
13. `last_dx`
14. `page_width`
15. `page_height`
16. `lv_anim_t` 对应的收口状态

不再维护任何 `tileview`、`tile id`、`scroll begin/end` 私有状态。

## 7. 公开接口

`ui_home_pager.h` 对外只暴露这些接口：

```c
void ui_home_pager_init(ui_home_pager_t *pager, const ui_home_pager_config_t *cfg);
void ui_home_pager_deinit(ui_home_pager_t *pager);
lv_obj_t *ui_home_pager_root(const ui_home_pager_t *pager);
lv_obj_t *ui_home_pager_page(const ui_home_pager_t *pager, uint8_t page_id);
uint8_t ui_home_pager_active_page(const ui_home_pager_t *pager);
void ui_home_pager_set_active_page(ui_home_pager_t *pager, uint8_t page_id, bool animate);
void ui_home_pager_set_locked(ui_home_pager_t *pager, bool locked);
void ui_home_pager_set_page_count(ui_home_pager_t *pager, uint8_t page_count);
```

约束固定如下：

1. `ui_home_runtime.c` 负责创建页内容
2. pager 只负责给出每页容器对象
3. pager 不直接 mount/unmount 页内容
4. pager 不直接触发数据刷新

## 8. 手势模型

### 8.1 轴锁策略

主页 pager 必须使用明确的轴锁，不允许让横向和纵向语义互相抢。

阈值固定为：

1. `drag_start_threshold = 12px`
2. `axis_bias_threshold = 4px`

判定规则固定为：

1. 初始状态 `axis = NONE`
2. 若 `abs(dx) >= 12` 且 `abs(dx) >= abs(dy) + 4`
   - 锁定为 `X`
3. 若 `active_page == MENU` 且 `abs(dy) >= 12` 且 `abs(dy) >= abs(dx) + 4`
   - 锁定为 `Y`
4. 其他情况继续等待，不提前认定方向

一旦锁定：

1. 本次手势生命周期内不再改方向
2. `X` 锁只驱动横向切页
3. `Y` 锁只允许 `MENU` 页保留现有上/下进二级页语义

### 8.2 横向拖动

当锁定为 `X`：

1. `track.x = clamp(track_start_x + dx, min_x, max_x)`
2. `min_x = -(page_count - 1) * page_width`
3. `max_x = 0`
4. 手指怎么动，`track` 就怎么跟

这是本轮“跟手感”的核心，不允许改成只在 release 后跳页。

### 8.3 释放收口

释放时只允许落到：

1. 当前页
2. 左邻页
3. 右邻页

不允许单次手势跨两页。

收口规则固定为：

1. `progress = -track_x / page_width`
2. `nearest_page = round(progress)`
3. 如果 `abs(dx) < page_width * 0.18`
   - 回弹到 `from_page`
4. 否则：
   - `dx < 0` 目标为 `from_page + 1`
   - `dx > 0` 目标为 `from_page - 1`
5. 最终目标页再做 `clamp(0, page_count - 1)`

收口动画固定为：

1. `140ms`
2. `LV_ANIM_PATH_EASE_OUT`

禁止再走 `lv_obj_set_tile_id(...)`。

### 8.4 纵向手势

纵向语义保持现状，但只保留在主页 pager 的 `MENU` 页：

1. `MENU + 上滑` -> `BLE Scan`
2. `MENU + 下滑` -> `Settings`
3. 非 `MENU` 页忽略纵向切页

触发条件固定为：

1. 本次手势轴锁已判定为 `Y`
2. `active_page == UI_HOME_PAGE_MENU_ID`
3. `abs(dy) >= 20px`
4. `PRESS_LOST/RELEASED` 时触发一次，不允许连续重复触发

## 9. 和编辑态的协作

pager 与编辑态的约束必须写死：

1. 进入编辑态时，调用 `ui_home_pager_set_locked(..., true)`
2. 退出编辑态时，调用 `ui_home_pager_set_locked(..., false)`
3. `locked = true` 时：
   - 不允许横向拖动
   - 不允许纵向菜单跳转
   - 不允许启动新的收口动画

这部分不再通过“清掉 `tileview` 的 scrollable flag”实现。

## 10. 页面创建与挂载策略

### 10.1 页容器

每个主页页容器改为普通 `lv_obj`：

1. 父对象是 `pager.track`
2. 宽高固定等于整屏
3. `x = page_id * page_width`
4. `y = 0`
5. 不可滚动
6. 无边框、无 padding、透明背景

### 10.2 mount 策略

本轮继续保留当前策略，不扩 scope：

1. 激活页常驻
2. 左右相邻页预挂载
3. 其他页卸载或隐藏

对应现有代码：

1. `ui_home_sync_tile_mounts(...)` 保留
2. `ui_home_runtime_refresh_visible_tiles(...)` 保留

即：

1. 换容器
2. 不换 mount 策略

## 11. `ui_home_perf` 保留方式

现有 perf 日志不能丢，但触发点要改。

新的统计边界固定为：

1. 第一次锁定为横向 `X` 时
   - `ui_home_page_switch_perf_begin(from_page)`
2. 释放并决定 `to_page` 时
   - `ui_home_page_switch_perf_commit(to_page)`
3. 收口动画完成后
   - 启动与现状一致的 settle timer
4. settle timer 到期
   - `ui_home_page_switch_perf_finish(...)`

日志 tag 继续使用：

1. `ui_home_perf`

日志字段尽量保持不变，这样历史对比不需要改分析口径。

## 12. 代码改造清单

### 12.1 `ui_home_runtime.c`

必须做的改动：

1. 删除 `s_home_tileview`
2. 新增 `static ui_home_pager_t s_home_pager`
3. 删除这些 `tileview` 专属函数：
   - `ui_home_tileview_value_changed`
   - `ui_home_tileview_gesture`
   - `ui_home_page_switch_scroll_begin`
   - `ui_home_page_switch_scroll_end`
   - `ui_home_create_tile`
4. 新增 pager 回调桥接：
   - `ui_home_pager_commit_cb(...)`
   - `ui_home_pager_vertical_cb(...)`
5. `ui_home_set_active_page(...)` 改为走 `ui_home_pager_set_active_page(...)`
6. `ui_home_runtime_screen_init()` 里改为初始化 pager 并逐页创建普通 page object
7. `ui_home_enter_edit_mode()` / `ui_home_exit_edit_mode()` 改为显式锁定/解锁 pager

### 12.2 `ui_home_runtime.h`

原则上不新增对外 API。

如果实现过程发现必须向外暴露新接口，说明方案边界有问题，应优先回到 `ui_home_runtime.c` 私有化解决。

### 12.3 `ui_home_pager.c/.h`

实现内容固定为：

1. 创建 `root`
2. 创建 `track`
3. 分页容器定位
4. 处理 `PRESSED / PRESSING / RELEASED / PRESS_LOST`
5. 处理 `track` 收口动画
6. 对外回调 `commit_cb`
7. 对外回调 `vertical_cb`

禁止在这里写：

1. NVS 逻辑
2. 仪表页刷新
3. 编辑态 overlay UI
4. BLE / Settings 路由细节

### 12.4 `main/export_path/CMakeLists.txt`

新增：

1. `ui_home_pager.c`

其他 source 不应顺手调整。

## 13. 测试与 contract

### 13.1 需要更新的现有 contract

`tests/integration/test_home_navigation_workflow_contract.ps1` 改为检查：

1. 主页不再调用 `lv_tileview_create(ui_ScreenPageHome)`
2. 主页仍保留 `ui_home_open_menu_overlay(...)`
3. `BLE Scan` 返回仍回到 `ui_home_runtime_show_page(UI_HOME_PAGE_MENU_ID, ...)`
4. `Settings` 返回仍回到 `ui_home_runtime_show_page(UI_HOME_PAGE_MENU_ID, ...)`

### 13.2 新增 contract

新增：

1. `tests/dashboard/test_ui_home_pager_contract.ps1`

至少检查：

1. `ui_home_runtime.c` 不再使用 `lv_tileview_create`
2. `ui_home_runtime.c` 不再使用 `lv_tileview_get_tile_act`
3. `ui_home_runtime.c` 不再使用 `lv_obj_set_tile_id`
4. `ui_home_runtime.c` 仍保留 `ui_home_sync_tile_mounts`
5. `ui_home_runtime.c` 仍保留 `ui_home_runtime_refresh_visible_tiles`
6. `ui_home_runtime.c` 仍保留 `ui_home_open_menu_overlay`
7. `ui_home_pager.c` 存在横向轴锁和释放收口逻辑

### 13.3 静态测试入口

`tools/run_ui_platform_static_tests.ps1` 需要新增调用：

1. `tests/dashboard/test_ui_home_pager_contract.ps1`

## 14. 实施顺序

顺序固定如下：

1. 新增 `ui_home_pager.c/.h`
2. 接入 `CMakeLists.txt`
3. 在 `ui_home_runtime.c` 中替掉 `tileview` 容器
4. 保持原 `ui_home_sync_tile_mounts` / refresh 逻辑不动
5. 接回编辑态锁定
6. 接回菜单页上下跳转
7. 更新 contract
8. 跑静态测试
9. 跑 `idf.py build`

不允许“先删 tileview 再临时用硬切页顶着”，必须一轮内把 follow-hand pager 接完整。

## 15. 验收标准

### 15.1 行为

必须全部满足：

1. 主页左右滑动仍然跟手
2. `MENU` 页上滑仍进 `BLE Scan`
3. `MENU` 页下滑仍进 `Settings`
4. 非 `MENU` 页不响应上下切页
5. 长按仪表页进入编辑态后，主页横切被锁定
6. 退出编辑态后，主页横切恢复
7. `ADD` 页和普通仪表页都能正常横切到相邻页

### 15.2 性能

这轮不强行承诺“数值一定下降到多少”，但必须满足：

1. `ui_home_perf` 继续可用
2. 不允许比当前 `tileview` 版本更卡
3. 真机主观手感不允许比现在更不跟手

### 15.3 工程

必须全部满足：

1. `tools/run_ui_platform_static_tests.ps1` 通过
2. `idf.py build` 通过
3. 主页路径不再依赖 `lv_tileview`
4. `Settings` / `BLE Scan` / `OBD` 的现有交互 contract 不回归

## 16. 评审结论

这套方案的优点是：

1. scope 小，只打主页横切
2. 直接绕开 `tileview/scroll` 路径
3. 保留现有 mount、refresh、编辑态和 sibling screen 路由
4. 代码边界清楚，适合后续继续做主页横切性能压缩

这套方案的主要风险是：

1. 自管手势会比 `tileview` 多一层输入状态机
2. 如果轴锁写得含糊，会把纵向菜单入口和长按编辑态重新搅乱

因此本方案已经把阈值、状态机、文件边界和测试口径写死；实现时不应再临场发挥出第二套交互模型。
