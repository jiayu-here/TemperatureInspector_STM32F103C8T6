#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <stdint.h>

typedef struct
{
    float ds_low;
    float ds_high;
    float pt_low;
    float pt_high;
} AppSettings;

uint8_t Settings_Load(AppSettings *settings);
uint8_t Settings_Save(const AppSettings *settings);

#endif
