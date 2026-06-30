# `app_main.c` 阅读说明

`app_main()` 是整个设备上电后的启动入口。

如果你从 Web 视角看，它很像：

- 前端的 `main.tsx`
- 后端的 `bootstrap()`

只是这里启动的不是 HTTP 服务，而是一整套硬件、UI 和后台任务。

## 启动顺序为什么这么重要

嵌入式里，很多模块有严格依赖：

- `NVS` 要先起来，后面要读配置
- `I2C` 要先起来，后面 LCD/触摸/IO 扩展要用
- `LCD` 要先起来，后面 LVGL 才能注册显示驱动
- `LVGL` 要先起来，后面 `ui_init()` 才能创建页面

所以 `app_main()` 读起来不要只看“调用了什么”，还要看“为什么这个顺序不能乱”。

## 主流程拆解

### 1. 初始化持久化

`nvs_storage_init()`

读取：

- 用户设置
- 里程统计
- 上次选中的车型

### 2. 恢复车型配置

`vehicle_profile_set_active()`

这里决定后续：

- 档位推导规则
- 机油温查询策略

### 3. 初始化硬件总线和控制芯片

- `I2C_Init()`
- `EXIO_Init()`

这一步是为了后续能控制屏幕、触摸和其他扩展 IO。

### 4. 初始化屏幕链路

- `LCD_SetFlushCallback()`
- `LCD_Init()`

这里会把：

- 屏幕控制器
- 背光
- 触摸

一并拉起来。

### 5. 初始化 LVGL

这里做了几件关键事：

- `lv_init()`
- 分配双缓冲 DMA 内存
- 注册显示驱动
- 注册触摸输入驱动
- 创建 LVGL 定时器和任务

可以把这部分理解成“把 GUI 运行时装起来”。

### 6. 创建 UI

`ui_init()`

这一步不是更新数据，而是先把页面树和控件树创建出来。

### 7. 启动通信和采集任务

- `elm327_ble_start_default()`
- `racechrono_ble_diy_start()`
- `rs485_brake_temp_start()`
- `oil_pressure_start()`

这一步后，系统才开始“活起来”。

### 8. 启动里程统计

`vMileageDataStatisticTask()`

这相当于一个后台定时 worker。

## 你读这份文件时最该盯住的点

- 哪些模块是初始化一次就结束
- 哪些模块会创建常驻任务
- 哪些全局对象会被后续模块共享，比如 `panel_handle`、`tp`、`lvgl_mux`

## 最后一个建议

第一次读 `app_main.c`，不要抠 LVGL 细节，先把它当成“装配图”看。
