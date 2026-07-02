#ifndef APP_PT100_H
#define APP_PT100_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

uint8_t PT100_ReadTemperature(ADC_HandleTypeDef *hadc, float *temperature_c);

#endif
