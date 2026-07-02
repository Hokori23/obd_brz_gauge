# `ui_ScreenPageSettings.c` 阅读说明

这是当前设置页实现说明。它已经不是“把所有设置项挤在一页里”的旧结构，而是一个两级设置页。

## 当前结构

一级页只负责分类导航：

- `DISPLAY`
- `DASHBOARD`
- `VEHICLE`

二级页才展示具体配置控件：

- `DISPLAY`
  - 亮度
  - 旋转（WS175）
- `DASHBOARD`
  - `BOOT PAGE`
  - `OBD POLL`
- `VEHICLE`
  - 车型选择

## 为什么这样改

主要是为了解决三个问题：

1. 单页密度太高，圆屏上操作拥挤
2. 后续还会继续加配置项，单页继续堆会很难维护
3. 两级结构更容易做圆屏安全区布局，不容易溢出

## 关键交互

### 一级页进入二级页

分类卡片点击后，会切换当前 section，并重建内容区。

相关逻辑：

- `ui_settings_open_section()`
- `ui_settings_show_section()`

### 返回逻辑

- 在二级页点击 `BACK`：返回一级分类页
- 在二级页上滑：也先返回一级分类页
- 在一级分类页上滑：退出设置页并返回首页

相关逻辑：

- `ui_settings_back_click()`
- `on_settings_background()`
- `ui_settings_close_to_home()`

### 亮度

`on_bright_slider_change()`

- 更新 `cfg.brightness_day`
- `nvs_cfg_set()`
- `board_set_brightness()`

这是“持久化 + 立即生效”的典型例子。

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

这意味着车型切换不仅是 UI 设置，还会影响首页仪表支持项和后续运行时逻辑。

## 布局实现重点

这个页面现在不再依赖旧的绝对行表，而是：

1. 先根据圆屏计算安全内容带
2. 在内容带里放分类卡片或详情 panel
3. 用 flex column 做纵向排布

所以如果后面再继续扩展这页，优先沿用：

- `ui_settings_safe_span_for_band()`
- `ui_settings_create_content_panel()`
- `ui_settings_create_category_card()`

不要重新回到“写死 y 坐标堆控件”的方式。
