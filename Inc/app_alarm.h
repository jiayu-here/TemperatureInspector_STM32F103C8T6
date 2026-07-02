#ifndef APP_ALARM_H
#define APP_ALARM_H

#include <stdint.h>

void Alarm_Set(uint8_t enabled);
void Alarm_Task(uint8_t enabled);

#endif

