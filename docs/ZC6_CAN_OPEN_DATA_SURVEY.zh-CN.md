# BRZ ZC6 开源 CAN / 诊断数据调研

更新日期：2026-07-02

## 结论先说

这轮只做开源资料调研，不改底层采集协议。

先纠正一个口径：如果之前把“ZC6 没有爆震 / AM”说成了绝对结论，那是不准确的。

更准确的说法应该是：

- 如果说的是 `Subaru logger / SSM / 调参链路`，那 `爆震相关`、`Knock Correction`、`IAM / AM` 确实是有 live data 的。
- 如果说的是 `FT86 主 CAN 上已公开、稳定、适合被动 sniff 的原厂广播 ID`，这轮仍然没有找到足够可靠的公开证据。

当前能高置信度找到、而且适合做被动 CAN 读取的 ZC6 / FT86 Gen1 数据，主要还是基础行车量：

- `RPM`：有公开 CAN ID
- `SPD`：有公开 CAN ID
- `OIL`：有公开 CAN ID
- `CLT`：有公开 CAN ID
- `TPS / 油门相关`：有公开 CAN ID，但要区分踏板和节气门
- `BKT / OIP`：不是原厂 FT86 开源 CAN 常见项，仍应视为外接传感器链路

而用户关心的：

- `爆震相关`：这轮没有找到稳定、公开、可直接被动 sniff 的 ZC6 原厂 CAN ID
- `AM / IAM / Advance Multiplier`：这轮也没有找到稳定、公开、可直接被动 sniff 的原厂 CAN ID

更合理的判断是：

- `爆震`
- `FBKC / FLKC / Knock Correction`
- `IAM / AM`

这些值更像是 Subaru ECU 的诊断 / logger / tuning 数据，而不是 FT86 主车身 CAN 上长期公开广播的基础频道。

同一轮公开资料里还能确认另一件很重要的事：

- `横向加速度`
- `纵向加速度`

在 `FT86 Gen1 CAN ID 0x0D0` 上是有公开映射的，因此如果我们要先做 `ZC6` 专属的横纵向加速度仪表，这条链路比爆震 / AM 更适合作为第一优先级。

## 开源资料里高置信度的 FT86 Gen1 CAN 频道

根据 `timurrrr/ft86` 的 Gen1 文档，以及同作者给 RaceChrono DIY BLE 设备整理的 `ft86.md`：

- `0x140 (320)`
  - `Engine RPM`
  - `Accelerator position`
  - `Throttle position`
- `0x0D1 (209)`
  - `Speed`
  - `Brake pressure / brake position`
- `0x0D4 (212)`
  - 四轮轮速
- `0x360 (864)`
  - `Engine oil temperature`
  - `Coolant temperature`
- `0x0D0 (208)`
  - `Steering angle`
  - `Yaw rate`
  - 横纵向加速度
- `0x141 (321)`
  - 文档把其中一个字段标成 `Engine load?`
  - 这个量更像“较高置信度但仍建议实车复核”

### 对本项目现有仪表项的映射建议

高置信度可继续保留：

- `RPM`
- `SPD`
- `OIL`
- `CLT`

可保留但需要注明语义：

- `TPS`
  - 要确认我们最终想显示的是踏板开度还是节气门开度
- `LOD`
  - 开源文档里能找到接近 `engine load` 的字段，但置信度不如 `RPM / SPD / OIL / CLT`

不应当视为 ZC6 原厂公开 CAN 常驻项：

- `BST`
  - ZC6 原厂自吸，本项目应按车型隐藏
- `OIP`
  - 仍应视为外接油压链路
- `BKT`
  - 仍应视为外接刹车温度链路

## 为什么这轮不建议把爆震和 AM 当作“已找到 CAN ID”

这轮查到的公开 FT86 Gen1 CAN 文档，能稳定列出来的都是基础运行量；没有出现：

- `Knock`
- `Knock Correction`
- `Fine Learning Knock Correction`
- `Ignition Advance Multiplier / IAM / AM`

