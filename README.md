# STM32F103C8T6 Temperature Inspector

这是一个基于 STM32F103C8T6 最小系统板的温度检测与报警工程。项目已经接入 DS18B20、PT100 电压采集、OLED、按键、蜂鸣器、报警灯、SD 卡 FatFs 记录、串口命令、阈值掉电保存，以及 ESP-01S 云服务预留流程。

## 项目定位

本项目面向嵌入式温度监测场景，重点覆盖传感器采集、数据显示、报警判断、数据记录和串口交互。它既可以作为 STM32 课程设计，也可以作为温度采集系统、传感器综合实验或嵌入式项目作品展示。

## 功能亮点

- 同时支持 DS18B20 数字温度传感器和 PT100 模拟温度采集。
- OLED 显示当前温度、阈值、报警状态和系统信息。
- 支持按键设置、蜂鸣器报警和报警灯提示。
- 使用 FatFs 将温度记录写入 SD 卡。
- 支持串口命令交互，便于调试和参数查看。
- 阈值参数支持掉电保存。
- 预留 ESP-01S 云服务流程，便于后续扩展联网能力。
- 支持 ST-Link 烧录，工程结构适配 Keil MDK。

## 工程入口

主工程文件：

```text
Projects/MDK-ARM/TemperatureInspector_STM32F103C8T6.uvprojx
```

详细说明文档：

```text
Docs/工程说明书.md
```

已生成 HEX：

```text
build_gcc/TemperatureInspector.hex
```

## 快速使用

1. 按 `Docs/工程说明书.md` 的引脚表完成接线。
2. 将 SD 卡格式化为 FAT32，并插入 SD 模块。
3. 使用 Keil 打开 `Projects/MDK-ARM/TemperatureInspector_STM32F103C8T6.uvprojx`。
4. 用 ST-Link 连接 PA13、PA14、GND、3.3V，BOOT0 保持 0。
5. 编译并下载，或直接烧录已生成的 `build_gcc/TemperatureInspector.hex`。

## 当前默认参数

```text
测温周期：15 秒
阈值判断周期：15 秒
SD 写入周期：15 秒
DS18B20 默认报警阈值：低于 10 C 或高于 60 C
PT 默认报警阈值：低于 0 C 或高于 100 C
云服务：默认关闭，代码流程保留
报警确认：连续 2 次超阈值才报警
```

## 目录说明

| 目录/文件 | 作用 |
|---|---|
| `Inc/` | 当前实际使用的应用头文件和 HAL 配置 |
| `Src/` | 当前实际使用的应用源码 |
| `Core/` | 同步备份的应用源码/头文件 |
| `Projects/MDK-ARM/` | Keil 工程 |
| `Middlewares/Third_Party/FatFs/` | FatFs 文件系统源码 |
| `Drivers/` | STM32 HAL/CMSIS 驱动 |
| `Docs/` | 工程说明、CubeMX 配置、论文分析 |
| `build_gcc/` | GCC 编译输出，含可烧录 HEX |
| `TemperatureInspector_STM32F103C8T6.ioc` | CubeMX 配置文件 |

## 适用场景

- STM32 嵌入式课程设计。
- 多传感器温度采集与报警系统。
- OLED、SD 卡、串口、按键和蜂鸣器综合实验。
- Keil MDK + STM32 HAL 工程结构学习。

## 注意事项

- PB3/PB4 用作按键，所以调试方式必须保持 `Serial Wire`，不要启用完整 JTAG。
- 上板前建议先检查供电、电平、传感器接线和 SD 卡格式。
- 云服务流程目前为预留状态，默认关闭，后续可继续接入实际平台。
