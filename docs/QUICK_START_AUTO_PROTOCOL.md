# 🚀 快速使用自动协议检测

## 立即开始（仅需 3 步）

### 第 1 步：编译并烧录
```bash
cd /Users/parklu/personProject/obd_brz_gauge
idf.py build
idf.py -p PORT flash monitor
```

### 第 2 步：在仪表设置中选择自动协议
1. 主屏 → swipe up/down → **版本页面**
2. → swipe down → **设置页面**
3. 找到 **OBD Protocol** → 选择 **"0 - Automatic"**
4. 屏幕提示 "Saved" ✓

### 第 3 步：连接 OBD 设备
1. 仪表底部 → BLE SCAN 页面（或从主屏左滑至 BLE SCAN）
2. 等待扫描，找到你的 OBD 设备（如 "OBDII"）
3. 点击连接

## 它会怎么做？

```
连接中... 
  ↓
🔍 自动检测协议中...
  尝试协议 1... ⏳ 2s 超时
  尝试协议 2... ⏳ 2s 超时
  尝试协议 3... ⏳ 2s 超时
  ...
  尝试协议 6... ✓ 成功！RPM 读取正常
  保存协议 6 到存储器
  ↓
🎉 数据开始更新
  RPM: 850
  Speed: 0
  Temp: 85°C
  ...
```

## 预期结果

### ✅ 成功的迹象
- 主屏显示 **有变化的数据**（RPM、温度等不再是 0）
- 日志显示 `Protocol X: SUCCESS! (RPM=YYY)`
- 数据每 100ms 刷新一次

### ❌ 失败的迹象
- 所有参数显示 0
- 日志显示 `Protocol auto-detect FAILED`
- 日志持续打印同样的超时消息

---

## 如果自动检测失败

### 快速方案
请参考：[BRZ_ZD8_PROTOCOL_GUIDE.md](./BRZ_ZD8_PROTOCOL_GUIDE.md#快速修复步骤)

1. 进入设置，手动尝试协议 **5 或 3**
2. 每个协议尝试 10 秒，观察数据是否出现

### 深度诊断
参考：[OBD_TROUBLESHOOTING.md](./OBD_TROUBLESHOOTING.md)

---

## 常见问题速答

| 问题 | 答案 |
|-----|------|
| 检测会花多久？ | 最多 22 秒；通常 3-8 秒 |
| 检测时会显示错误数据？ | 不会。检测过程隐藏，完成后才显示 |
| 可以中途取消检测？ | 是，断开 BLE 连接即可 |
| 下次连接还需要检测吗？ | 不需要。协议已保存；除非选回 "Auto" 模式 |
| 多台设备可以用不同协议吗？ | 可以。仪表记住最后检测的协议 |

---

## 进度跟踪

- [x] 自动协议检测功能实现
- [x] RPM 验证机制
- [x] NVS 自动保存
- [x] 日志输出
- [x] 设置页面已支持 "0 - Automatic"

---

## 下一步

✅ **立即试用** 

如有任何问题：
- 查看 **OBD_TROUBLESHOOTING.md** —— 完整故障排查指南
- 查看 **BRZ_ZD8_PROTOCOL_GUIDE.md** —— ZD8 专用指南  
- 查看 **AUTO_PROTOCOL_DETECTION.md** —— 详细技术文档
