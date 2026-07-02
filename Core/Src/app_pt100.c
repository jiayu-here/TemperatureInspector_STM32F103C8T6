#include "app_pt100.h"
#include "app_config.h"

/* 读取 PA1 的 ADC 电压，并按 T = 51.2 * V 换算为 PT 温度。 */
uint8_t PT100_ReadTemperature(ADC_HandleTypeDef *hadc, float *temperature_c)
{
    uint32_t adc = 0;
    float voltage;

    HAL_ADC_Start(hadc);
    if (HAL_ADC_PollForConversion(hadc, 20) != HAL_OK)
    {
        HAL_ADC_Stop(hadc);
        return 0;
    }
    adc = HAL_ADC_GetValue(hadc);
    HAL_ADC_Stop(hadc);

    voltage = ((float)adc * PT100_ADC_VREF) / PT100_ADC_MAX;
    *temperature_c = PT100_TEMP_PER_VOLT * voltage;
    return 1;
}
