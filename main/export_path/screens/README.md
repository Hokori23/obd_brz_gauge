# `screens`

这里是一屏一个文件的页面实现目录。

当前主流程里最重要的几个 screen：

- `ui_ScreenPageLogo.c`
  启动 Logo 页
- `ui_ScreenPageBLEScan.c`
  蓝牙扫描页
- `ui_ScreenPageODBProtocal.c`
  OBD 协议配置页
- `ui_ScreenPageSettings.c`
  设置页，现已改成两级结构

## 读这些页面时重点看什么

1. 页面创建了哪些 LVGL 控件
2. 页面如何进入、如何退出
3. 是否有手势约定
4. 页面是否直接改动 `nvs_storage`、`elm327_ble_client` 或首页运行时

## 一个实用判断

如果你看到大量：

- `lv_obj_*`
- `lv_label_*`
- `lv_btn_*`
- `lv_roller_*`

那一段大多是在描述静态 UI 结构。

如果你看到：

- `obd_data_get_*`
- `nvs_cfg_get/set`
- `elm327_ble_*`
- `ui_home_runtime_*`

那一段才是在连接页面和系统行为。

## 当前要知道的几个页面规则

- `BLE Scan` 和 `Settings` 仍然与首页平级，由首页上下滑进入
- `Settings` 不是单页大表单了，而是：
  - 一级分类：`DISPLAY` / `DASHBOARD` / `VEHICLE`
  - 二级详情：具体设置控件
- `No page config` 仍然保留，但已经不是死路，必须能返回来源仪表页或 `MENU`

## 页面说明

- [ui_ScreenPageBLEScan.md](ui_ScreenPageBLEScan.md)
- [ui_ScreenPageSettings.md](ui_ScreenPageSettings.md)

## WS175 相关记录

- [docs/WS175_DASHBOARD_POLISH_2026-07-02.md](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/docs/WS175_DASHBOARD_POLISH_2026-07-02.md)
