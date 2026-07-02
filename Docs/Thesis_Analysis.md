# Thesis Analysis And Implementation Mapping

Appendix was not used. The implementation is based on the main thesis body and your confirmed hardware.

## Confirmed Hardware

- Board: STM32F103C8T6 green board
- Display: 4-pin I2C OLED
- WiFi: ESP-01S
- Storage: 6-pin SPI MicroSD module
- PT100 interface: voltage output to STM32 ADC
- Download/debug method: ST-Link SWD
- DS18B20 alarm rule: below 10 C or above 60 C
- PT100 alarm rule: below 0 C or above 100 C

## Thesis Requirements Mapped To Code

- Real-time temperature inspection: `app.c`
- DS18B20 temperature acquisition: `app_ds18b20.c`
- PT100 temperature acquisition through ADC linear formula `temperature = 51.2 * voltage`: `app_pt100.c`
- OLED local display: `app_oled.c`
- LED + buzzer alarm: `app_alarm.c`
- SD card historical data storage: `app_storage.c`
- ESP8266/ESP-01S WiFi remote upload: `app_esp8266.c`
- OneNET MQTT upload: minimal MQTT packet build in `app_esp8266.c`
- Three buttons for independent DS18B20/PT100 threshold adjustment and switching: `app_keys.c`

## Thesis Pins Preserved

- MCU: STM32F103C8T6
- USART1 optional debug UART: PA9 TX, PA10 RX
- ST-Link SWD:
  - SWDIO: PA13
  - SWCLK: PA14
- DS18B20 data pin: PA0
- SD card SPI2:
  - CS: PB12
  - SCK: PB13
  - MISO: PB14
  - MOSI: PB15

## Pins Reassigned For Your Actual Modules

- PT100 voltage input:
  - ADC: PA1 / ADC1_IN1
- Keys:
  - K1: PB3
  - K2: PB4
  - K3: PB5
- OLED I2C1:
  - SCL: PB6
  - SDA: PB7
- ESP-01S USART2:
  - STM32 TX: PA2
  - STM32 RX: PA3
- Buzzer: PB0
- Alarm LED: PB1
- Board LED: PC13

## Calibration Items

- PT100 conversion uses your provided relation: `temperature = 51.2 * voltage`.
- ADC reference is set to 3.3 V in `PT100_ADC_VREF`.
- If measured temperature is off, adjust `PT100_TEMP_PER_VOLT` in `app_config.h`.

## OneNET Items Still Needed

Fill these in `app_config.h` before expecting cloud upload:

- `WIFI_SSID`
- `WIFI_PASSWORD`
- `ONENET_PRODUCT_ID`
- `ONENET_DEVICE_NAME`
- `ONENET_DEVICE_TOKEN`
