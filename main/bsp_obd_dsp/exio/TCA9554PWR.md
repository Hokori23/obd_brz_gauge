# `TCA9554PWR.c/.h` 阅读说明

这是 `TCA9554` IO 扩展芯片的具体驱动。

## 这层和 `I2C_Driver` 的关系

- `I2C_Driver` 只懂“如何在总线上读写”
- `TCA9554PWR` 才懂“这个芯片有哪些寄存器、每一位是什么意思”

## 核心能力

- `Read_REG()` / `Write_REG()`：直接读写芯片寄存器
- `Mode_EXIO()` / `Mode_EXIOS()`：设置引脚方向
- `Read_EXIO()` / `Read_EXIOS()`：读取输入电平
- `Set_EXIO()` / `Set_EXIOS()`：设置输出电平
- `Set_Toggle()`：翻转输出

## 这类芯片的本质

你可以把它理解成一个“挂在 I2C 上的远程 GPIO 模块”。

ESP32 不再直接拉某个引脚高低，而是：

1. 通过 I2C 发命令给扩展芯片
2. 扩展芯片替你控制外部引脚

## 在本项目里的作用

至少承担了：

- LCD reset
- Touch reset

这也是为什么显示和触摸初始化前，要先 `EXIO_Init()`。
