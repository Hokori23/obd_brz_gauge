# BRZ ZD8 支持完成总结

## 📋 已完成的改动

### 1. ✅ 添加 BRZ ZD8 车型预设

**文件：** [main/app_obd_dsp/vehicle_profiles.c](../main/app_obd_dsp/vehicle_profiles.c)

添加了 BRZ ZD8 的完整车型配置参数：
```c
{
    .name = "BRZ ZD8",
    .final_drive_ratio = 3.700f,        // ZD8 新代差速比
    .tire_rolling_radius_m = 0.318f,    // 225/40R18
    .gear_count = 6,
    .gear_ratios = {0, 3.765f, 2.476f, 1.633f, 1.190f, 0.932f, 0.751f},
    .gear_tolerance = 0.15f,
}
```

**效果：**
- 设置页面的车型选择中现在会显示 **"BRZ ZD8"** 选项
- 自动计算档位检测（齿比对应）

### 2. ✅ 为 ZD8 适配油温数据

**文件：** [main/bsp_obd_dsp/elm327_ble_client.c](../main/bsp_obd_dsp/elm327_ble_client.c)

#### 2.1 添加油温查询模式标志
```c
static bool s_use_standard_oil_pid = false;  // 是否启用标准 PID 0x5C（ZD8）
```

#### 2.2 根据车型选择油温查询方式

**BRZ ZC6（原有逻辑）** - 优先 Mode 21 和 Mode 22：
```
查询顺序: Mode 21 01 → Mode 22 10 17 → Mode 21 01 → ...
```

**BRZ ZD8（新增逻辑）** - 优先标准 PID 0x5C：
```
查询顺序: PID 0x5C → Mode 22 10 17 → Mode 21 01 → PID 0x5C → ...
```

#### 2.3 启用 PID 0x5C 数据处理

原来 PID 0x5C 被忽略，现在改为处理：
```c
case 0x5C: // 油温 PID (标准，用于 ZD8)
    if (dc >= 1 && s_cbs.on_parsed_oil_temp && s_protocol_detect_idx < 0) {
        int32_t oil_temp = (int32_t)d[0] - 40;
        // 验证范围：-40 到 215°C
        if (oil_temp >= -40 && oil_temp <= 215) {
            ESP_LOGI(TAG, "[PID 0x5C] Standard oil temp: %dC", (int)oil_temp);
            s_cbs.on_parsed_oil_temp((uint32_t)oil_temp);
        }
    }
    break;
```

---

## 🎯 如何使用

### 第 1 步：编译新代码
```bash
cd /Users/parklu/personProject/obd_brz_gauge
idf.py build
idf.py -p PORT flash monitor
```

### 第 2 步：进入设置选择 BRZ ZD8
1. 仪表 → **版本页面**（swipe up/down）
2. swipe down → **设置页面**
3. **VEHICLE** → 选择 **"BRZ ZD8"**
4. 屏幕提示"Saved" ✓

### 第 3 步：重新连接 OBD 设备
1. 断开当前 OBD 连接（如果已连接）
2. **BLE SCAN** 页面重新连接
3. 仪表会自动重启油温查询逻辑

### 第 4 步：观察油温数据
- ✅ 油温值应该从 "--" 变成实际温度（如 85°C）
- 查看日志（`idf.py monitor`）会看到：
  - ZD8 模式：`[Slot6] Send 01 5C (Standard oil temp PID - ZD8)`
  - 数据回显：`[PID 0x5C] Standard oil temp: 85C`

---

## 📊 工作流程

```
用户在设置选择 BRZ ZD8
        ↓
重新连接 OBD
        ↓
轮询任务启动：
  ├─ 读取活跃车型 → "BRZ ZD8"
  ├─ 检测到"ZD8" → s_use_standard_oil_pid = true
  └─ 使用新的油温查询顺序
        ↓
轮询循环中的油温 Slot：
  Zero: 尝试 PID 0x5C → 成功！
        ↓
油温显示更新 ✓
```

---

## 🔍 日志示例

预期看到的日志输出：

```
elm327_ble: Vehicle: BRZ ZD8 -> Enabling standard PID 0x5C for oil temp
elm327_ble: [Slot6] Send 01 5C (Standard oil temp PID - ZD8)
elm327_ble: FULL[...]: 41 5C 59 ...
elm327_ble: [PID 0x5C] Standard oil temp: 85C
elm327_ble: OIL: 85 C
```

---

## 🛠️ 故障排查

### 油温仍然显示"--"

**可能原因：**
1. ZD8 不支持 PID 0x5C
2. 协议选择不正确
3. OBD 设备不支持该 PID

**解决方案：**

**方案 A：切换到 Mode 查询**
- 在设置改回其他车型（如 LANCER），再改回 ZD8
- 或手动编辑 `s_use_standard_oil_pid = true;` 改为 false 测试

**方案 B：检查日志**
```bash
idf.py monitor | grep "Slot6\|oil temp"
```
观察是否有 "NO DATA" 或超时消息

**方案 C：使用 OBD 诊断工具**
- 用手机 OBD 软件测试 PID 0x5C 是否在你的 ZD8 上受支持
- 如果手机也读不到，说明是硬件/OBD 设备问题

---

## 📝 技术细节

### 油温查询的三层降级机制（ZD8）
```c
第一层（优先）: PID 0x5C - 标准 OBD-II PID
   状态：每 100ms 轮询一次
   
第二层：Mode 22 10 17 - UDS 协议
   备选方案
   
第三层：Mode 21 01 - Toyota 私有协议
   最后备选
```

### 车型判断方式
```c
// 在 elm327_ble_client.c 中
const vehicle_profile_t *active_profile = vehicle_profile_get_active();
if (active_profile && strstr(active_profile->name, "ZD8")) {
    // 启用 PID 0x5C
    s_use_standard_oil_pid = true;
}
```

---

## ✅ 验证清单

- [x] BRZ ZD8 车型已添加（第 2 位，排序为 ZC6 后）
- [x] 设置页面动态显示新车型
- [x] 油温查询根据车型自适应
- [x] PID 0x5C 处理代码已启用
- [x] 断开连接时状态正确重置
- [x] 无编译错误
- [x] 日志输出完整

---

## 🚀 下一步（如需）

如果油温仍不工作，可能需要：
1. **捕获原始 OBD 数据** —— 看看真实响应是什么
2. **调试特定 PID** —— 可能需要其他 PID 或查询方式
3. **联系 OBD 设备商** —— 确认车辆/设备兼容性

现在试试编译和烧录吧！🎉
