# Honda Civic Type R FL5 开源 CAN / DBC 资料调研

更新日期：2026-07-03

## 结论先说

截至 `2026-07-03`，`FL5 Civic Type R` 可以确认已经有公开开源资料，但成熟度和 `FT86 Gen1` 不一样。

更准确地说：

- `有公开开源支持`
  - 社区已经把它纳入 `Honda Civic 2022-24` 平台支持链路
  - `opendbc` 里存在明确的 `Honda: Civic Type R support` 合并记录
- `有公开 DBC / 指纹 / 平台配置`
  - 不只是“论坛有人说能用”，而是已有代码、PR、测试路由
- `暂未找到像 FT86 那样现成、直观、面向仪表开发的完整公开信号表`
  - 也就是我还没找到一份社区常用的 `FL5 -> 哪个 CAN ID 是 RPM / SPD / 温度 / G 值` 的整表文档

所以当前最合适的判断是：

1. `FL5` 不是资料空白车型
2. `FL5` 的开源证据链已经足够强，值得继续挖
3. 但现阶段更像“平台级支持已打通”，而不是“仪表字段表已经整理完成”

## 目前能确认的公开证据

### 1. opendbc 已明确覆盖 11 代 Civic 平台

`opendbc` 当前公开文档里明确列出：

- `Honda Civic 2022-24`
- `Honda Civic Hatchback 2022-24`

而 `FL5` 属于这一代 Civic Type R 平台内。

这至少能证明两件事：

1. 社区已经把这代 Civic 作为可识别、可接入的平台处理
2. `FL5` 不需要从零开始猜它是不是完全封闭

### 2. 已有明确的 Civic Type R support PR

`commaai/opendbc` 的 PR `#1659` 标题就是：

- `Honda: Civic Type R support`

这个 PR 的关键信息有：

- 创建日期：`2025-01-23`
- 合并日期：`2025-02-22`
- PR 说明里直接写明：
  - `Car already supported. Just adding trim`
  - 新增了 `route with openpilot`
  - `harness Type: Bosch A`

这说明：

- `FL5` 不是“未来可能支持”
- 而是已经进入开源车库体系里的实际支持项
- 并且是作为 `Civic 2022` 平台下的一个具体 trim 在维护

### 3. 开源实现里已经补了手动挡识别

PR `#1659` 的改动里最有价值的一点，是它不只是加了名字，还补了 `manual transmission` 相关逻辑：

- `interface.py`
  - 对 `HONDA_CIVIC_2022` 增加“如果没有某个自动变速箱特征 ID，则按手动挡处理”的分支
- `carstate.py`
  - 手动挡时改读 `GEARBOX_ALT_2`
  - 新增 `GEAR_MT`
  - 可识别 `clutchPressed`
  - `GEAR_MT == 14` 时识别 `reverse`
- `routes.py`
  - 新增一条注释明确标成 `Civic Type R with manual transmission` 的测试路由

这条证据很重要，因为它说明：

- 社区已经不只是在“认车型”
- 而是在处理 `FL5` 的实际差异点
- `FL5` 至少有一部分报文结构已经被公开消化

## 当前能看到的 DBC 线索

`opendbc` 里 `Honda Civic 2022-24` 当前挂到：

- `honda_bosch_radarless_generated`

这个 DBC 里可以直接看到一些公开信号，例如：

- `ACC_CONTROL` (`0x1C8 / 456`)
- `CRUISE_FAULT_STATUS` (`0x1D3 / 467`)
- `GEARBOX_ALT_2` (`0x1DD / 477`)
  - `GEAR_MT`
- `SPEED_LIMIT_DASH_DISPLAY` (`0x1EF / 495`)

同时它还显式导入了几份公共子 DBC：

- `_honda_common.dbc`
- `_bosch_2018.dbc`
- `_steering_sensors_a.dbc`
- `_gearbox_common.dbc`

这能说明：

- 公开 DBC 不是空壳
- 已经有一部分 Honda Civic 2022+ 平台报文被结构化

但也要注意边界：

- 这些更偏 `ADAS / 控制 / 平台状态`
- 还不能直接等同于“我们已经拿到了面向仪表开发的全套被动量字段表”

也就是说，`DBC 已有` 和 `仪表所需字段已整理完成` 是两件事。

## 新增发现：已经能看到一批“仪表候选量”

继续往下翻 `opendbc` 的 Honda 公共 DBC 后，已经能看到一批非常接近本项目需求的字段。这一点比“只有平台支持”更进一步。

当前能公开看到的候选量包括：

- `ENGINE_DATA` (`0x158 / 344`)
  - `ENGINE_RPM`
  - `XMISSION_SPEED`
- `POWERTRAIN_DATA` (`0x17C / 380`)
  - `PEDAL_GAS`
  - `ENGINE_RPM`
  - `BRAKE_SWITCH`
  - `BRAKE_PRESSED`
  - `ACC_STATUS`
