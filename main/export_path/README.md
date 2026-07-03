# `export_path` 模块说明

这个目录承载了当前项目的大部分 UI 代码。
虽然保留了明显的 `SquareLine Studio + LVGL` 导出痕迹，但现在已经不只是“纯生成文件”，而是实际的 UI 运行时目录。

## 目录结构

```text
export_path/
|-- ui.c / ui.h                 顶层 UI 入口与全局 screen 引用
|-- ui_helpers.*                通用 UI 辅助函数
|-- ui_platform.*               屏幕尺寸、圆屏安全区和布局缩放
|-- ui_round_shell.*            圆屏视觉外壳与主题组件
|-- ui_home_runtime.*           首页运行时逻辑
|-- ui_home_runtime_widgets.*   首页卡片和指标槽位构建
|-- ui_home_pager.*             首页横向分页器
|-- ui_dashboard_config.*       仪表页配置界面
|-- screens/                    各独立页面实现
|-- fonts/ images/ assets/      导出的字体、图片和资源
`-- components/                 组件钩子和扩展点
```

## 当前最重要的几个模块

### `ui_home_runtime`

这是首页的运行时核心。
它负责：

- 首页 tile 构建
- 页面切换
- 编辑态
- 懒刷新策略
- 当前页与相邻页的内容挂载

### `ui_home_runtime_widgets`

这个模块专门负责指标卡片的结构和排版。
如果你在调字体、间距、圆屏裁切安全区，通常都会落到这里。

### `ui_dashboard_config`

这个模块负责仪表页配置界面，包括：

- 页面类型选择
- 槽位数量
- 指标项选择
- 档位页相关参数

### `screens/ui_ScreenPageSettings.c`

这是设置页的核心实现，当前已经是分组分页结构，而不是把所有设置堆在一页里。

## UI 和业务层是怎么连起来的

这里不是严格的 MVC，也不是前端状态库模式，而是更偏嵌入式运行时的直连方式：

1. 页面初始化函数创建 LVGL 控件。
2. 运行时逻辑直接读取 `obd_data_get_*()`、`nvs_cfg_get()` 等业务数据。
3. 用户操作会直接调用运行时模块或持久化模块。
4. 当显示需求变化时，再调用 `aux_sensor_demand_refresh()` 同步采集层。

## 当前约定

- 首页优先刷新当前激活页，避免无意义的后台刷新。
- 翻页后要立即刷新新激活页，避免显示旧值。
- 非当前展示页尽量不继续触发重数据读取。
- 布局适配优先通过 `ui_platform` 和 `ui_round_shell` 收敛，不在页面里散落硬编码。

## 建议阅读顺序

1. [ui.c](ui.c)
2. [screens/README.md](screens/README.md)
3. [ui_home_runtime.c](ui_home_runtime.c)
4. [ui_home_runtime_widgets.c](ui_home_runtime_widgets.c)
5. [ui_dashboard_config.c](ui_dashboard_config.c)
6. [ui_platform.c](ui_platform.c)
