# `elm327_ble_client.c/.h` 阅读说明

这是整个项目最复杂、也最值得你花时间的文件。

它同时承担四个角色：

1. BLE GATT 客户端
2. ELM327 命令发送器
3. OBD 文本协议解析器
4. 周期轮询调度器

## 先建立整体图

```text
ESP32
  -> BLE 扫描并连接 OBD 适配器
  -> 找到服务 / 特征值
  -> 发送 "01 0C\r" 这类 ASCII 命令
  -> 收到类似 "41 0C AA BB >" 的文本响应
  -> 解析成 RPM/速度/温度
  -> 写入 obd_data_cache
```

## 为什么它看起来这么大

因为这里把几个本来可以拆层的东西都放在一个文件里了：

- BLE 连接状态机
- 扫描模式
- 协议自动检测
- OBD 轮询
- 多车型机油温策略
- 文本解析

这在嵌入式项目里很常见，尤其是先把功能跑通的阶段。

## 建议分 5 段读

### 1. 对外 API

先看头文件里的这些函数：

- `elm327_ble_init_and_start()`
- `elm327_ble_start_default()`
- `elm327_ble_scan_only_start()`
- `elm327_ble_connect_by_name()`
- `elm327_ble_send_command()`

先只理解“别人怎么用它”，不要一开始就扎进 GATT 事件分支。

### 2. 默认回调

`default_on_parsed_*` 这一组函数负责把解析结果写进 [obd_data_cache.c](../app_obd_dsp/obd_data_cache.c)。

这部分相当于“协议层 -> 业务 store”的桥。

### 3. `obd_poll_task()`

这是轮询主循环。连接建立后，它会按周期发送 PID。

你读这里时重点看：

- 协议自动检测如何决定 `ATSPx`
- 轮询哪些 PID
- 机油温查询如何按车型切换
- `MAP (01 0B)` / `IGN (01 0E)` 如何并入轮询
- monitor-mode 如何在 `G-force` / `GEAR-MONITOR` 之间切换

### 4. GATT notify 处理

这是最关键的一段。

ELM327 返回的是文本流，不一定一包就完整，所以代码会：

1. 把多次 notify 累积到缓冲区
2. 等到收到 `>` 提示符
3. 再按 `41` / `61 01` / `62` 等头部做解析

这一段很像“流式协议拼包 + 文本协议解析”。

### 5. 机油温特殊处理

机油温不是所有车都能通过同一个 PID 读到，所以这里实现了：

- 多种查询模式
- 失败计数
- 按策略回退
- 读数过滤和平滑

这是项目里最强的“车型差异化逻辑”之一。

### 6. 现在新增的 monitor-mode 分支

- `0x0D0`：ZC6 `G-force` monitor
- `0x141`：ZC6 `GEAR-MONITOR`
- 当前仅保留两个 monitor-mode 分支：
  - `0x0D0`：ZC6 `G-force`
  - `0x141`：ZC6 `GEAR-MONITOR`

这两个路径都复用 ELM327 `ATCRA + ATMA`，因此同一时刻只能启用一个 monitor-mode 分支。

### 7. 自愈逻辑

当前实现会记录最近一次有效 OBD 数据时间：

- 短时间恢复有效数据时，清空自愈计数
- 超时后先重做 ELM 初始化
- 连续重做仍无效时，再升级为强制 BLE 断开重连

## 你最需要掌握的几个概念

### `AT` 命令

发给 ELM327 适配器本身的控制命令，比如：

- `ATZ`：复位
- `ATE0`：关闭回显
- `ATSPx`：设置协议

### `01 0C`

发给车辆 OBD 的查询命令，不是发给蓝牙模块本身。

### `41 0C`

表示车辆对 `01 0C` 的响应。

所以你可以简单记：

- `01 xx`：请求
- `41 xx`：Mode 01 响应
- `21 xx` / `61 xx`：厂商扩展请求/响应
- `22 xxxx` / `62 xxxx`：UDS 风格请求/响应

## 对 Web 开发者最容易绕晕的点

- 这里既有 BLE 协议状态机，又有 OBD 协议状态机。
- 收到的不是结构化 JSON，而是带噪声的文本流。
- 很多逻辑依赖时间和设备响应顺序。

## 最后一个阅读建议

第一次读，不要试图一次吃透整个文件。按这条线看最有效：

`elm327_ble_start_default()` -> `obd_poll_task()` -> notify 解析 -> `default_on_parsed_*()` -> `obd_data_cache`
