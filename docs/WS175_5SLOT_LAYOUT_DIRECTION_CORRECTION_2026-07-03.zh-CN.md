# WS175 3-6 槽指标布局方向纠偏

更新时间：`2026-07-03`

## 1. Review 结论

当前判断基本成立：问题不是单纯的字号、间距参数没调准，而是布局模型选错了。

目标图 `P5` 的视觉逻辑更接近“指标内容块”，而当前 `P1~P4` 的实现仍然更接近“槽位网格里放三行文字”。这会天然导致：

1. `value` 不够强，无法成为第一视觉中心。
2. `label` 和 `unit` 与 `value` 的关系松散，像三段独立文字。
3. 分割线、槽位边界和行列网格感过强，内容块本身反而不够成立。
4. 只靠微调 `centered` 参数，很容易在不同槽数之间反复失衡。

所以“需要从 centered slot layout 切到 metric block layout”这个判断是对的。

但之前文档里“后续只重做 5 槽，不要先做 3/4/6”的表述已经过期。用户最新确认：`3/4/5/6` 槽都要优化，只是不能继续套同一套简单居中参数。

## 2. 当前实现偏差

当前代码的主要问题在 `ui_home_runtime_widgets.c`：

1. `ui_home_widgets_use_centered_metric_layout()` 直接让 `3~6` 槽共用 centered layout。
2. `label / value / unit` 仍按固定三段 band 摆放。
3. `unit` 现在被放在底部 band，和 `value` 不够贴合。
4. `value` 的自适应字号虽然有最长值 sample，但它仍只是在“槽位文字”模型里计算，没有围绕内容块的视觉主次去计算。
5. `ui_home_runtime.c` 里的同行字号统一是正确方向，但只是排版辅助，不足以解决整体视觉模型问题。

因此，下一步不要继续在这些 helper 上小幅改百分比；应该先建立可复用的 `metric block` 布局模型。

## 3. 目标效果

`3/4/5/6` 槽统一遵循同一套视觉原则，但每种槽数有自己的模板参数。

核心目标：

1. `value` 是绝对主角，字号按指标最长可能值自适应。
2. `label` 固定字号，作为标题存在，不能和 `value` 抢层级。
3. `unit` 固定字号，作为 `value` 下方的小注脚，必须贴近 value，而不是贴槽位底部。
4. 同一视觉行里的左右指标，`value` 字号取较小值保持一致。
5. 内容块应优先稳定在各自安全视觉中心，而不是机械等分槽位。
6. 分割线只是辅助结构，不能决定文字层级。
7. 底部单槽必须按自身安全区独立居中，不能继承左右列锚点。

## 4. 下一步工作

### 4.1 建立 metric block 数据模型

在 `ui_home_runtime_widgets.c` 内新增内部结构，不暴露到公共 API：

```c
typedef struct {
    lv_coord_t x;
    lv_coord_t y;
    lv_coord_t w;
    lv_coord_t h;
    lv_coord_t content_x;
    lv_coord_t content_y;
    lv_coord_t content_w;
    lv_coord_t content_h;
    lv_coord_t center_x;
    lv_coord_t label_y;
    lv_coord_t value_y;
    lv_coord_t unit_y;
    int16_t label_font;
    int16_t value_font;
    int16_t unit_font;
} ui_home_metric_block_layout_t;
```

职责边界：

1. `ui_home_runtime.c` 继续负责生成槽位几何和同行字号统一。
2. `ui_home_runtime_widgets.c` 负责把单个槽位转成 `metric block`，并摆放 label/value/unit。
3. 不把 `metric block` 变成全局公开模型，避免扩大改动面。

### 4.2 替换 centered band 布局

移除或停止使用这些 centered helper：

1. `ui_home_widgets_centered_name_top_offset`
2. `ui_home_widgets_centered_name_band_height`
3. `ui_home_widgets_centered_value_band_height`
4. `ui_home_widgets_centered_value_y_offset`
5. `ui_home_widgets_centered_unit_band_height`
6. `ui_home_widgets_centered_unit_bottom_offset`

新的布局方式：

1. 先算 `content rect`，它是圆屏安全区内可读内容区域。
2. 再在 `content rect` 内放置一个纵向内容块。
3. `value` 放在内容块视觉中心附近。
4. `label` 放在 `value` 上方，距离固定或按槽高轻微缩放。
5. `unit` 放在 `value` 下方，距离必须小于 label 到 value 的距离。

### 4.3 字号规则

固定字号建议先用保守值，后续按真机照片微调：

1. `3 槽`：label `18~20`，unit `14~16`
2. `4 槽`：label `17~18`，unit `14~15`
3. `5 槽`：label `18~20`，unit `14~16`
4. `6 槽`：label `15~16`，unit `12~14`

`value` 字号规则：

1. 继续用最长 sample 自适应。
2. RPM sample 使用 `9999`。
3. SPEED sample 使用 `999`。
4. BAT / OILP sample 使用 `99.9`。
5. BOOST sample 使用 `-99.9`。
6. 温度类 sample 使用 `999`。
7. 同行双槽 value 字号统一取较小值。

