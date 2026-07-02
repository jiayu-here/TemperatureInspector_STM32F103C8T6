#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "stm32f1xx_hal.h"

/* DS18B20 单总线温度传感器引脚，外部需要 4.7k 上拉到 3.3V。 */
#define DS18B20_PORT GPIOA
#define DS18B20_PIN  GPIO_PIN_0

/* 三个按键均为低电平按下，GPIO 内部上拉。 */
#define KEY1_PORT GPIOB
#define KEY1_PIN  GPIO_PIN_3
#define KEY2_PORT GPIOB
#define KEY2_PIN  GPIO_PIN_4
#define KEY3_PORT GPIOB
#define KEY3_PIN  GPIO_PIN_5

/* 声光报警输出：高电平有效。 */
#define BUZZER_PORT GPIOB
#define BUZZER_PIN  GPIO_PIN_0
#define ALARM_LED_PORT GPIOB
#define ALARM_LED_PIN  GPIO_PIN_1
#define BOARD_LED_PORT GPIOC
#define BOARD_LED_PIN  GPIO_PIN_13

/* SD 卡 SPI2 片选引脚，SCK/MISO/MOSI 在 main.c 中配置为 PB13/PB14/PB15。 */
#define SD_CS_PORT GPIOB
#define SD_CS_PIN  GPIO_PIN_12

/* 主要运行周期配置。 */
#define OLED_I2C_ADDR       (0x3C << 1)
#define SAMPLE_PERIOD_MS    15000U
#define DISPLAY_PERIOD_MS   500U
#define LOG_PERIOD_MS       15000U
#define WIFI_PERIOD_MS      15000U

/* 云服务默认关闭；打开前需要先填写下方 WiFi 和 OneNET 参数。 */
#define APP_ENABLE_WIFI      0

/* 连续多少次超阈值才真正报警，用来减少偶发抖动导致的误报警。 */
#define ALARM_CONFIRM_COUNT  2U

#define APP_ENABLE_FATFS     1

/* 默认阈值；按键或串口修改后会保存到内部 Flash。 */
#define DS18B20_LOW_DEFAULT_C   10.0f
#define DS18B20_HIGH_DEFAULT_C  60.0f
#define PT100_LOW_DEFAULT_C      0.0f
#define PT100_HIGH_DEFAULT_C   100.0f
#define TEMP_STEP_C          1.0f

/* PT100 模块按用户给出的关系式换算：温度 = 51.2 * ADC 电压。 */
#define PT100_ADC_VREF       3.3f
#define PT100_ADC_MAX        4095.0f
#define PT100_TEMP_PER_VOLT  51.2f

/* ESP-01S/OneNET 参数占位；开启云服务时再填写。 */
#define WIFI_SSID            "YOUR_WIFI_SSID"
#define WIFI_PASSWORD        "YOUR_WIFI_PASSWORD"
#define ONENET_MQTT_HOST     "mqtts.heclouds.com"
#define ONENET_MQTT_PORT     1883
#define ONENET_MQTT_PORT_STR "1883"
#define ONENET_PRODUCT_ID    "YOUR_PRODUCT_ID"
#define ONENET_DEVICE_NAME   "YOUR_DEVICE_NAME"
#define ONENET_DEVICE_TOKEN  "YOUR_DEVICE_TOKEN"

#endif
