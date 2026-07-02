#ifndef APP_STORAGE_H
#define APP_STORAGE_H

#include <stdint.h>

uint8_t Storage_Init(void);
uint8_t Storage_Log(float ds_temp, float pt_temp,
                    float ds_low, float ds_high,
                    float pt_low, float pt_high,
                    uint8_t ds_alarm, uint8_t pt_alarm,
                    uint8_t alarm);
uint8_t Storage_CacheCloud(float ds_temp, float pt_temp, uint8_t alarm);
uint8_t Storage_ClearCloudCache(void);

#endif