- `WHEEL_SPEEDS` (`0x1D0 / 464`)
  - 四轮轮速
- `CAR_SPEED` (`0x309 / 777`)
  - `CAR_SPEED`
  - `ROUGH_CAR_SPEED`
- `STEERING_SENSORS` (`0x14A / 330`)
  - `STEER_ANGLE`
  - `STEER_ANGLE_RATE`
  - `STEER_WHEEL_ANGLE`
- `KINEMATICS` (`0x094 / 148`)
  - `LAT_ACCEL`
  - `LONG_ACCEL`
- `VEHICLE_DYNAMICS` (`0x1EA / 490`)
  - `LAT_ACCEL`
  - `LONG_ACCEL`

从 `carstate.py` 还能进一步看出社区当前的典型用法：

- `vEgoRaw`
  - 由 `WHEEL_SPEEDS` 和 `ENGINE_DATA.XMISSION_SPEED` 混合得到
- `gasPressed`
  - 直接看 `POWERTRAIN_DATA.PEDAL_GAS`
- `brakePressed`
  - 主要看 `POWERTRAIN_DATA.BRAKE_PRESSED` / `BRAKE_SWITCH`
- `steeringAngleDeg`
  - 来自 `STEERING_SENSORS.STEER_ANGLE`
- `steeringRateDeg`
  - 来自 `STEERING_SENSORS.STEER_ANGLE_RATE`

这说明：

1. `FL5` 至少已经有一部分“做仪表最常见的数据量”可以从公开 Honda 平台 DBC 里看到候选来源
2. `RPM / 车速 / 油门 / 刹车 / 四轮轮速 / 横纵向加速度 / 转向` 这几类数据最值得优先继续核实
3. 目前还欠的主要不是“完全没线索”，而是“这些候选字段对 FL5 实车是否逐项成立、缩放是否一致、ELM327 被动监听能否稳定拿到”

## 2026-07-03 继续补充：新增坐实的几点

### 1. 转向角已经能在公开 DBC 里直接看到

`_steering_sensors_a.dbc` 里直接定义了：

- `STEERING_SENSORS` (`0x14A / 330`)
  - `STEER_ANGLE`
  - `STEER_ANGLE_RATE`
  - `STEER_WHEEL_ANGLE`

而 `carstate.py` 也明确按下面方式使用：

- `steeringAngleDeg = STEERING_SENSORS.STEER_ANGLE`
- `steeringRateDeg = STEERING_SENSORS.STEER_ANGLE_RATE`

这意味着 `FL5` 的以下基础仪表项，当前公开证据已经更扎实：

- `方向盘角度`
- `方向盘角速度`
- `发动机转速`
- `车辆速度`
- `油门 / 刹车状态`

### 2. 手动挡支持已经落到 DBC 字段层面

`honda_bosch_radarless_generated` 中的：

- `GEARBOX_ALT_2` (`0x1DD / 477`)
  - `GEAR_MT`

其枚举已公开给出：

- `1st` 到 `6th`
- `reverse`
- `Clutch`

这比“PR 提到支持手动挡”更进一步，因为它说明公开 DBC 已经把手动挡语义写出来了。

不过当前仍要保留一个工程边界：

- `opendbc carstate.py` 对手动车的主逻辑仍主要依赖 `REVERSE_LIGHT` 做 `reverse/drive` 区分
- 所以 `GEAR_MT` 虽然很有价值，但是否能稳定做完整档位显示，仍建议后续实车抓包确认

### 3. 横纵向加速度已有两个公开候选来源

当前公开 DBC 里同时存在：

- `KINEMATICS` (`0x094 / 148`)
  - `LAT_ACCEL`
  - `LONG_ACCEL`
- `VEHICLE_DYNAMICS` (`0x1EA / 490`)
  - `LAT_ACCEL`
  - `LONG_ACCEL`

其中 `VEHICLE_DYNAMICS.LONG_ACCEL` 的 DBC 注释还明确提示它更像：

- `wheel speed derivative, noisy and zero snapping`

这说明：

1. `FL5` 的 `G-force` / 加速度量不是没有公开入口
2. 但横纵向加速度究竟优先取 `KINEMATICS` 还是 `VEHICLE_DYNAMICS`，还需要按 `FL5` 实车表现筛选
3. 如果以后做 `FL5` 加速度页，最好两个来源都先保留在候选清单里

### 4. 温度类字段仍未在公开 Honda DBC 中坐实

这轮继续检索 `honda_bosch_radarless`、`_honda_common.dbc`、`_bosch_2018.dbc` 后，暂时没有找到能直接对应下列量的公开字段：

- `Coolant temperature`
- `Oil temperature`
- `IAT / Intake air temperature`

当前能看到的“温度”更接近：

- `CAM_TEMP_HIGH`
  - 这是摄像头 / ADAS 状态相关提示位，不是发动机仪表温度量

因此截至 `2026-07-03`，更稳妥的说法是：

