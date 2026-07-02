# Settings / OBD 统一交互重构 Review

## 1. Findings

### 1.1 高优先级：`Settings` 当前实现存在手势语义冲突

文件：

1. [main/export_path/screens/ui_ScreenPageSettings.c](/D:/Program Files (x86)/Code Projects/obd_brz_gauge/main/export_path/screens/ui_ScreenPageSettings.c)

问题：

1. `on_settings_background()` 把 `上滑` 定义为返回动作。
2. `ui_settings_prepare_content_root()` 又把 `s_settings_content` 设成了 `LV_DIR_VER` 页面级纵向滚动容器。
3. 这会导致同一个纵向手势同时承担“滚内容”和“返回”两种语义。

结论：

1. 这是当前最需要消除的交互结构性问题。
2. 该问题不能通过文案提示解决，只能通过重新定义页面结构解决。

### 1.2 高优先级：`Settings` 与 `Home` 的页面组织方式不一致

文件：

1. [main/export_path/ui_home_runtime.c](/D:/Program Files (x86)/Code Projects/obd_brz_gauge/main/export_path/ui_home_runtime.c)
2. [main/export_path/screens/ui_ScreenPageSettings.c](/D:/Program Files (x86)/Code Projects/obd_brz_gauge/main/export_path/screens/ui_ScreenPageSettings.c)

问题：

1. `Home` 是横向同级页模型。
2. `Settings` 是“根页卡片 -> 详情页”二级模型。
3. 用户从主页进入设置后，系统的空间逻辑突然切换，连续性差。

结论：

1. `Settings` 不需要和 `Home` 共用完全相同的底层业务代码。
2. 但必须在用户感知上统一为“同级横向页 + 统一返回方向”。

### 1.3 高优先级：独立 OBD 页面属于交互孤岛

文件：

1. [main/export_path/ui.c](/D:/Program Files (x86)/Code Projects/obd_brz_gauge/main/export_path/ui.c)
2. [main/export_path/screens/ui_ScreenPageODBProtocal.c](/D:/Program Files (x86)/Code Projects/obd_brz_gauge/main/export_path/screens/ui_ScreenPageODBProtocal.c)

问题：

1. `OBD Protocol` 不是从 `Settings` 体系进入。
2. 它有自己独立的返回和保存行为。
3. Logo 页双击进入该页面的入口与正式产品 IA 不一致。

结论：

1. `OBD Protocol` 不应继续作为独立 screen 保留在主流程中。
2. 它应当成为 `Settings` 的一个分类页。

### 1.4 高优先级：`OBD Protocol` 当前使用隐式长按保存模型，不可发现

文件：

1. [main/export_path/ui.c](/D:/Program Files (x86)/Code Projects/obd_brz_gauge/main/export_path/ui.c)

问题：

1. `ui_event_obd_prot_background()` 通过 `LV_EVENT_LONG_PRESSED_REPEAT` 累积时间。
2. 达到阈值后才写入 `cfg.protocol` 并 `esp_restart()`。
3. 用户若不知道这套规则，就无法可靠完成设置。

结论：

1. 该行为必须替换为明确确认 modal。
2. 所有高影响设置项都应该采用“确认后执行”的同一模型。

### 1.5 中优先级：`Settings` 现在已经开始承担过多例外布局

文件：

1. [main/export_path/screens/ui_ScreenPageSettings.c](/D:/Program Files (x86)/Code Projects/obd_brz_gauge/main/export_path/screens/ui_ScreenPageSettings.c)

问题：

1. 当前页面同时包含分类卡片、详情页面板、弹窗提示、纵向滚动、返回层级切换。
2. 继续在这个模型上叠加 `OBD`，复杂度会继续上升。

结论：

1. 现有结构适合收口，不适合继续扩展。
2. 本轮应该借 `OBD` 并入的机会，把它改造成更规整的横向分类结构。

### 1.6 中优先级：`Settings` 返回目标与页面层级耦合过深

文件：

1. [main/export_path/screens/ui_ScreenPageSettings.c](/D:/Program Files (x86)/Code Projects/obd_brz_gauge/main/export_path/screens/ui_ScreenPageSettings.c)

问题：

1. 当前 `Back` 和 `上滑` 都要先判断 `s_settings_section == ROOT` 还是 detail。
2. 这让返回语义依赖于临时内部状态，而不是稳定的空间规则。

结论：

1. 改成同级横向页后，返回规则可以简化为“上滑离开 Settings”。
2. 这样实现和体验都会更稳定。

## 2. Open Questions

本轮方案不保留开放问题。以下点已强制定案：

1. `OBD` 并入 `Settings`。
2. `Settings` 不再使用页面级纵向滚动。
3. `Protocol` 改为确认后重启。
4. 不保留说明性交互提示小字。

## 3. 实施约束

### 3.1 不允许的中间态

1. 删除独立 OBD 页后，却暂时没有新的 `Settings -> OBD` 入口。
2. 继续保留 `Protocol` 的长按保存逻辑，同时再加确认 modal。
3. `Settings` 既保留纵向滚动，又引入横向分类切换。
4. 通过提示文案弥补交互结构问题。

### 3.2 允许的技术选择

1. `Settings` 内部使用 `lv_tileview`。
2. `Settings` 内部使用自建横向容器加手势切页。
3. 将 `Settings` 运行态逻辑拆到单独 `.c/.h` 文件。

唯一要求是：

1. 用户感知必须满足目标交互模型。
2. 实现后不得再出现手势语义冲突。

## 4. Change Summary

建议实施路径如下：

1. 先重写 `Settings` 的页面组织方式。
2. 再迁入 `OBD Protocol` 设置。
3. 最后移除独立 `OBD Protocol` screen 和 logo 双击入口。

这是风险最低的顺序，因为它保证任何时刻都只有一套有效配置路径。
## 2026-07-03 Follow-up

This review is now also a durable home for the reasoning that was first
collected in a temporary worktree note and has since been folded into the
long-lived topic docs.

Key confirmed outcomes:

- `Settings` and `OBD` should be treated as one unified interaction system.
- The old standalone OBD protocol screen is not part of the accepted long-term IA.
- Modal consistency is part of the UX review, not a cosmetic afterthought.
- The round-screen layout review must include touch target size, clipping, and page-title consistency.

Key remaining performance finding:

- The current home horizontal switch path still produces two full-screen redraw rounds per page switch.
- This finding is independent of display rotation mode.
- Therefore, future optimization should target the tile/scroll implementation path before revisiting per-page visual polish.
