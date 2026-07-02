#include "app_keys.h"
#include "app_config.h"

static uint8_t IsPressed(GPIO_TypeDef *port, uint16_t pin)
{
    return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET;
}

/* 按键扫描：
 * KEY1/KEY2 短按调整阈值，长按 1 秒后每 250ms 连续调整；
 * KEY3 切换调节项；KEY1+KEY2 同时按用于报警静音。
 */
KeyEvent Keys_Scan(void)
{
    static KeyEvent held_key = KEY_NONE;
    static uint32_t press_start_ms = 0;
    static uint32_t last_repeat_ms = 0;
    uint32_t now = HAL_GetTick();
    KeyEvent current = KEY_NONE;
    uint8_t k1 = IsPressed(KEY1_PORT, KEY1_PIN);
    uint8_t k2 = IsPressed(KEY2_PORT, KEY2_PIN);
    uint8_t k3 = IsPressed(KEY3_PORT, KEY3_PIN);

    if (!k1 && !k2 && !k3)
    {
        held_key = KEY_NONE;
        return KEY_NONE;
    }

    if (k1 && k2)
        current = KEY_1_2;
    else if (k1)
        current = KEY_1;
    else if (k2)
        current = KEY_2;
    else if (k3)
        current = KEY_3;

    if (current != held_key)
    {
        HAL_Delay(20);
        held_key = current;
        press_start_ms = now;
        last_repeat_ms = now;
        return current;
    }

    if (current == KEY_1 || current == KEY_2)
    {
        if ((now - press_start_ms) >= 1000U && (now - last_repeat_ms) >= 250U)
        {
            last_repeat_ms = now;
            return current;
        }
    }

    return KEY_NONE;
}