这里我做一个明确区分：

- “没在这轮公开 CAN 文档里找到”
  - 这是基于公开资料的事实
- “因此它大概率不是主车身 CAN 常驻广播，而是 ECU logger / 诊断量”
  - 这是基于资料缺失和 Subaru 调参资料的推断

也就是说，这不是 100% 证明车上绝对没有相关报文，而是目前没有可靠公开证据支持“直接被动 sniff 就能稳定拿到”。

## 重新澄清：你记得“爆震是有的”这件事，方向是对的

这次补充搜索后，比较合理的统一口径如下：

1. `爆震 / Knock Correction / IAM / AM` 在 Subaru 的 logger / 调参生态里是存在的。
2. 它们不是这轮已经被公开坐实的 `FT86 主 CAN 常驻广播量`。
3. 所以“有”与“能不能直接做被动 CAN 仪表”是两件事，不能混为一谈。

对本项目而言，这个区分非常关键：

- 如果目标是 `现有 ELM327 + 被动监听风格` 的轻量实时仪表，那么优先级更高的是 `RPM / SPD / OIL / CLT / G-force` 这类已公开频道。
- 如果目标是 `后续补上爆震 / IAM / FBKC / FLKC`，技术路径应转向 `SSM / logger / 扩展诊断请求`，而不是默认继续在公开 FT86 主 CAN 广播里找。

## 爆震 / AM 更像诊断与调参数据

EcuTek 的 Subaru ignition timing 资料明确把下面这些作为日志参数讨论：

- `Ign, Cor. AK (Knock Ret.)`
- `Ign. Cor. AP (Learnt Cor.)`
- `Ign. Cor. AT (Coarse Cor.)`
- `Knock Correction`
- `Knock Sensor Input`
- `Ignition Timing`
- `Learned Ignition Timing`

同页还明确说明：

- 这是围绕 Subaru CAN Suite 的日志参数解释
- 对 BRZ 也“非常相似”

BRZ 的 EcuTek 资料也明确提到：

- 有高速日志能力
- 有“relative to the advance multiplier”的 ignition 视图

这说明：

- `AM / IAM`
- `Knock correction`

这些值在调参生态里是存在的，而且能被读到；
但它更像 ECU 内部日志 / 诊断参数，不等同于“原厂公开 CAN ID 已经被社区稳定枚举”。

RomRaider 的 logger 定义也能进一步说明这一点。公开定义里明确存在：

- `Advance Multiplier`
- `Knock Correction`

而且 `Advance Multiplier` 的说明直接把它描述为反映发动机爆震倾向、并参与 `Knock Correction` 点火修正的量。这进一步支持了“这些量确实存在且可读”，但读取语境更接近 `Subaru logger / 诊断参数`，而不是“FT86 主 CAN 被动广播表里的常规字段”。

## 对当前仓库的直接影响

这轮调研后，仓库里的产品决策可以更明确：

1. `ZC6` 下应继续隐藏 `BST`
2. `OIP / BKT` 继续按外接传感器逻辑处理，不要误当原厂 ZC6 基础 CAN 量
3. 如果后续要上：
   - `爆震`
   - `AM / IAM`
   - `FBKC / FLKC`

   那下一步应该调研的是：
   - Subaru SSM / logger 参数
   - ELM327 可否通过扩展 PID 或厂商诊断请求稳定读取
   - 采样率是否够做仪表实时显示
4. 如果后续优先扩展一个 `ZC6` 专属新仪表页类型，建议优先做 `横纵向加速度表`，因为它已经有公开 CAN 映射，工程风险明显低于爆震 / IAM。

## 这轮给出的工程判断

### 适合继续依赖的“开源稳定量”

- `RPM`
- `SPD`
- `OIL`
- `CLT`

### 可以做，但要先实车确认语义/精度

- `TPS`
- `LOD`

### 这轮不建议宣称“已经找到公开稳定 CAN ID”

- `爆震`
- `AM / IAM`
- `FBKC / FLKC`

## 对其他车型的参考价值

