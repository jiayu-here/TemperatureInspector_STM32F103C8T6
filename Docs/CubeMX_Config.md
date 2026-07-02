# CubeMX Configuration

Open CubeMX and configure these items for STM32F103C8T6.

## Project Manager

- Project Name: `TemperatureInspector`
- Toolchain / IDE: `MDK-ARM`
- Generate peripheral initialization as pair of `.c/.h` files: optional

## System Core

- SYS:
  - Debug: `Serial Wire`. This disables full JTAG and keeps PB3/PB4 usable for keys.
- RCC:
  - HSE: `Crystal/Ceramic Resonator`
  - LSE: optional. Enable only if your board has 32.768 kHz crystal.
- Clock:
  - HSE: 8 MHz
  - SYSCLK: 72 MHz
  - AHB: 72 MHz
  - APB1: 36 MHz
  - APB2: 72 MHz

## GPIO

- PA0: GPIO output, DS18B20 DQ. The driver switches input/output dynamically.
- PA1: ADC1_IN1, PT100 voltage input. Formula: `temperature = 51.2 * voltage`.
- PB3, PB4, PB5: GPIO input, Pull-up, keys.
- PB0: GPIO output, buzzer.
- PB1: GPIO output, alarm LED.
- PB12: GPIO output, SD CS, default high.
- PC13: GPIO output, board status LED.

## I2C

- I2C1:
  - PB6: SCL
  - PB7: SDA
  - Speed: 100 kHz
  - OLED address: usually `0x3C`

## SPI

## ADC

- ADC1:
  - IN1 on PA1
  - Resolution: 12 bit
  - Continuous Conversion: Disabled
  - External Trigger: Software start
  - Sampling time: 55.5 cycles or longer

- SPI2:
  - PB13: SCK
  - PB14: MISO
  - PB15: MOSI
  - Mode: Full-Duplex Master
  - Data Size: 8 bit
  - CPOL: Low
  - CPHA: 1 Edge
  - NSS: Software
  - Prescaler: start slow, e.g. 256 for init, then faster if your SD driver supports it

## USART

- USART1:
  - PA9 TX, PA10 RX
  - 115200 8N1
  - Optional debug printf UART. Firmware download/debug uses ST-Link SWD, not USART1.

- USART2:
  - PA2 TX, PA3 RX
  - 115200 8N1
  - ESP8266 AT command interface

## Middleware

- FATFS:
  - Enable User-defined or SPI SD card mode depending on CubeMX version.
  - If CubeMX does not provide SPI SD glue automatically, keep FatFs disabled first and use the project without SD logging, then add SD diskio later.

## Required User Code Calls

In generated `main.c`:

```c
#include "app.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    MX_SPI2_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_FATFS_Init();

    App_Init();

    while (1)
    {
        App_Loop();
    }
}
```

## ST-Link Download Settings

In Keil:

- `Options for Target` -> `Debug` -> choose `ST-Link Debugger`.
- `Settings` -> Port: `SW`.
- `Flash Download` -> enable `Reset and Run` if you want the program to start after download.

Hardware wiring:

- ST-Link `SWDIO` -> board `SWDIO / PA13`
- ST-Link `SWCLK` -> board `SWCLK / PA14`
- ST-Link `GND` -> board `GND`
- ST-Link `3.3V` -> board `3.3V` only when you power the board from ST-Link
- Keep `BOOT0 = 0`
