#ifndef APP_OLED_H
#define APP_OLED_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

void OLED_Init(I2C_HandleTypeDef *hi2c);
void OLED_Clear(void);
void OLED_ShowString(uint8_t row, uint8_t col, const char *text);
void OLED_UpdateStatus(float ds_temp, float pt_temp,
                       uint8_t ds_valid, uint8_t pt_valid,
                       float ds_low, float ds_high,
                       float pt_low, float pt_high,
                       uint8_t ds_alarm, uint8_t pt_alarm, uint8_t alarm,
                       uint8_t muted, uint8_t selected_limit,
                       uint8_t sd_ok, uint8_t wifi_ok, uint16_t sd_errors,
                       float ds_min, float ds_max, float pt_min, float pt_max);

#endif
