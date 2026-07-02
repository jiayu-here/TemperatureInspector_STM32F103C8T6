#include "app_ds18b20.h"
#include "app_config.h"

/* 微秒级延时，DS18B20 单总线时序需要用到。 */
static void DelayUs(uint16_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < ticks) {}
}

/* DS18B20 数据线切为开漏输出模式。 */
static void DQ_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

/* DS18B20 数据线释放为输入模式，用于读取传感器响应。 */
static void DQ_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

/* 复位总线并检测 DS18B20 是否存在。 */
static uint8_t DS18B20_Reset(void)
{
    uint8_t presence;
    DQ_Output();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    DelayUs(480);
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    DQ_Input();
    DelayUs(70);
    presence = (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_RESET);
    DelayUs(410);
    return presence;
}

/* 按单总线时序写 1 bit。 */
static void DS18B20_WriteBit(uint8_t bit)
{
    DQ_Output();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    if (bit)
    {
        DelayUs(6);
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        DelayUs(64);
    }
    else
    {
        DelayUs(60);
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        DelayUs(10);
    }
}

/* 按单总线时序读 1 bit。 */
static uint8_t DS18B20_ReadBit(void)
{
    uint8_t bit;
    DQ_Output();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    DelayUs(6);
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    DQ_Input();
    DelayUs(9);
    bit = (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_SET);
    DelayUs(55);
    return bit;
}

/* 写 1 字节，低位先发送。 */
static void DS18B20_WriteByte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        DS18B20_WriteBit(data & 0x01U);
        data >>= 1;
    }
}

/* 读 1 字节，低位先接收。 */
static uint8_t DS18B20_ReadByte(void)
{
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        data >>= 1;
        if (DS18B20_ReadBit())
            data |= 0x80U;
    }
    return data;
}

/* scratchpad CRC8 校验，避免错误数据被显示成温度。 */
static uint8_t DS18B20_Crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++)
    {
        uint8_t inbyte = data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            uint8_t mix = (crc ^ inbyte) & 0x01U;
            crc >>= 1;
            if (mix)
                crc ^= 0x8CU;
            inbyte >>= 1;
        }
    }
    return crc;
}

/* 启动温度转换并读取结果；失败返回 0，OLED 显示 DS:ERR。 */
uint8_t DS18B20_ReadTemperature(float *temperature_c)
{
    uint8_t scratchpad[9];
    int16_t raw;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    if (!DS18B20_Reset())
        return 0;
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0x44);
    HAL_Delay(750);

    if (!DS18B20_Reset())
        return 0;
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0xBE);

    for (uint8_t i = 0; i < 9; i++)
        scratchpad[i] = DS18B20_ReadByte();

    if (DS18B20_Crc8(scratchpad, 8) != scratchpad[8])
        return 0;

    raw = (int16_t)(((uint16_t)scratchpad[1] << 8) | scratchpad[0]);
    if (raw == (int16_t)0xFFFF || raw == (int16_t)0x7FFF)
        return 0;

    *temperature_c = raw * 0.0625f;
    return 1;
}
