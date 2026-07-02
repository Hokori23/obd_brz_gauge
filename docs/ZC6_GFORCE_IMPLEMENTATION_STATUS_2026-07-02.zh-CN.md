# ZC6 G-Force 实现状态

更新时间：`2026-07-02`

## 1. 当前结论

- `G-force-esp32` 可以做真接入，不是 UI 占位。
- 当前 `WS 1.75 AMOLED` 板子带有 `QMI8658` 六轴 IMU。
- `G-force-OBD` 也可以做真接入，因为公开资料已经给出了 `FT86 / ZC6 gen1` 的相关 CAN ID 和字段。
- 但本仓库原先没有“被动 CAN 监听”链路，所以 `G-force-OBD` 还差一层底层接入，不是差页面。

## 2. 已落地内容

### 2.1 `G-force-esp32`

本地已新增：

- `main/bsp_obd_dsp/qmi8658_gforce.c`
- `main/bsp_obd_dsp/qmi8658_gforce.h`

实现内容：

- 通过板级共享 I2C 总线访问 `QMI8658`
- 读取板载加速度计
- 做一次简单静态偏置采样
- 对横向 / 纵向分量做轻量低通和平滑
- 将结果写入缓存：
  - `obd_data_set_lat_accel_imu_x100()`
  - `obd_data_set_lon_accel_imu_x100()`

当前启用策略：

- 仅当首页当前激活页类型为 `G-force-esp32` 时启用采样

### 2.2 页面类型拆分

仪表页类型已拆成：

- `METRIC`
- `GEAR`
- `G-force-OBD`
- `G-force-esp32`

说明：

- `GEAR` 仍是基于 `RPM + Speed + 车型齿比` 的推导档位
- `G-force-OBD` 使用车辆数据源
- `G-force-esp32` 使用板载 `QMI8658`

## 3. `G-force-OBD` 现状

公开资料显示，`FT86 / BRZ / FR-S gen1 (2013-2020)` 的 `0x0D0` 帧包含：

- steering angle
- yaw rate
- lateral acceleration
- longitudinal acceleration

这意味着：

- `2019 ZC6` 在已知公开映射覆盖范围内
- 真实 `G-force-OBD` 可以做

但当前仓库仍缺：

1. ELM327 被动监控模式切换
2. 原始 CAN 行解析
3. `0x0D0` 帧解码到缓存
4. 监控模式与现有 PID 轮询模式的协调

也就是说，当前 `G-force-OBD` 的难点在 BLE/ELM327 适配层，而不是 UI。

## 4. 板载 IMU 结论

`WS 1.75 AMOLED` 官方示例中存在 `QMI8658` 使用代码，说明这块板子确实带 IMU。

当前仓库中，板载 IMU 走的是：

- `board_get_shared_i2c_bus()`
- `qmi8658_gforce_start()`
- `aux_sensor_demand_refresh()`

## 5. 下一步建议

优先级建议：

1. 先在实机确认 `G-force-esp32` 方向和零点是否符合当前安装方向
2. 再给 `ELM327 BLE` 增加被动 CAN 监听模式
3. 解码 `0x0D0`，把 `G-force-OBD` 从“可开发”推进到“已接入”

## 6. 公开参考

- `timurrrr/ft86`
- `timurrrr/RaceChronoDiyBleDevice`
- Waveshare `ESP32-S3-Touch-AMOLED-1.75` 官方示例中的 `QMI8658` 示例

## 7. 2026-07-02 当日新增状态

- `ELM327 BLE` 已补上 `G-force-OBD` 专用监听链路切换：
  - 进入页时走 `ATCRA0D0 + ATMA`
  - 离开页时回到标准 `PID polling` 初始化序列
- 常规 OBD 指标轮询已改成“只轮询当前页面真正需要的指标”。
- `G-force` 页面已升级为圆形十字坐标图，并补上：
  - 当前 G 红点
  - 红点平滑跟随
  - 黄色历史峰值包络
  - 中心点与发光层次
- 当前环境仍缺 `idf.py`，所以这次只能补仓库内契约测试，尚未完成整机编译验证。

## 8. 状态纠偏

上面第 3 节里“当前仓库仍缺 ELM327 监听链路”的判断已经过时，现状应以本节为准：

1. `G-force-OBD` 已经补上专用监听链路切换。
2. 当前实现路径是：
   - 进入 `G-force-OBD` 页
   - 停止常规 PID 轮询
   - 发送 `ATCRA0D0`
   - 发送 `ATMA`
   - 解析 `0x0D0`
   - 写入 `lat/lon accel` 缓存
   - 离开页面后恢复标准 PID polling init sequence
3. 所以上述“还差底层接入”的描述只代表更早阶段的状态，不再代表当前代码状态。
4. 当前真正剩余的是：
   - 真机验证 `0x0D0` 在目标车和目标 ELM327 上是否稳定持续输出
   - 真机确认横纵方向符号是否与安装方向一致
   - 若需要，再继续把 UI 细节向参考图收敛

## 9. G-force UI 参考要求