这轮顺手对比下来，可以把“开源资料成熟度”粗分成三档：

1. `像 FT86 / MX-5 这类已有社区 CAN 表的车型`
   - 社区已经把 `CAN ID -> 信号语义 -> 缩放关系` 整理成可直接复用的文档或数据库
   - 这类车型最适合走“被动监听 + 轻量仪表”路线
2. `像 Honda Civic / FL5 这类已有 DBC / 指纹 / 平台支持的车型`
   - 开源资料已经证明“平台可接入、报文可识别、部分字段可解析”
   - 但还不一定有人把“适合仪表显示的整车信号表”整理到 FT86 那么直观
3. `只有 logger / 诊断参数、缺少公开广播表的量`
   - 典型就是 `爆震 / IAM / FBKC / FLKC`
   - 更适合沿 `SSM / logger / 厂商诊断` 路线推进，而不是继续假设主 CAN 广播里能直接捞到

对本项目的含义是：

- `ZC6` 继续优先吃透已经公开坐实的 `RPM / SPD / OIL / CLT / G-force`
- 需要“调参类”数据时，优先开一条独立的 `SSM / logger` 研究线
- 如果以后扩展到 `Honda FL5`，不要沿用 `FT86 已有现成表` 的预期；要按“平台级资料已存在，但仪表级映射仍需继续挖”的节奏推进

## 2026-07-03 补充：FL5 的公开资料状态

这一轮继续往下找后，可以把 `FL5 Civic Type R` 的公开资料状态说得更具体：

- `不是没有开源资料`
  - `opendbc` 已明确合入 `Honda: Civic Type R support`
  - 该 PR 合并日期是 `2025-02-22`
- `但目前更像平台级支持，不像 FT86 那样已有一份社区常用整车信号表`
  - `opendbc` 把 `Honda Civic 2022-24` / `Civic Hatchback 2022-24` 归到同一平台
  - 当前平台配置使用 `honda_bosch_radarless_generated`
- `已经能确认的 FL5 关键信号方向`
  - 手动挡支持是公开坐实的
  - DBC 里能看到 `GEARBOX_ALT_2 / GEAR_MT`
  - PR 里也明确补了 `manual transmission` 的识别和测试路由

因此，更合理的工程判断是：

1. `FL5` 已经有公开开源的 CAN / DBC / 指纹资料
2. 这些资料足以说明“这车不是黑盒，社区已经打通过平台支持”
3. 但截至 `2026-07-03`，我还没有找到一份像 `FT86 gen1.md` 那样、可直接拿来做仪表页字段表的 `FL5` 社区整理版公开文档
4. 如果后续真要做 `FL5` 仪表支持，下一步应优先：
   - 继续解读 `opendbc` 的 Honda DBC
   - 把适合仪表的被动量单独拎出来
   - 再配合实车抓包确认 `RPM / SPD / 温度 / 油门 / 刹车 / G` 等字段

## 来源

- FT86 Gen1 CAN 文档  
  https://github.com/timurrrr/ft86/blob/main/can_bus/gen1.md
- RaceChrono DIY BLE 设备的 FT86 CAN 数据库  
  https://github.com/timurrrr/RaceChronoDiyBleDevice/blob/master/can_db/ft86.md
- RomRaider logger definitions  
  https://github.com/RomRaider/original.flat/blob/master/trunk/docs/logger/log_defs.xml
- EcuTek: Subaru Ignition Timing Parameters (CAN Vehicles)  
  https://ecutek.atlassian.net/wiki/spaces/SUPPORT/pages/77856769
- EcuTek: Subaru BRZ/Scion FR-S/Toyota GT86 fact sheet  
  https://download.ecutek.com/marketing/factsheets/EcuTek%20Fact%20Sheet%20-%20Subaru%20BRZ.pdf
- commaai/opendbc CARS  
  https://github.com/commaai/opendbc/blob/master/docs/CARS.md
- commaai/opendbc PR #1659: Honda: Civic Type R support  
  https://github.com/commaai/opendbc/pull/1659
