# BRZ ZC6 开源 CAN / 诊断数据调研

更新日期：2026-07-04

## 结论先说

截至 `2026-07-04`，我没有找到一份足够可靠、可直接复用的公开资料，能够证明：

- `BRZ ZC6 / FT86 Gen1` 在原厂被动广播 CAN 上已经有社区坐实的 `点火提前角 / Ignition Timing` 信号 `CAN ID + 字段定义`

更准确地说：

- `Ignition Timing`
- `Knock Correction`
- `Advance Multiplier / IAM / AM`

这些量在 `Subaru logger / SSM / 调参` 生态里是明确存在的。公开资料能证明它们是可读参数。

但这不等于：

- 它们已经被社区稳定映射为 `FT86 Gen1` 原厂被动广播 CAN 上的公开信号

所以对仓库里的工程判断应该是：

1. `ZC6 点火提前角指标` 目前不能写成“已有开源被动 CAN ID”
2. 如果后续要做 `点火提前角 / IAM / Knock Correction`，技术路线应优先转向：
   - `SSM / logger`
   - 厂商扩展诊断
   - 或 ELM327 能否稳定透传该类请求
3. 对当前这种轻量仪表场景，优先级更高的仍然是已有开源被动 CAN 映射的基础信号

## 关于点火提前角，这次能确认到什么

### 能确认的事实

`RomRaider` 公开 logger 定义里明确存在：

- `Ignition Timing`
- `Knock Correction`
- `Advance Multiplier`

其中：

- `Ignition Timing` 被描述为“所有修正后的实际点火提前角”
- `Knock Correction` 被描述为“对主点火表增加或减少的修正量”
- `Advance Multiplier` 被描述为决定 knock correction advance/retard 的量

这能证明：

- `ZC6 / Subaru FA20` 这类参数不是虚构量
- 它们在 Subaru ECU 的 logger / 诊断语境下是公开存在的

### 不能直接推出的结论

我这次没有找到公开资料能把上面这些量直接落到：

- `0x140`
- `0x141`
- `0x360`
- 或其他 FT86 社区常用被动广播 ID

也就是说，当前没有可靠证据支持：

- “ZC6 点火提前角已经有公开被动 CAN ID，可以像 RPM / 车速那样直接 sniff”

这是这次更新的核心结论。

## ZC6 当前可确认的开源被动 CAN ID

下面这些来自公开社区资料，适合作为仓库里“可依赖的被动 CAN 开源量”基线。

### 高置信度、适合仪表开发优先使用

- `0x0D0 (208)`
  - `Steering angle`
  - `Yaw rate`
  - `Lateral acceleration`
  - `Longitudinal acceleration`
- `0x0D1 (209)`
  - `Speed`
  - `Brake position / brake pressure`
- `0x0D4 (212)`
  - `Wheel speed FL / FR / RL / RR`
- `0x140 (320)`
  - `Engine RPM`
  - `Accelerator position`
  - `Throttle position`
- `0x360 (864)`
  - `Engine oil temperature`
  - `Coolant temperature`

这些已经足以支撑当前仓库里最重要的一批 ZC6 仪表量：

- `RPM`
- `SPD`
- `OIL`
- `CLT`
- `TPS`
- `G-force`

## 仍可参考，但要标注置信度或语义风险

- `0x018 (24)`
  - `Steering angle`
  - 公开资料认为可用，但 `0x0D0` 已提供更完整同类数据
- `0x141 (321)`
  - `Engine load?`
  - `Gear`
  - `Accelerator pedal position?`
  - 其中 `engine load?` 和部分字段在原始资料里本身就带问号，建议视为“可参考但应实车复核”
- `0x144 (324)`
  - `Brake pedal pressed`
- `0x152 (338)`
  - `Hand brake on`
  - `Brake pedal pressed`
- `0x282 (642)`
  - `Seatbelt plugged in`
- `0x361 (865)`
  - `Gear`
- `0x374 (884)`
  - `Driver / passenger door`
  - `Fog lights`
  - `Boot open`
- `0x375 (885)`
  - `Driver / passenger door`
  - `Boot open`
  - `Lights on`

