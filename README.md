# STM32F103C8T6 Temperature Inspector

这是一个基于 STM32F103C8T6 最小系统板的温度检测与报警工程，已经接入 DS18B20、PT100 电压采集、OLED、按键、蜂鸣器、报警灯、SD 卡 FatFs 记录、串口命令、阈值掉电保存，以及 ESP-01S 云服务预留流程。

当前固件已经按 ST-Link 烧录方式配置，主工程在：

```text
Projects/MDK-ARM/TemperatureInspector_STM32F103C8T6.uvprojx
```

详细说明请看：

```text
Docs/工程说明书.md
```

## 快速使用

1. 按 `Docs/工程说明书.md` 的引脚表接线。
2. SD 卡格式化为 FAT32，插入 SD 模块。
3. Keil 打开 `Projects/MDK-ARM/TemperatureInspector_STM32F103C8T6.uvprojx`。
4. 用 ST-Link 连接 PA13/PA14/GND/3.3V，BOOT0 保持 0。
5. 编译并下载，或直接烧录已生成的：

```text
build_gcc/TemperatureInspector.hex
```

## 当前默认参数

- 测温周期：15 秒
- 阈值判断周期：15 秒
- SD 写入周期：15 秒
- DS18B20 默认报警阈值：低于 10 C 或高于 60 C
- PT 默认报警阈值：低于 0 C 或高于 100 C
- 云服务：默认关闭，代码流程保留
- 报警确认：连续 2 次超阈值才报警

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

## 注意

PB3/PB4 用作按键，所以调试方式必须保持 `Serial Wire`，不要启用完整 JTAG。
