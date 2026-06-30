# `screens`

这里是一屏一个文件的页面实现。

文件名基本已经说明了页面职责：

- `ui_ScreenPageLogo.c`：启动 Logo 页
- `ui_ScreenPageBLEScan.c`：蓝牙扫描页
- `ui_ScreenPageODBProtocal.c`：OBD 协议配置页
- `ui_ScreenPageTemp.c` / `TempCustom.c`：温度类页面
- `ui_ScreenPageInfo.c` / `InfoCustom.c`：综合信息页
- `ui_ScreenPageBrakeTemp.c` / `BrakeWarn.c`：刹车温度显示与告警
- `ui_ScreenPageOilPressure.c` / `OilWarn.c`：机油压力显示与告警
- `ui_ScreenPageNeedle.c`：指针仪表页
- `ui_ScreenPageSettings.c`：设置页
- `ui_ScreenPageEasterEgg.c`：彩蛋页

## 读这些页面时看什么

1. 页面创建了哪些 LVGL 控件。
2. 数据刷新来自哪里。
3. 页面切换靠什么事件触发。
4. 是否直接调用了 `nvs_storage` 或 `elm327_ble_client`。

## 一个实用判断

如果你看到大量 `lv_obj_...`、`lv_label_...`、`lv_img_...`，那一段多半是静态 UI 结构。

如果你看到：

- `obd_data_get_*`
- `nvs_cfg_get/set`
- `elm327_ble_*`

那一段才是页面和系统行为发生耦合的地方。

## 已补的页面说明

- [ui_ScreenPageBLEScan.md](ui_ScreenPageBLEScan.md)
- [ui_ScreenPageTemp.md](ui_ScreenPageTemp.md)
- [ui_ScreenPageSettings.md](ui_ScreenPageSettings.md)
- [ui_ScreenPageNeedle.md](ui_ScreenPageNeedle.md)
