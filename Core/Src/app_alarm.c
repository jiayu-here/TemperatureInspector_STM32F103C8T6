#include "app_alarm.h"
#include "app_config.h"

/* 直接控制报警灯和蜂鸣器，高电平有效。 */
void Alarm_Set(uint8_t enabled)
{
    HAL_GPIO_WritePin(ALARM_LED_PORT, ALARM_LED_PIN, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* 报警节奏任务：报警有效时每 300ms 翻转一次，实现闪灯/间歇蜂鸣。 */
void Alarm_Task(uint8_t enabled)
{
    static uint32_t last_toggle = 0;
    static uint8_t state = 0;

    if (!enabled)
    {
        state = 0;
        Alarm_Set(0);
        return;
    }

    if ((HAL_GetTick() - last_toggle) >= 200U)
    {
        state ^= 1U;
        Alarm_Set(state);
        last_toggle = HAL_GetTick();
    }
}