## 对仓库当前指标的直接影响

### 可以继续当作 ZC6 开源被动 CAN 指标

- `RPM`
- `SPD`
- `OIL`
- `CLT`
- `TPS`

## Update 2026-07-04

结合当前仓库实现，ZC6 相关路径现在建议按下面区分：

- `GEAR-DERIVED`
  - 来源：`RPM + Speed + vehicle_profiles`
  - 说明：适合作为默认可用档位路径，但不应承诺可靠识别 `R`
- `GEAR-MONITOR`
  - 来源：ELM327 monitor mode + `0x141`
  - 说明：适合作为 ZC6 专用增强路径，但要接受适配器不支持 `ATMA/ATCRA` 的现实风险
- `OIL-CAN`
  - 来源：ELM327 monitor mode + `0x360`
  - 说明：温度变化慢，低频 monitor-mode 是合理实现路径
- `IGN`
  - 当前仓库实现来源：标准 OBD `01 0E`
  - 说明：这条路径优先于“假定已有公开被动 CAN ID”
- `G-force`

### 可以做，但要明确语义和置信度

- `LOD`
  - 社区资料里能找到接近 `engine load` 的字段，但不如 `RPM / SPD / OIL / CLT` 稳
- `GEAR`
  - 社区资料可用，但要接受它更偏“推导值”而非绝对高置信原厂直出值

### 不应写成“已有 ZC6 开源被动 CAN ID”

- `点火提前角 / Ignition Timing`
- `Knock Correction`
- `Advance Multiplier / IAM / AM`
- `FBKC / FLKC`

### 明确不是 ZC6 原厂开源基础 CAN 量

- `BST`
  - ZC6 原厂自吸，不应当作基础量暴露
- `OIP`
  - 继续按外挂油压链路处理
- `BKT`
  - 继续按外挂刹车温度链路处理

## 对后续开发的建议

如果后续你要补一个 `ZC6 点火提前角` 仪表项，合理路径不是继续猜被动广播 ID，而是：

1. 先单独调研 `Subaru SSM / logger` 参数访问路径
2. 确认当前 ELM327 BLE 盒子是否能稳定读该类参数
3. 评估刷新率是否够做实时表盘
4. 再决定要不要把它接入当前仪表体系

换句话说：

- `点火提前角` 现在更像“诊断/调参链路功能”
- 不是“现成开源被动 CAN 指标”

## 这次新增结论的依据

### FT86 社区被动 CAN 资料

`timurrrr/ft86` 的 `gen1` 页面公开列出了：

- `0x0D0`
- `0x0D1`
- `0x0D4`
- `0x140`
- `0x141`
- `0x144`
- `0x152`
- `0x282`
- `0x360`
- `0x361`
- `0x374`
- `0x375`

但没有给出 `Ignition Timing` 对应的被动广播 CAN 字段。该页甚至还列了“希望继续找到的量”，说明这份资料本身也仍在补全中。

`RaceChronoDiyBleDevice` 的 `ft86.md` 推荐 ID 同样集中在：

- `RPM`
- `Speed`
- `Oil temp`
- `Coolant temp`
- `Throttle / accelerator`
- `Brake`
- `Steering angle`
- `Yaw / acceleration`

同样没有给出 `Ignition Timing` 的被动广播映射。

### Subaru logger / 调参资料

`RomRaider` logger definitions 公开包含：

- `Ignition Timing`
- `Knock Correction`
- `Advance Multiplier`

因此我这里的判断是：

- “这些量存在且可读”是事实
- “这些量已有 FT86 被动广播 CAN ID”是当前没有证据支持的推断，不能写进仓库当结论

这是一条基于公开资料边界做出的工程判断。

## 来源

- timurrrr/ft86: CAN bus (2013-2020 model years)  
  https://github.com/timurrrr/ft86/blob/main/can_bus/gen1.md
- RaceChronoDiyBleDevice: FT86 CAN DB  
  https://github.com/timurrrr/RaceChronoDiyBleDevice/blob/master/can_db/ft86.md
- RomRaider logger definitions  
  https://github.com/RomRaider/original.flat/blob/master/trunk/docs/logger/log_defs.xml
