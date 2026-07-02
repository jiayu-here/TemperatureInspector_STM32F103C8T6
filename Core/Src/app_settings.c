#include "app_settings.h"
#include "app_config.h"
#include "stm32f1xx_hal.h"
#include <string.h>

/* 使用 64KB Flash 的最后一页保存阈值配置。 */
#define SETTINGS_FLASH_ADDR 0x0800FC00UL
#define SETTINGS_MAGIC      0x54494D50UL

typedef struct
{
    uint32_t magic;
    AppSettings settings;
    uint32_t checksum;
} SettingsRecord;

/* 简单校验和，用来判断 Flash 中的配置是否完整有效。 */
static uint32_t CalcChecksum(const AppSettings *settings)
{
    const uint8_t *bytes = (const uint8_t *)settings;
    uint32_t sum = 0x13572468UL;

    for (uint32_t i = 0; i < sizeof(AppSettings); i++)
    {
        sum = (sum << 5) | (sum >> 27);
        sum += bytes[i];
    }
    return sum;
}

/* 限制阈值范围，避免 Flash 中异常数据被当成有效配置。 */
static uint8_t SettingsValid(const AppSettings *settings)
{
    return settings->ds_low <= settings->ds_high &&
           settings->pt_low <= settings->pt_high &&
           settings->ds_low > -100.0f && settings->ds_high < 150.0f &&
           settings->pt_low > -100.0f && settings->pt_high < 250.0f;
}

/* 读取保存的阈值配置。 */
uint8_t Settings_Load(AppSettings *settings)
{
    const SettingsRecord *record = (const SettingsRecord *)SETTINGS_FLASH_ADDR;

    if (record->magic != SETTINGS_MAGIC)
    {
        return 0;
    }
    if (record->checksum != CalcChecksum(&record->settings))
    {
        return 0;
    }
    if (!SettingsValid(&record->settings))
    {
        return 0;
    }

    memcpy(settings, &record->settings, sizeof(AppSettings));
    return 1;
}

/* 擦除最后一页 Flash，然后写入新的阈值配置。 */
uint8_t Settings_Save(const AppSettings *settings)
{
    SettingsRecord record;
    const uint16_t *halfwords = (const uint16_t *)&record;
    uint32_t address = SETTINGS_FLASH_ADDR;
    uint32_t count = sizeof(SettingsRecord) / sizeof(uint16_t);
    FLASH_EraseInitTypeDef erase;
    uint32_t page_error = 0;
    uint8_t ok = 1;

    if (!SettingsValid(settings))
    {
        return 0;
    }

    record.magic = SETTINGS_MAGIC;
    record.settings = *settings;
    record.checksum = CalcChecksum(settings);

    HAL_FLASH_Unlock();

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = SETTINGS_FLASH_ADDR;
    erase.NbPages = 1;
    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK)
    {
        ok = 0;
    }

    for (uint32_t i = 0; ok && i < count; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address, halfwords[i]) != HAL_OK)
        {
            ok = 0;
        }
        address += 2U;
    }

    HAL_FLASH_Lock();
    return ok;
}
