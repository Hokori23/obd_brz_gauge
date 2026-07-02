# `export_path`

这是 UI 相关目录。虽然它保留了明显的 `SquareLine Studio + LVGL` 导出痕迹，但现在已经不只是“纯生成物”，而是承载了项目当前主 UI 运行时逻辑的目录。

## 目录

```text
export_path/
|-- ui.c / ui.h             顶层 UI 入口和少量全局 screen 状态
|-- ui_helpers.*            UI 辅助函数
|-- ui_layout.h             圆屏布局常量与结构体
|-- ui_platform.*           基于真实屏幕尺寸的布局缩放
|-- ui_home_runtime.*       动态首页运行时
|-- ui_dashboard_config.*   仪表编辑页
|-- screens/                各二级页与具体 screen 实现
|-- images/                 图片资源转成的 C 数组
|-- fonts/                  字体资源转成的 C 数组
|-- components/             UI 组件钩子
`-- assets/                 额外导出资源
```

## 目前最重要的几个文件

- `ui_home_runtime.c`
  当前首页主运行时。负责 tile 构建、激活页切换、编辑态、以及只刷新当前页的首页刷新策略。
- `ui_dashboard_config.c`
  仪表编辑页。负责页类型、slot 数量和指标项编辑；异常情况下会显示 `No page config`，但必须允许返回。
- `screens/ui_ScreenPageSettings.c`
  当前设置页实现。现在已经是两级结构，而不是把全部配置塞在一页里。
- `ui_layout.h` / `ui_platform.c`
  负责把 WS175 等圆屏设备上的布局约束做成统一缩放与安全区计算。

## 当前 UI 和业务层的连接方式

不是传统 MVC，也不是前端状态库风格，而是：

1. `ui_init()` / 各 screen init 创建 LVGL 控件
2. 页面事件或定时刷新逻辑直接读取 `obd_data_get_*()` / `nvs_cfg_get()`
3. 需要改变系统行为时直接调用运行时模块或持久化模块
4. 通过 `aux_sensor_demand_refresh()` 把“当前页面真正需要什么数据”同步回采集层

## 当前约定

- 首页只实时刷新当前激活 tile
- 翻页后立即刷新新激活页，避免展示旧值
- 非当前展示页不应继续触发不必要的数据读取
- 设置页采用 `DISPLAY / DASHBOARD / VEHICLE` 两级结构
- `BLE Scan` 和 `OBD Protocol` 仍然与 `Settings` 平级，不并入设置页

## 阅读建议

1. 先看 `ui.c` / `ui.h`
2. 再看 [screens/README.md](screens/README.md)
3. 接着看 `ui_home_runtime.c`
4. 最后按页面阅读具体 `ui_ScreenPage*.c`

## 就近说明

- [ui.md](ui.md)
- [screens/ui_ScreenPageBLEScan.md](screens/ui_ScreenPageBLEScan.md)
- [screens/ui_ScreenPageSettings.md](screens/ui_ScreenPageSettings.md)
