#ifndef APP_ESP8266_H
#define APP_ESP8266_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

uint8_t ESP8266_Init(UART_HandleTypeDef *huart);
uint8_t ESP8266_Upload(float ds_temp, float pt_temp,
                       float ds_low, float ds_high,
                       float pt_low, float pt_high,
                       uint8_t alarm);

#endif