1. 主体是圆形十字坐标图。
2. 当前值使用单个红点表达。
3. 红点需要轻量防抖和平滑跟随。
4. 历史区需要黄色不规则包络面，而不是只有黄线。
5. 允许工程化近似，但视觉方向必须优先接近用户提供的参考图。
6. 当前值不应退回成左右两个数字块，主语义应由中心坐标图承担。
7. 象限语义要明确表达：
   - 左上：`ACC + LEFT`
   - 右上：`ACC + RIGHT`
   - 左下：`BRAKE + LEFT`
   - 右下：`BRAKE + RIGHT`
8. 红点除平滑外，还要有轻量光晕和惯性跟随感，避免只剩“生硬跳点”。
9. 黄色历史区除了轮廓，还要有不规则填充面和一定发光层次，视觉上更接近参考图里的“历史抓地包络”。

## 10. 2026-07-02 晚间补充

### 10.1 编译验证状态更新

第 7 节里“当前环境仍缺 `idf.py`、尚未完成整机编译验证”的描述已经过时。

当前已完成：

1. `powershell -ExecutionPolicy Bypass -File .\tools\run_ui_platform_static_tests.ps1`
2. 显式补齐本机 `cmake` / `ninja` 到 `PATH` 后执行 `idf.py build`
3. 成功产出：
   - `build/obd_brz_gauge.elf`
   - `build/obd_brz_gauge.bin`

因此当前状态应更新为：

1. 仓库内静态契约测试已通过
2. ESP-IDF 实际整机编译已通过
3. 剩余未闭环的是“真机路测和传感器方向校验”，而不是“源码尚未编译验证”

### 10.2 本轮 G-force UI 细化约束

这部分用于锁定 2026-07-02 晚间继续收敛后的实现方向，避免后续再退回成“通用图表风格”。

1. 黄色历史区不仅要有轮廓线，还要有低透明度填充面，语义上是“历史 G 峰值包络面”。
2. 历史黄色包络与红点必须共用同一套横纵坐标方向，不能出现方向错位。
3. 红点跟随要做分档惯性：
   - 小位移慢跟随，避免抖
   - 大位移快跟随，避免拖尾太重
4. 静止或小 G 状态下，历史包络要缓慢回落，避免黄区永久挂在高值。
5. 四象限文字位置应更接近圆盘内部的 45 度分布，而不是简单贴在四角。
6. 允许继续做视觉微调，但不允许回退成“双数值卡片”或“只有折线没有包络面”的方案。
7. 视觉重心应放在：
   - 深色圆盘底
   - 白色十字轴
   - 单个红点与红色柔光
   - 黄色历史峰值包络面
8. 若后续继续微调，优先调整：
   - 红点阻尼体感
   - 黄区衰减速度
   - 象限标签距离圆心的 45 度分布
   而不是重新改回数值主导式布局。

### 10.3 当前自测覆盖边界

截至本轮，G-force 相关自测已经至少覆盖到下面几层：

1. 按需请求链路：
   - `tests/dashboard/test_aux_sensor_demand_obd_contract.ps1`
   - 约束“当前页面未展示的常规 OBD 指标不再继续轮询”
   - `tests/dashboard/test_aux_sensor_demand_logic.c`
   - 用可编译静态断言锁定：
     - `GEAR` 页强制只要 `RPM + Speed`
     - `G-force-OBD` 页停常规 PID
     - 普通指标页按位组合请求掩码
2. `G-force-OBD` 监听链路：
   - `tests/dashboard/test_gforce_obd_monitor_contract.ps1`
   - 约束 `ATCRA0D0 + ATMA`、监听字节流入口、以及 lat/lon 缓存写入
   - `tests/dashboard/test_zc6_gforce_monitor_decode.c`
   - 将 `ELM327` 监听文本行解析抽为独立逻辑，生产代码直接复用
3. `ZC6 0x0D0` 字节换算：
   - `tests/dashboard/test_zc6_gforce_decode.c`
   - 静态断言横向 / 纵向字节到 `x100 g` 的核心换算关系
4. `G-force-esp32` 板载 IMU 链路：
   - `tests/dashboard/test_qmi8658_gforce_contract.ps1`
   - 约束启停门控、无效 sentinel、deadzone、平滑滤波、重新启用时 bias 复位
5. `G-force` UI 参考约束：
   - `tests/dashboard/test_gforce_ui_contract.ps1`
   - 约束圆盘、十字轴、单红点、黄色填充包络、平滑跟随、历史衰减等关键特征

这意味着当前仓库已经不只是“有实现”，而是对：

1. 数据请求是否按需
2. `G-force-OBD` 是否走专用监听链路
3. `0x0D0` 的核心换算是否保持稳定
4. `G-force-esp32` 是否保留启停和平滑语义
5. UI 是否退回错误形态

都具备了仓库内可重复执行的保护。

但仍需明确边界：

1. 这些测试还不能替代真机路测。
2. 仍需目标车辆上确认：
   - `0x0D0` 在目标 ELM327 上是否持续稳定输出
   - 横纵向正负方向是否和实际安装方向一致
   - 圆屏上红点阻尼 / 黄区衰减的体感是否最终满意
3. 对应的实机执行口径已经单独整理到：
   - [ZC6_GFORCE_ROAD_TEST_PLAN.zh-CN.md](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/docs/ZC6_GFORCE_ROAD_TEST_PLAN.zh-CN.md:1)