### 4.4 3/4/5/6 模板要求

`3 槽`：

1. 上方双槽左右对齐。
2. 底部单槽独立按圆屏安全区居中。
3. 底部单槽的 `value` 可以比上方更强，但不要破坏整体密度。

`4 槽`：

1. 2x2 内容块。
2. 左列、右列各自形成稳定视觉列。
3. 上下两行的 `value` 视觉中心要一致。

`5 槽`：

1. 2 + 2 + 1。
2. 上两行左右列对齐。
3. 底部单槽是独立大内容块，不是第三行普通格子。

`6 槽`：

1. 2 + 2 + 2。
2. 六个指标密度最高，优先保证 value 可读。
3. label/unit 需要更弱，避免压缩 value。

### 4.5 验收标准

每次改完后用真机照片对照 `P5` 检查：

1. 第一眼是否先看到 value。
2. label 是否像标题，而不是和 value 同权重。
3. unit 是否贴着 value，而不是贴着槽位底部。
4. 3/4/5/6 是否都没有文字裁剪。
5. 左右列是否稳定，不随指标名长度明显漂移。
6. 底部单槽是否独立居中。
7. 分割线是否退到辅助层级。

## 5. 执行注意事项

1. 不要新增高频 debug 日志。
2. 不要继续追加 mojibake 文本；本文档按 UTF-8 保存。
3. 不要只修 5 槽，`3/4/5/6` 都属于本轮优化范围。
4. 不要把 `1/2` 槽一起重构，除非发现公共 API 必须调整。
5. 改完至少运行 `tools/run_ui_platform_static_tests.ps1`。

## 6. 本轮落地后的踩坑记录

### 6.1 字号变小不是字体 picker 乱选

真机日志里出现过：

```text
metric dbg slots=3 s0:RPM rw77 vr71x125 f16 tw56 s1:SPD rw77 vr71x125 f16 tw42 s2:BAT rw162 vr184x147 f56 tw160
```

这说明 `RPM / SPD` 字号变成 `f16` 的直接原因是布局给上排双槽的宽度只有 `77px`，value 可用宽度只有 `71px`。字体选择器只是按照 `value_rect` 正确降档，不是随机选小字号。

根因是两层圆屏安全区约束叠加：

1. `ui_home_build_gauge_layout()` 对整行调用 `ui_round_shell_safe_span_for_band()`，上/下行会被圆形最窄处支配。
2. `ui_home_widgets_metric_content_rect()` 又按整个槽位高度计算一次 safe span，进一步压缩 value rect。

正确方向不是继续改字号上限，而是让 `3/4/5/6` 的 metric 页使用稳定内接网格，再由 metric block 自己做内容层级。

### 6.2 最佳 debug 方法

这类圆屏 metric 布局问题不要先靠照片猜，也不要在 LVGL task 里逐槽打长日志。

推荐 debug 输出只保留低频 snapshot：

1. 每个槽数或页面最多输出一次。
2. 输出字段保持短而关键：
   - `slot_count`
   - item name
   - raw slot width
   - `value_rect_w/value_rect_h`
   - selected `value_font`
   - measured `value_text_w`
3. 先看 `raw width` 和 `value_rect`，再判断是布局问题还是字体测量问题。
4. 日志不要放在刷新循环里，也不要逐帧输出。

曾经的错误 debug 方法是在 `ui_home_log_metric_slot_style()` 里逐槽 `ESP_LOGI` 长字符串。它跑在 LVGL task 中，UART 写阻塞后触发 task watchdog，backtrace 落在 `uart_write / esp_log / ui_home_log_metric_slot_style`。这类日志以后必须低频、短行，必要时改成延迟输出或只在人工验证固件里打开。

### 6.3 1/2 槽的独立旧路径

`1/2` 槽不走 `3/4/5/6` 的 dense metric block 路径，而是旧的 box slot path。它原本把 label 放左上、unit 放右下，value 在中间 band，所以天然不和 value 共轴。

后续如果继续调整 1/2 槽，要同时检查：

1. 初始 create path。
2. item 变化后的 typography refresh path。

否则会出现初始化对齐了、刷新后又偏回旧位置的问题。

## 7. 技术债 TODO

1. `UI_HOME_METRIC_LAYOUT_DEBUG` 当前是一次性 snapshot 诊断开关。真机布局最终稳定后，应改成构建开关或运行时 debug flag，默认关闭。
2. `3/4/5/6` 的 `row_weights`、grid inset、5/6 value fit padding 目前是从真机照片和日志收敛出来的模板参数。后续应整理成明确的 per-slot-count layout profile，避免继续散落为魔法数字。
3. 长期更干净的方向是把 metric layout resolver 抽成更纯的几何函数，让 host 测试可以直接断言 `raw slot width / value_rect / selected font`，而不是只靠 PowerShell 正则保护实现形状。
