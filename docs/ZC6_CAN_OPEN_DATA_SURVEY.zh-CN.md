# BRZ ZC6 开源 CAN / 诊断数据调研

更新日期：2026-07-02

## 结论先说

这轮只做开源资料调研，不改底层采集协议。

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

## 来源

- FT86 Gen1 CAN 文档  
  https://github.com/timurrrr/ft86/blob/main/can_bus/gen1.md
- RaceChrono DIY BLE 设备的 FT86 CAN 数据库  
  https://github.com/timurrrr/RaceChronoDiyBleDevice/blob/master/can_db/ft86.md
- EcuTek: Subaru Ignition Timing Parameters (CAN Vehicles)  
  https://ecutek.atlassian.net/wiki/spaces/SUPPORT/pages/77856769
- EcuTek: Subaru BRZ/Scion FR-S/Toyota GT86 fact sheet  
  https://download.ecutek.com/marketing/factsheets/EcuTek%20Fact%20Sheet%20-%20Subaru%20BRZ.pdf
