# `export_path`

这是 UI 导出目录，基本可以视为生成代码和资源目录。

从文件结构判断，它很像由 `SquareLine Studio + LVGL` 导出的工程。

## 目录

```text
export_path/
|-- ui.c / ui.h             UI 入口和对象声明
|-- ui_helpers.*            UI 辅助函数
|-- ui_events.h             事件声明
|-- screens/                各页面实现
|-- images/                 图片资源转成的 C 数组
|-- fonts/                  字体资源转成的 C 数组
|-- components/             UI 组件钩子
`-- assets/                 额外导出资源
```

## 这个目录的特性

- 以生成结果为主，不像手写 UI 那么整洁。
- 页面对象命名和文件名更接近设计稿。
- 修改时要小心，因为设计工具重新导出可能覆盖手工改动。

## UI 和业务怎么连接

典型模式不是 MVC，也不是前端状态管理库，而是：

1. `ui_init()` 创建页面和控件。
2. 定时器、事件回调或页面逻辑直接调用 `obd_data_get_*()`。
3. 把实时数据写回 `label`、`arc`、`needle` 等控件。

## 你读这个目录时的建议

- 先看 `ui.c` 和 `ui.h`，了解页面入口。
- 再看 [screens/README.md](screens/README.md)。
- 最后按页面读具体 `ui_ScreenPage*.c` 文件。

## 就近说明

- [ui.md](ui.md)
- [screens/ui_ScreenPageBLEScan.md](screens/ui_ScreenPageBLEScan.md)