1. `FL5` 的 `RPM / 速度 / 油门 / 刹车 / 转向 / 四轮轮速 / 加速度候选` 已经有公开 DBC 线索
2. `冷却液温度 / 油温 / 进气温度` 这类发动机温度量，当前仍未在公开 Honda DBC 中坐实
3. 这些温度类字段更可能需要：
   - 继续挖其他 Civic / Honda 私有 DBC
   - 走诊断 PID / 厂商扩展请求
   - 或靠实车抓包比对补证

## 还有哪些公开来源值得继续看

### Honda Civic B-CAN 项目

公开仓库 `Honda-Civic-B-CAN` 提供了 2016+ Civic 的 `B-CAN` 接入方案，公开说明里提到这条总线可见：

- 盲点指示
- 灯光
- 空调
- 以及更多 BCM 相关数据

这个项目的价值在于：

- 它证明 Honda Civic 平台除了常见动力总线外，还有社区已经在接入的 `B-CAN`
- 对以后想挖 `车身功能类状态` 有参考意义

但它当前还不能直接当成：

- `FL5 专属信号表`
- 或 `可直接照搬的仪表字段数据库`

更适合把它视为 `总线接入参考`。

## 对本项目的工程判断

如果以后要给这个仓库扩展 `FL5` 支持，我建议按下面这个口径推进：

### 可以先当成已确认的事

- `FL5` 存在公开开源支持
- `FL5` 与 `Honda Civic 2022-24` 平台高度相关
- `manual transmission` 相关解析已被开源项目公开实现
- `RPM / 车速 / 油门 / 刹车 / 四轮轮速 / G 值 / 转向` 已经有公开 DBC 候选字段
- `STEER_ANGLE` 与 `GEAR_MT` 已经能在公开 DBC 里直接看到
- `opendbc` 已经是最值得继续深挖的主来源

### 还不能过度宣称的事

- 不能直接说“FL5 全套仪表字段已经有公开 CAN ID 表”
- 不能直接说“已经找到和 FT86 同级别的社区整车信号文档”
- 不能直接把平台 DBC 等同于“所有想要的仪表量都能稳定被动读取”
- 不能直接说 `Coolant / Oil / IAT` 这类温度量已经在公开 DBC 里坐实

### 下一步最值得做的工作

1. 继续逐段解读 `honda_bosch_radarless_generated`
2. 聚焦提取：
   - `RPM`
   - `Speed`
   - `Coolant / Oil / IAT`
   - `Throttle / Pedal`
   - `Brake`
   - `Steering / Yaw / G-force`
3. 如果公开 DBC 里没有直接写清楚，就结合：
   - `fingerprints`
   - `carstate.py`
   - `Honda-Civic-B-CAN`
   - 后续实车抓包
4. 把“适合仪表显示的被动量”单独沉淀成一份 `FL5 passive gauge candidates` 清单

## 和 ZC6 的对比

目前两者的公开资料状态差异，可以这样理解：

- `ZC6 / FT86 Gen1`
  - 已经有更直观的社区 CAN 表
  - 更适合直接做被动仪表
- `FL5`
  - 平台支持和代码证据更强
  - 但“适合仪表开发的整车字段表”还没有 FT86 那么现成

所以如果只看“今天就能最快落地的被动仪表”：

- `ZC6` 资料成熟度更高

如果看“后续继续挖有没有前景”：

- `FL5` 是很值得继续跟进的

## 来源

- commaai/opendbc CARS  
  https://github.com/commaai/opendbc/blob/master/docs/CARS.md
- commaai/opendbc PR #1659: Honda: Civic Type R support  
  https://github.com/commaai/opendbc/pull/1659
- commaai/opendbc Honda values  
  https://raw.githubusercontent.com/commaai/opendbc/master/opendbc/car/honda/values.py
- commaai/opendbc Honda fingerprints  
  https://raw.githubusercontent.com/commaai/opendbc/master/opendbc/car/honda/fingerprints.py
- commaai/opendbc Honda radarless DBC generator source  
  https://raw.githubusercontent.com/commaai/opendbc/master/opendbc/dbc/generator/honda/honda_bosch_radarless.dbc
- commaai/opendbc Honda common DBC  
  https://raw.githubusercontent.com/commaai/opendbc/master/opendbc/dbc/generator/honda/_honda_common.dbc
- commaai/opendbc Honda Bosch 2018 DBC  
  https://raw.githubusercontent.com/commaai/opendbc/master/opendbc/dbc/generator/honda/_bosch_2018.dbc
- commaai/opendbc Honda steering sensors DBC  
  https://raw.githubusercontent.com/commaai/opendbc/master/opendbc/dbc/generator/honda/_steering_sensors_a.dbc
- commaai/opendbc Honda carstate  
  https://raw.githubusercontent.com/commaai/opendbc/master/opendbc/car/honda/carstate.py
- vanillagorillaa/Honda-Civic-B-CAN  
  https://github.com/vanillagorillaa/Honda-Civic-B-CAN
