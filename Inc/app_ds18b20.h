#ifndef APP_DS18B20_H
#define APP_DS18B20_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

uint8_t DS18B20_ReadTemperature(float *temperature_c);

#endif

