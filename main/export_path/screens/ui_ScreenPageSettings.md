# `ui_ScreenPageSettings.c` 阅读说明

这是当前统一设置页的实现说明。它已经不再是旧的“根分类页 + 二级详情页”结构，而是一个横向同级分页的圆屏运行时页面。

## 当前结构

设置页内部固定为四个同级分类页：

- `DISPLAY`
- `DASHBOARD`
- `VEHICLE`
- `OBD`

每个分类页都在一屏内完成操作，不依赖页面级纵向滚动。

### `DISPLAY`

- 亮度
- 旋转（仅 `WS175`）

### `DASHBOARD`

- `BOOT PAGE`
- `OBD POLL`
- `RACECHRONO BLE`
- `OIL PRESSURE`

### `VEHICLE`

- `VEHICLE PROFILE`

### `OBD`

- `PROTOCOL`

## 为什么这样改

主要是为了解决四个问题：

1. 旧设置页在圆屏上信息密度过高，选择器和触控区不够友好
2. 页面级纵向滚动会和 `上滑返回` 抢手势
3. 独立 `OBD Protocol` 页让交互模型出现例外，用户需要额外记忆
4. 不同页面自己写一套圆屏安全区逻辑，代码维护成本高

## 关键交互

### 页面切换

- 从首页下滑进入 `Settings`
- 在 `Settings` 内左右滑切换 `DISPLAY / DASHBOARD / VEHICLE / OBD`
- 在任一分类页上滑直接返回首页
- `HOME` 按钮和上滑返回语义一致

相关逻辑：

- `ui_settings_tileview_value_changed()`
- `ui_settings_gesture_event()`
- `ui_settings_close_to_home()`

### 亮度

`on_bright_slider_change()`

- 更新 `cfg.brightness_day`
- `nvs_cfg_set()`
- `board_set_brightness()`

这是“持久化 + 立即生效”的典型设置项。

### 启动页

`on_page_roller_change()`

- 更新 `cfg.default_page`
- `nvs_cfg_set()`

当前语义已经是动态首页：

- `0` = `MENU`
- `1..8` = `GAUGE 1..8`

### 车型切换

`on_vehicle_roller_change()`

- 更新 `cfg.vehicle_profile_idx`
- `nvs_cfg_set()`
- `vehicle_profile_set_active()`
- 标记 `s_settings_requires_home_rebuild = true`

这意味着车型切换不仅是 UI 配置，还会影响首页仪表支持项和后续运行时逻辑。

### OBD 协议

`on_protocol_roller_released()`

- 仅在候选协议和当前持久化协议不一致时弹确认框
- 确认后写入 `cfg.protocol` 并 `esp_restart()`
- 取消后把 `roller` 恢复到当前持久化值

这里已经不再使用旧的“长按保存协议”模型。

## 布局实现重点

这个页面现在优先依赖统一的圆屏运行时 helper，而不是各自写一套几何逻辑：

1. `ui_round_shell_safe_span_for_band()`
2. `ui_round_shell_create_header_button()`
3. `ui_round_shell_create_title_block()`
4. `ui_round_shell_apply_dark_roller_preset()`
5. `ui_round_shell_apply_modal_theme()`

所以如果后面继续扩展这页，优先沿用这些共享 helper，不要重新回到“写死 y 坐标堆控件”或“再加一层二级设置页”的方式。
