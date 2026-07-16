# 温度检测仪硬件交付说明

本目录是 `TemperatureInspector_STM32F103C8T6` 的 KiCad 10 硬件工程。主控采用与现有 CubeMX/固件引脚兼容的 `STM32F103CBT6`（LQFP-48）。

- GitHub 工程：<https://github.com/jiayu-here/TemperatureInspector_STM32F103C8T6>
- KiCad 工程：`TemperatureInspector_STM32F103C8T6_Hardware.kicad_pro`
- 原理图：`TemperatureInspector_STM32F103C8T6_Hardware.kicad_sch`
- PCB：`TemperatureInspector_STM32F103C8T6_Hardware.kicad_pcb`
- 原理图 PDF：`Documentation/Schematic.pdf`

## 1. 电路功能

- USB-C 仅作为 5 V 供电输入，不连接 USB 数据线。
- AP2112K-3.3 将 5 V 转换为 3.3 V。
- DS18B20 数字温度采集。
- PT100 模块模拟电压采集，板上提供 1 kΩ + 100 nF RC 滤波。
- OLED、Micro-SD、ESP-01S、UART1、SWD 接口。
- 3 个按键、复位键、有源蜂鸣器、报警灯、运行灯和电源灯。

## 2. 接口针序

### J1 - USB-C 电源

USB-C 5 V 电源输入；CC1/CC2 各使用 5.1 kΩ 下拉。封装选用电源专用的 `GCT USB4125 6P`，与原理图电源引脚一一对应。该接口不支持 USB 数据通信，也不实现 USB-PD 协议。

### J2 - DS18B20

| 引脚 | 信号 | MCU |
|---|---|---|
| 1 | 3V3 | - |
| 2 | DS18B20_DQ | PA0 |
| 3 | GND | - |

### J3 - PT100 模块

| 引脚 | 信号 | MCU/说明 |
|---|---|---|
| 1 | 5V | 模块电源 |
| 2 | 3V3 | 备用 3.3 V 电源 |
| 3 | PT100_RAW/VOUT | 经 1 kΩ + 100 nF 滤波后进入 PA1/ADC1_IN1 |
| 4 | GND | - |

PT100 模块的 VOUT 必须保持在 0-3.3 V 范围内，禁止将 5 V 模拟电压直接送入 PA1。

### J4 - OLED（I2C）

| 引脚 | 信号 | MCU |
|---|---|---|
| 1 | GND | - |
| 2 | 3V3 | - |
| 3 | I2C_SCL | PB6 |
| 4 | I2C_SDA | PB7 |

### J5 - Micro-SD（SPI2）

| 引脚 | 信号 | MCU |
|---|---|---|
| 1 | 3V3 | - |
| 2 | SD_CS | PB12 |
| 3 | SD_SCK | PB13 |
| 4 | SD_MISO | PB14 |
| 5 | SD_MOSI | PB15 |
| 6 | GND | - |

### J6 - 调试串口（USART1）

| 引脚 | 信号 | MCU |
|---|---|---|
| 1 | 3V3 | - |
| 2 | DEBUG_TX | PA9/USART1_TX |
| 3 | DEBUG_RX | PA10/USART1_RX |
| 4 | GND | - |

### J7 - SWD

| 引脚 | 信号 | MCU |
|---|---|---|
| 1 | 3V3 | 目标板电平参考 |
| 2 | SWDIO | PA13 |
| 3 | SWCLK | PA14 |
| 4 | GND | - |

### JP1 - BOOT0

| 引脚 | 信号 |
|---|---|
| 1 | 3V3 |
| 2 | BOOT0 |

正常运行时 JP1 保持断开，R4 将 BOOT0 下拉。需要进入 STM32 ROM Bootloader 时再短接 JP1。

## 3. 固件引脚映射

| 功能 | MCU 引脚 |
|---|---|
| DS18B20 | PA0 |
| PT100 ADC | PA1 |
| ESP-01S UART TX/RX | PA2/PA3（USART2） |
| 调试 UART TX/RX | PA9/PA10（USART1） |
| SWDIO/SWCLK | PA13/PA14 |
| 蜂鸣器 | PB0 |
| 报警灯 | PB1 |
| KEY1/KEY2/KEY3 | PB3/PB4/PB5 |
| OLED SCL/SDA | PB6/PB7（I2C1） |
| SD CS/SCK/MISO/MOSI | PB12/PB13/PB14/PB15（SPI2） |
| 运行灯 | PC13 |

PB3/PB4 已作为按键使用，调试模式必须保持 Serial Wire，不能启用完整 JTAG。

## 4. PCB 与层叠

- 板框：100 mm × 80 mm。
- 板厚：1.6 mm。
- 4 层铜：
  - L1 / F.Cu：元件和信号。
  - L2 / In1.Cu：整面 GND。
  - L3 / In2.Cu：整面 3V3。
  - L4 / B.Cu：信号和 GND 辅助覆铜。
- 4 个 M3 安装孔。
- 推荐制造材料：常规 FR-4；具体介质厚度和铜厚由板厂按其标准 4 层 1.6 mm 叠层确认。

## 5. 设计规则

| 项目 | 设置 |
|---|---|
| 默认/最小线宽 | 0.18 mm |
| 最小铜间距 | 0.15 mm |
| 铜到板边 | 0.30 mm |
| 默认过孔 | 0.40/0.20 mm（外径/钻孔） |
| USB_5V 主干 | 0.60 mm |
| +5V 支路 | 0.30 mm |
| 差分线宽/间距 | 0.18/0.18 mm |
| 差分过孔间距 | 0.18 mm |

USB-C 为纯供电接口，所以当前 PCB 没有实际 USB 差分对；差分规则已保留供后续扩展使用。

## 6. ERC/DRC 验证

最终 KiCad 10.0.4 检查结果：

- ERC：0 Errors，0 Warnings。
- DRC：0 Violations。
- 未连接焊盘：0。
- 封装错误：0。

报告位于：

- `Verification/erc.rpt`
- `Verification/erc.json`
- `Verification/drc.rpt`
- `Verification/drc.json`

## 7. 制造文件

`Manufacturing` 目录包含：

- `Gerber/`：11 个制板层 Gerber 文件和 `.gbrjob` 作业文件。
- `Drill/`：PTH/NPTH Excellon 钻孔文件、钻孔图和钻孔报告。
- `BOM.csv`：物料清单。
- `Component_Positions_All.csv`：全部元件坐标。
- `PickPlace_SMD.csv`：SMD 贴片坐标。

Gerber 使用 KiCad 扩展名 `.gbr`。下单时将 `Gerber` 和 `Drill` 一并交给板厂；贴片时再提供 BOM 和坐标文件。

## 8. 上电与生产注意事项

1. 首次上电前检查 USB 5 V、3.3 V 和 GND 是否短路。
2. 所有外设逻辑电平必须为 3.3 V。
3. PT100 模块输出不得超过 3.3 V。
4. ESP-01S 瞬时电流较大，实际装配时确认 AP2112K、USB 电源和去耦满足峰值电流。
5. ESP-01S 在 3D 预览中没有完整器件模型，只显示焊盘和天线禁布区；不影响 PCB 制造文件。
6. ERC/DRC 通过只代表规则检查通过。批量生产前仍应先打样、实测电源和各接口，再让板厂执行 DFM 检查。
