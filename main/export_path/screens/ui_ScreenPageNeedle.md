# `ui_ScreenPageNeedle.c` 阅读说明

这是指针仪表页，也是 UI 层里比较有“仪表盘味道”的页面。

## 页面职责

1. 用 `lv_meter` 画 270 度指针表盘
2. 在中间显示当前值和单位
3. 支持选择不同数据源作为指针输入

## 为什么它值得读

它体现了 UI 不是只读固定字段，而是“一个页面可配置绑定不同数据源”。

## 页面结构

- `ui_ScreenPageNeedle`：主表盘页
- `ui_ScreenPageNeedleConfig`：数据源选择页

## 关键逻辑

### `build_needle_sources()`

根据车型是否支持增压，动态决定数据源列表里要不要出现 `BOOST`。

这说明 UI 也会消费车型配置，不只是 OBD 层会用。

### `on_needle_source_changed()`

把用户选择的数据源写入 `NVS`，并立即调用 `ui_needle_apply_source()` 重建表盘配置。

### `ui_needle_apply_source()`

虽然实现在别处，但从这里可以知道：

- 表盘量程
- 名称
- 单位

都不是固定的，而是由当前数据源驱动。

## 你可以怎么理解

这更像一个“可配置数据绑定的组件页”，而不是写死的传统仪表页。
