<a id="english"></a>
# STM32F103C8T6 Temperature Inspector

[English](#english) | [中文](#中文)

This is a temperature-monitoring and alarm project for an STM32F103C8T6 minimum-system board. It integrates DS18B20, PT100 voltage acquisition, OLED, keys, a buzzer, alarm LEDs, SD-card logging through FatFs, serial commands, persistent thresholds, and a reserved ESP-01S cloud-service flow.

## Purpose

The project is designed for embedded temperature-monitoring scenarios. It covers sensor acquisition, data display, alarm decisions, data logging, and serial interaction. It can serve as an STM32 course project, a temperature-acquisition system, a sensor-integration experiment, or an embedded-project portfolio piece.

## Highlights

- Supports both the DS18B20 digital temperature sensor and PT100 analog temperature acquisition.
- OLED displays current temperature, thresholds, alarm status, and system information.
- Supports key configuration, buzzer alarms, and alarm-light indications.
- Uses FatFs to write temperature logs to an SD card.
- Provides serial commands for debugging and parameter inspection.
- Threshold parameters persist across power cycles.
- Reserves an ESP-01S cloud-service flow for future network expansion.
- Supports ST-Link programming and a Keil MDK-compatible project structure.

## Project Entry Points

Main project file:

~~~text
Projects/MDK-ARM/TemperatureInspector_STM32F103C8T6.uvprojx
~~~

Detailed documentation:

~~~text
Docs/工程说明书.md
~~~

Generated HEX file:

~~~text
build_gcc/TemperatureInspector.hex
~~~

## Quick Start

1. Complete wiring using the pin table in Docs/工程说明书.md.
2. Format the SD card as FAT32 and insert it into the SD module.
3. Open Projects/MDK-ARM/TemperatureInspector_STM32F103C8T6.uvprojx in Keil.
4. Connect ST-Link to PA13, PA14, GND, and 3.3 V; keep BOOT0 at 0.
5. Build and download, or directly flash build_gcc/TemperatureInspector.hex.

## Default Parameters

~~~text
Temperature sample period: 15 seconds
Threshold-check period: 15 seconds
SD write period: 15 seconds
DS18B20 default alarm range: below 10 C or above 60 C
PT default alarm range: below 0 C or above 100 C
Cloud service: disabled by default; flow retained in code
Alarm confirmation: alarm after 2 consecutive out-of-range readings
~~~

## Directory Guide

| Directory / file | Purpose |
|---|---|
| Inc/ | Application headers and HAL configuration currently in use |
| Src/ | Application source currently in use |
| Core/ | Synchronized backup of application source and headers |
| Projects/MDK-ARM/ | Keil project |
| Middlewares/Third_Party/FatFs/ | FatFs file-system source |
| Drivers/ | STM32 HAL/CMSIS drivers |
| Docs/ | Project manual, CubeMX configuration, and paper analysis |
| build_gcc/ | GCC build output, including flashable HEX |
| TemperatureInspector_STM32F103C8T6.ioc | CubeMX configuration file |

## Suitable Uses

- STM32 embedded-systems course project
- Multi-sensor temperature acquisition and alarm system
- Integrated OLED, SD-card, serial, key, and buzzer experiment
- Keil MDK and STM32 HAL project-structure study

## Notes

- PB3/PB4 are used as keys. Keep debugging in Serial Wire mode; do not enable full JTAG.
- Before powering the board, check power, logic levels, sensor wiring, and SD-card formatting.
- The cloud-service flow is reserved and disabled by default; it can be connected to an actual platform later.

<a id="中文"></a>
# STM32F103C8T6 温度检测仪

[English](#english) | [中文](#中文)

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

~~~text
Projects/MDK-ARM/TemperatureInspector_STM32F103C8T6.uvprojx
~~~

详细说明文档：

~~~text
Docs/工程说明书.md
~~~

已生成 HEX：

~~~text
build_gcc/TemperatureInspector.hex
~~~

## 快速使用

1. 按 Docs/工程说明书.md 的引脚表完成接线。
2. 将 SD 卡格式化为 FAT32，并插入 SD 模块。
3. 使用 Keil 打开 Projects/MDK-ARM/TemperatureInspector_STM32F103C8T6.uvprojx。
4. 用 ST-Link 连接 PA13、PA14、GND、3.3V，BOOT0 保持 0。
5. 编译并下载，或直接烧录已生成的 build_gcc/TemperatureInspector.hex。

## 当前默认参数

~~~text
测温周期：15 秒
阈值判断周期：15 秒
SD 写入周期：15 秒
DS18B20 默认报警阈值：低于 10 C 或高于 60 C
PT 默认报警阈值：低于 0 C 或高于 100 C
云服务：默认关闭，代码流程保留
报警确认：连续 2 次超阈值才报警
~~~

## 目录说明

| 目录/文件 | 作用 |
|---|---|
| Inc/ | 当前实际使用的应用头文件和 HAL 配置 |
| Src/ | 当前实际使用的应用源码 |
| Core/ | 同步备份的应用源码/头文件 |
| Projects/MDK-ARM/ | Keil 工程 |
| Middlewares/Third_Party/FatFs/ | FatFs 文件系统源码 |
| Drivers/ | STM32 HAL/CMSIS 驱动 |
| Docs/ | 工程说明、CubeMX 配置、论文分析 |
| build_gcc/ | GCC 编译输出，含可烧录 HEX |
| TemperatureInspector_STM32F103C8T6.ioc | CubeMX 配置文件 |

## 适用场景

- STM32 嵌入式课程设计。
- 多传感器温度采集与报警系统。
- OLED、SD 卡、串口、按键和蜂鸣器综合实验。
- Keil MDK + STM32 HAL 工程结构学习。

## 注意事项

- PB3/PB4 用作按键，所以调试方式必须保持 Serial Wire，不要启用完整 JTAG。
- 上板前建议先检查供电、电平、传感器接线和 SD 卡格式。
- 云服务流程目前为预留状态，默认关闭，后续可继续接入实际平台。
