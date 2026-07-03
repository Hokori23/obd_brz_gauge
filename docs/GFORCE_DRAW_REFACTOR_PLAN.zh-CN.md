# G-force 页面绘制链路重构方案

更新时间：`2026-07-03`

## 1. 目标

将首页 `G-force-OBD` / `G-force-esp32` 页面从“多 LVGL 子对象高频更新”重构为“单绘制面自绘 + 纯逻辑状态推进 + invalidate 刷新”。

本方案只做第一阶段可上板版本，目标明确为：

1. 保留圆盘、十字轴、当前 G 点、当前向量、中心点、轻量历史包络这些核心语义。
2. 去掉高频阶段对多个 `lv_obj_align` / `lv_line_set_points` / 样式切换的依赖。
3. 将高频逻辑抽成可 host 编译的纯 C helper，并纳入静态测试。
4. 保证 `idf.py build` 可通过，不把编译风险留到后续。

## 2. 当前问题

当前实现位于 [ui_home_runtime.c](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_home_runtime.c:1103)。

现状特征：

1. `G-force` 由多个对象拼接：
   - `gforce_outer_ring`
   - `gforce_mid_ring`
   - `gforce_inner_ring`
   - `gforce_axis_h`
   - `gforce_axis_v`
   - `gforce_vector_line`
   - `gforce_dot_glow`
   - `gforce_dot`
   - `gforce_center_dot`
2. 高频刷新路径在 [ui_home_runtime.c](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_home_runtime.c:1270) 内直接改对象位置和点集。
3. 每次刷新都涉及对象级 layout/style 更新，和 `GEAR` 转速环的单对象自绘路径不一致。
4. 历史数据结构已经预留，但没有形成真正统一的绘制管线，代码复杂度和收益不匹配。

## 3. 目标架构

### 3.1 分层

重构后页面只保留一个核心绘制对象：

1. `rt->gforce_plot`
   - 透明背景
   - 注册 `LV_EVENT_DRAW_MAIN`
   - 承载整张 G-force 图

页面高频刷新只做两步：

1. 调用纯逻辑 helper 推进显示态
2. `lv_obj_invalidate(rt->gforce_plot)`

### 3.2 纯逻辑边界

新增纯 C helper：

- `main/app_obd_dsp/ui_gforce_plot_logic.h`
- `main/app_obd_dsp/ui_gforce_plot_logic.c`

职责：

1. 输入原始 `lat_g / lon_g`
2. 做 clamp、deadzone、分档 follow
3. 维护 `display_lat_g / display_lon_g`
4. 维护按角度离散的历史半径数组

这层不依赖 LVGL，可直接被 `tests/dashboard` host 编译测试。

### 3.3 绘制内容

`LV_EVENT_DRAW_MAIN` 内一次性绘制：

1. 外环 / 中环 / 内环
2. 水平 / 垂直十字轴
3. 黄色历史包络线
4. 当前 G 向量线
5. 当前 G 点 glow
6. 当前 G 点
7. 中心点

不再创建这些元素各自的 `lv_obj`。

## 4. 为什么这样做

### 4.1 直接收益

1. 高频阶段从“改多个对象”变成“改一份状态”。
2. 减少对象树深度和布局参与者。
3. 性能模型与 `GEAR` 转速环统一，后续维护成本更低。
4. 可测试边界更清晰，减少“只能上板看”的盲区。

### 4.2 不做的事

第一阶段明确不做：

1. 不做复杂多边形填充历史面。
2. 不恢复象限文案。
3. 不引入新的交互或配置项。
4. 不把 `G-force` 退回成数字卡片布局。

原因：

1. 当前主要问题是绘制链路过碎，不是语义不够多。
2. 用户之前已明确不需要 `ACC LEFT` 这类文案。
3. 历史面先用轻量包络线表达，优先保证流畅度。

## 5. 详细实现步骤

### 5.1 纯逻辑 helper

新增 `ui_gforce_plot_state_t`：

1. `display_lat_g`
2. `display_lon_g`
3. `history_radius[48]`

新增函数：

1. `ui_gforce_plot_reset()`
2. `ui_gforce_plot_step()`
3. `ui_gforce_plot_angle_to_bin()`

关键规则锁定：

1. `max_g = 1.20`
2. `deadzone = 0.015`
3. `follow`
   - 默认 `0.14`
   - 中幅变化 `0.24`
   - 大幅变化 `0.34`
4. 历史半径每次推进都会衰减
5. 当前方向对应 bin 直接抬高，邻近 bin 轻度抬高，避免锯齿

### 5.2 UI runtime

调整 `ui_home_tile_runtime_t`：

1. 删除或停用旧 `gforce_*` 子对象字段
2. 保留：
   - `gforce_plot`
   - `gforce_plot_size`
   - `gforce_plot_radius`
   - `ui_gforce_plot_state_t gforce_plot_state`

新增：

1. `ui_home_gforce_draw_event()`
2. 轻量绘制 helper

### 5.3 刷新路径

`ui_home_refresh_gforce_tile()` 保持数据来源不变：

1. `G-force-esp32` 读 IMU 缓存
2. `G-force-OBD` 读 OBD 缓存

但不再直接改多个对象，而是：

1. 调用 `ui_home_gforce_update_plot()`
2. 后者内部调用 `ui_gforce_plot_step()`
3. 最后 `invalidate`

## 6. 测试与回归

### 6.1 新增 host test

新增：

- `tests/dashboard/test_ui_gforce_plot_logic.c`

覆盖：

1. reset 后状态为零
2. 输入被 clamp
3. display 值按 follow 逐步逼近目标
4. history bin 会被写入
5. history 会衰减

### 6.2 更新合同测试

更新 `tests/dashboard/test_gforce_ui_contract.ps1`，要求：

1. `ui_home_runtime.c` 存在 `ui_home_gforce_draw_event`
2. `gforce_plot` 注册 `LV_EVENT_DRAW_MAIN`
3. runtime 刷新路径调用 `ui_gforce_plot_step`
4. 页面仍保持透明背景
5. 不恢复旧象限说明文字

## 7. 上板验证口径

本轮开发完成后的板端验证按下面执行：

1. 切到 `G-force-OBD`
2. 确认页面不再出现明显卡顿或拖影恶化
3. 缓慢左右/前后变换姿态，确认红点跟随连续
4. 确认中心点固定在圆心
5. 确认历史黄色包络会随时间回落
6. 切回其他仪表页，确认页面切换和 demand-driven 逻辑未回退

## 8. 完成判定

这一阶段完成的判定标准：

1. 方案文档已入仓库
2. `G-force` 页面改为单 draw obj 自绘
3. 纯逻辑 helper 已存在且被 runtime 复用
4. host 静态测试通过
5. `idf.py build` 通过

