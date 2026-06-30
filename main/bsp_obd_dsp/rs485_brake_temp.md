# `rs485_brake_temp.c/.h` 阅读说明

这个模块走的是另一条传感器链路，不依赖 OBD。

## 它做什么

- 通过 `UART + RS485`
- 按 `Modbus RTU` 协议
- 轮询刹车温度传感器
- 把结果写进 `obd_data_cache`

## 关键流程

1. 组一个 Modbus 请求帧。
2. 通过 UART 发出去。
3. 读回响应字节流。
4. 校验 CRC16。
5. 解析前两个数据字节为温度。
6. 写入 `obd_data_set_brake_temp_x10()`。

## 你可以类比成

另一种“串口 RPC 调用”，只是协议不是 HTTP，而是 Modbus RTU。

## 重点看

- `modbus_crc16()`
- `build_req_frame()`
- `parse_temp_from_rx()`
- `query_with_cfg()`

## 为什么这里要控制 `DE/RE`

RS485 常见是半双工总线。

发送和接收不能同时占线，所以代码会在：

- 发之前切到 TX
- 发完再切回 RX

这就是 `DE/RE` 引脚控制的意义。
