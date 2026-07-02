# 圆屏页面 UI 回归清单

## 1. 目的

本清单用于回归当前已重构的圆屏运行时页面，覆盖以下目标：

1. 不出现文字溢出、遮挡、裁切异常
2. 不出现页面级纵向滚动与返回手势冲突
3. 顶部 `HOME`、标题、副标题视觉与交互保持统一
4. 关键选择控件在 `360x360` 与 `WS175` 上都可稳定点按

当前重点页面：

1. `BLE Scan`
2. `Settings`
3. `Dashboard Config`

## 2. 设备矩阵

每次 UI 结构调整后，至少在以下两种分辨率上回归：

1. `360x360`
2. `466x466`（`WS175`）

## 3. 通用回归项

所有页面都要检查：

1. 顶部 `HOME` 按钮可稳定点按，不贴边，不落入圆屏死角
2. 标题与副标题不和顶部按钮重叠
3. 副标题不因为长文案被裁切到不可读
4. 页面不存在意外纵向滚动条
5. 上滑返回与页面内控件操作不抢手势
6. 触控热点不会小到需要反复点按

## 4. BLE Scan

需要逐项确认：

1. `HOME`、`BLE SCAN`、`Saved and nearby devices` 顶部排布完整
2. `SAVED DEVICE` 标题、卡片、删除按钮在有已保存设备时完整可见
3. 删除按钮不会因为圆屏边缘裁切导致难点
4. 已保存设备名较长时，不会把删除按钮顶出卡片
5. `NEARBY` 区域和设备列表之间层次清晰
6. 扫描中 spinner 与状态文字不重叠
7. 上滑返回和点按 `HOME` 的行为一致，都会停扫并回首页

## 5. Settings

需要逐项确认：

1. `DISPLAY / DASHBOARD / VEHICLE / OBD` 四页左右滑动顺畅
2. 页码点与实际页一致，切页时高亮正确
3. `DASHBOARD` 页 `BOOT PAGE` roller 与三组两态按钮在两种分辨率下都不会换行或挤压
4. `VEHICLE` 页 `VEHICLE PROFILE` roller 不裁切
5. `OBD` 页协议选项在两种分辨率下可读，不出现明显横向溢出
6. 上滑返回与 `HOME` 行为一致
7. 任一页都不出现页面级纵向滚动

## 6. Dashboard Config

需要逐项确认：

1. `TYPE / SLOTS / EDIT SLOT / METRIC` 四行在两种分辨率下完整可见
2. `METRIC` roller 在支持项较长时仍可辨认，不出现明显裁切
3. 修改 `TYPE` 为非 `METRIC` 时，相关行隐藏逻辑正确
4. 修改 `SLOTS` 后，`EDIT SLOT` 选项数量同步更新
5. `EDIT SLOT` 切换后，`METRIC` 实际绑定到对应 slot
6. 页面不再依赖纵向滚动完成编辑
7. `HOME` 返回后，能回到对应仪表页而不是错误落回菜单

## 7. 回归结论记录

每次真机回归建议记录：

1. 回归日期
2. 固件构建目录：`build_default` 或 `build`
3. 页面名称
4. 是否通过
5. 具体问题和复现步骤

如果本清单中任一项失败，不应仅通过增加说明文字规避，必须优先修正交互或布局本身。

## 8. 本轮重点回归项

本轮在 `Settings / Dashboard Config` 继续开发后，除上述通用项外，额外重点确认：

1. `Settings` 顶部分类命名与最终方案一致：`DISPLAY / DASHBOARD / VEHICLE / OBD`。
2. `BOOT PAGE` 已从车辆分类中拆出，归位到 `DASHBOARD` 分类页。
3. `VEHICLE` 分类页只承载车型相关设置，不再混入启动页设置。
4. `Dashboard Config` 不因“显示全部项”重新引入页面级纵向滚动或新的返回手势冲突。
5. `Dashboard Config` 与 `Settings` 的顶部 `HOME`、标题块、卡片/选择器视觉语言保持一致，不出现明显割裂感。
6. `360x360` 与 `WS175` 下，长指标名和较长协议名都不会出现不可接受的裁切或误触。

## 9. 这轮新增回归点

1. `Settings` 页内新增纵向滚动后，不得和原有“上滑返回”抢手势；页面顶部、中部、底部都要分别验证。
2. `Dashboard Config` 如存在纵向滚动或长内容区域，也不得和返回手势冲突，重点验证 `TYPE / SLOTS / EDIT SLOT / METRIC` 所在区域。
3. `Settings` 并入 `OBD` 后，用户感知到的交互模型要与仪表页一致：左右切分类、上滑返回、点 `HOME` 返回，不能再保留第二套需要说明文字的模型。
4. `Menu <-> Gauge` 原交互保持不变；这轮统一只允许影响 `Settings / OBD / Dashboard Config`，不能回归破坏主页与仪表页现有手感。
## 2026-07-03 Additional Regression Points

When validating the latest WS175 home runtime and settings/navigation changes, also check:

1. Home horizontal page switching still feels fully follow-hand on device.
2. `ui_home_perf` does not regress further while page-switch behavior changes.
3. G-force pages still allow long-press entry into dashboard edit mode.
4. Rotation-confirm modal `Cancel` does not trigger reboot.
5. Vehicle-specific unsupported metrics stay hidden after switching vehicle profiles and rebuilding home.

Known current investigation result:

- The home horizontal switch path currently shows a stable two-full-screen redraw pattern.
- This remains true even when rotation is disabled, so it should be tracked as a container/runtime performance issue rather than a rotation-only issue.
