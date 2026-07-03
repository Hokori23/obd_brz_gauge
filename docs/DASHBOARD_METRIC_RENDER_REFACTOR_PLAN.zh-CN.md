# 仪表盘页渲染重构方案

更新时间：`2026-07-03`

## 1. 目标

为 1~6 槽普通仪表页建立下一阶段的统一渲染模型，解决“值被裁剪、宽度利用差、刷新路径做了过多布局工作”的结构性问题。

本文件是实施方案，不是泛泛 review。所有结论都对应当前仓库代码。

## 2. 当前现状

现有仪表页主要由：

1. [ui_home_runtime.c](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_home_runtime.c:994) 负责页面创建与刷新。
2. [ui_home_runtime_widgets.c](/D:/Program%20Files%20(x86)/Code%20Projects/obd_brz_gauge/main/export_path/ui_home_runtime_widgets.c:339) 负责槽位卡片、value host、字体估算。

当前架构优点：

1. 已经把 `value` 从槽位 panel 中独立为 `value_host`，开始利用圆盘外侧的可用宽度。
2. `slot_count` 已纳入字体分档。

当前核心问题：

1. 刷新时仍重复调用 `ui_home_runtime_widgets_apply_slot_typography()`。
2. 字体和宽度策略仍偏保守，且刷新态和创建态耦合。
3. 背景分割线、槽位装饰如果继续扩展，容易再次走向“很多对象拼装”的方向。
4. 目前普通仪表页和 `GEAR` / `G-force` 已经走成三套渲染思想，长期会继续分裂。

## 3. 目标架构

普通仪表页不建议做成全自绘文本页，而应该采用“静态背景自绘 + 动态文本对象”的混合模型。

### 3.1 静态背景层

新增一个页面级背景绘制对象，承载：

1. 象限分割线
2. 半圆切割辅助线
3. 特定模板的静态装饰

原则：

1. 一页一个 draw obj
2. 不再每条线一个对象
3. 创建时算一次模板几何，刷新时不动

### 3.2 动态文本层

保留每个槽位的：

1. `name_label`
2. `value_label`
3. `unit_label`

但调整策略：

1. 布局宽度只在创建时或模板变化时重算
2. 字体只在创建时或 item 变化时重算
3. 普通数据刷新只改文字和颜色

## 4. 明确不推荐的方向

### 4.1 不推荐整页纯自绘

原因：

1. 数值文字是主要内容，文本测量和对齐本来就是 LVGL label 的强项。
2. 若改成 canvas/纯 draw text，字体适配、国际化、单位切换都会变差。
3. 当前痛点不是“标签对象太多”，而是“静态与动态没有解耦”。

### 4.2 不推荐刷新时重复算 typography

当前 `ui_home_refresh_metric_tile()` 每次刷新都会再次调用 typography 应用函数。后续应收敛为：

1. 页面 mount 时做首次排版
2. page 配置变化时重排
3. 普通数据刷新不再重排

## 5. 可落地的分阶段实施

### 第一阶段

目标：

1. 维持现有 label 体系
2. 引入页面级背景绘制层
3. 为每个槽位缓存：
   - 几何
   - 文本安全宽度
   - 选定字体档

结果：

1. 先把“静态背景”和“动态文本”解耦
2. 先减少不必要的重排

### 第二阶段

目标：

1. 把 `ui_home_runtime_widgets_apply_slot_typography()` 改成只在必要时触发
2. 引入 `item + slot_count + slot_index` 维度的字体缓存

### 第三阶段

目标：

1. 若仍有性能瓶颈，再评估：
   - 仅对 value 做更细粒度的宽度缓存
   - 或按模板预生成布局表

## 6. 需要锁死的实现约束

后续实现时必须满足：

1. 普通仪表页不能回退成灰底卡片风格。
2. `value` 必须继续围绕视觉中心水平居中增长，而不是只按 title 宽度裁剪。
3. 1~6 槽都必须使用统一的安全宽度求解模型。
4. `SPD` 不再走独立特殊字号逻辑，应与同层级数值使用同一套字体策略。
5. 模板背景和分割线不应再依赖一批零碎对象。

## 7. 推荐的代码落点

### 7.1 新增背景绘制 helper

候选文件：

- `main/export_path/ui_home_runtime_widgets.c`

新增内容：

1. 模板背景 draw helper
2. 槽位安全区域缓存 helper

### 7.2 runtime 结构体缓存

在 `ui_home_tile_runtime_t` 中加入：

1. 每槽位布局缓存
2. 每槽位当前 item cache
3. 每槽位 typography cache

## 8. 测试策略

下一阶段需要新增的测试类型：

1. host 逻辑测试
   - 安全宽度计算
   - slot 字体档选择
2. powershell 合同测试
   - 普通数据刷新路径不再重新套 typography
   - 页面级背景 draw obj 存在

## 9. 与 G-force / GEAR 的统一点

未来首页渲染统一遵循：

1. `GEAR`
   - 单 draw obj 负责转速环
   - label 负责档位文本
2. `G-force`
   - 单 draw obj 负责整张图
3. 普通仪表页
   - 单 draw obj 负责静态背景
   - labels 负责动态文本

统一的不是“全部自绘”，而是：

1. 静态图形尽量集中自绘
2. 高频动态只改必要状态
3. 文本交给 label

## 10. 本阶段完成判定

这份方案入仓库后，下一阶段真正代码完成的判定标准是：

1. 普通仪表页具备页面级背景绘制对象
2. 刷新态不再重复做 typography 重排
3. 有对应 host/合同测试
4. `idf.py build` 通过

