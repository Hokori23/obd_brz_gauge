# `ui_ScreenPageSettings.c` 阅读说明

这是设置页，也是一个很值得读的页面。

## 它管理哪些设置

- 默认启动页
- 当前车型
- 屏幕亮度

## 这个页面为什么重要

它把三个层连起来了：

- UI 控件：roller / slider
- 持久化：`nvs_storage`
- 运行时效果：亮度立即生效、车型切换立即生效

## 关键交互

### 启动页切换

`on_page_roller_change()`
-> 更新 `cfg.default_page`
-> `nvs_cfg_set()`

### 亮度切换

`on_bright_slider_change()`
-> 更新 `cfg.brightness_day`
-> `nvs_cfg_set()`
-> `Set_Backlight()`

这是“持久化 + 立即生效”的典型例子。

### 车型切换

`on_vehicle_roller_change()`
-> 更新 `cfg.vehicle_profile_idx`
-> `nvs_cfg_set()`
-> `vehicle_profile_set_active()`

这说明车型不是单纯 UI 配置，而是会影响后续运行逻辑。

## 你读这页时要注意

- 页面展示项是静态 UI
- 真正关键的是每个控件改值后触发了哪些系统行为
