# `ui_ScreenPageBLEScan.c` 阅读说明

这是一个很适合你切入的页面，因为它不是纯展示页，而是“UI + 设备发现 + 配置持久化”的完整闭环。

## 页面职责

1. 展示已保存的蓝牙 OBD 设备
2. 扫描周边 BLE 设备
3. 允许用户点击设备并连接
4. 把选择结果保存到 `NVS`
5. 支持删除保存的设备

## 这个页面为什么值得先看

它能把三个层串起来：

- UI 层：LVGL 控件和事件
- 驱动层：`elm327_ble_client`
- 存储层：`nvs_storage`

## 关键交互链路

### 扫描

`start_scan()`
-> `elm327_ble_scan_only_start()`
-> 蓝牙线程发现设备
-> `scan_result_cb()`
-> 往 LVGL 列表里加按钮

### 选择设备

`on_device_selected()`
-> 停止扫描
-> 保存设备名到 `NVS`
-> 更新页面显示
-> `elm327_ble_connect_by_name()`
-> 跳转到温度页

### 删除设备

`on_saved_device_delete()`
-> 如已连接则先断开
-> 清空 `ble_device_name`
-> 写回 `NVS`
-> 隐藏已保存设备区域

## 这里有一个很典型的嵌入式 GUI 点

BLE 扫描回调不在 UI 线程里，所以代码要用 `lvgl_mux` 上锁后再更新 LVGL。

这和前端里“只能在主线程碰 DOM”有点像，只是这里要手动做锁保护。
