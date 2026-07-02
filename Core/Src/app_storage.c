#include "app_storage.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>

#if APP_ENABLE_FATFS
#include "ff.h"
static FATFS sd_fs;

/* 将温度转换成 0.1 C 单位，方便手工拼接 CSV 字符串。 */
static int32_t temp_to_x10(float temp)
{
    return (int32_t)((temp >= 0.0f) ? (temp * 10.0f + 0.5f) : (temp * 10.0f - 0.5f));
}

/* 把 0.1 C 单位的温度写成类似 25.3 的字符串。 */
static void append_temp(char *dst, size_t size, int32_t value_x10)
{
    int32_t whole = value_x10 / 10;
    int32_t frac = value_x10 % 10;

    if (frac < 0)
    {
        frac = -frac;
    }
    snprintf(dst, size, "%ld.%ld", (long)whole, (long)frac);
}
#endif

/* 挂载 SD 卡文件系统。 */
uint8_t Storage_Init(void)
{
#if APP_ENABLE_FATFS
    return (f_mount(&sd_fs, "0:", 1) == FR_OK);
#else
    return 0;
#endif
}

/* 追加一条温度记录；新建文件时自动写 CSV 表头。 */
uint8_t Storage_Log(float ds_temp, float pt_temp,
                    float ds_low, float ds_high,
                    float pt_low, float pt_high,
                    uint8_t ds_alarm, uint8_t pt_alarm,
                    uint8_t alarm)
{
#if APP_ENABLE_FATFS
    static FIL file;
    UINT written = 0;
    DWORD file_size;
    char line[128];
    char ds_buf[12];
    char pt_buf[12];
    char ds_low_buf[12];
    char ds_high_buf[12];
    char pt_low_buf[12];
    char pt_high_buf[12];

    if (f_open(&file, "0:/temp_log.csv", FA_OPEN_ALWAYS | FA_WRITE) != FR_OK)
    {
        return 0;
    }
    file_size = f_size(&file);
    if (file_size == 0U)
    {
        const char *header = "time_ms,ds18b20,pt100,ds_low,ds_high,pt_low,pt_high,ds_alarm,pt_alarm,alarm\r\n";
        if (f_write(&file, header, strlen(header), &written) != FR_OK || written != strlen(header))
        {
            f_close(&file);
            return 0;
        }
    }
    if (f_lseek(&file, f_size(&file)) != FR_OK)
    {
        f_close(&file);
        return 0;
    }

    append_temp(ds_buf, sizeof(ds_buf), temp_to_x10(ds_temp));
    append_temp(pt_buf, sizeof(pt_buf), temp_to_x10(pt_temp));
    append_temp(ds_low_buf, sizeof(ds_low_buf), temp_to_x10(ds_low));
    append_temp(ds_high_buf, sizeof(ds_high_buf), temp_to_x10(ds_high));
    append_temp(pt_low_buf, sizeof(pt_low_buf), temp_to_x10(pt_low));
    append_temp(pt_high_buf, sizeof(pt_high_buf), temp_to_x10(pt_high));

    snprintf(line, sizeof(line), "%lu,%s,%s,%s,%s,%s,%s,%u,%u,%u\r\n",
             HAL_GetTick(), ds_buf, pt_buf, ds_low_buf, ds_high_buf,
             pt_low_buf, pt_high_buf, ds_alarm, pt_alarm, alarm);
    if (f_write(&file, line, strlen(line), &written) != FR_OK)
    {
        f_close(&file);
        return 0;
    }
    f_close(&file);
    return written == strlen(line);
#else
    (void)ds_temp;
    (void)pt_temp;
    (void)ds_low;
    (void)ds_high;
    (void)pt_low;
    (void)pt_high;
    (void)ds_alarm;
    (void)pt_alarm;
    (void)alarm;
    return 0;
#endif
}

/* 云服务上传失败时，把待上传数据缓存到 SD 卡。 */
uint8_t Storage_CacheCloud(float ds_temp, float pt_temp, uint8_t alarm)
{
#if APP_ENABLE_FATFS
    FIL file;
    UINT written = 0;
    char line[64];
    char ds_buf[12];
    char pt_buf[12];

    if (f_open(&file, "0:/cloud_cache.csv", FA_OPEN_ALWAYS | FA_WRITE) != FR_OK)
        return 0;
    if (f_lseek(&file, f_size(&file)) != FR_OK)
    {
        f_close(&file);
        return 0;
    }

    append_temp(ds_buf, sizeof(ds_buf), temp_to_x10(ds_temp));
    append_temp(pt_buf, sizeof(pt_buf), temp_to_x10(pt_temp));
    snprintf(line, sizeof(line), "%lu,%s,%s,%u\r\n", HAL_GetTick(), ds_buf, pt_buf, alarm);
    if (f_write(&file, line, strlen(line), &written) != FR_OK)
    {
        f_close(&file);
        return 0;
    }
    f_close(&file);
    return written == strlen(line);
#else
    (void)ds_temp;
    (void)pt_temp;
    (void)alarm;
    return 0;
#endif
}

/* 云服务恢复后清除缓存文件。 */
uint8_t Storage_ClearCloudCache(void)
{
#if APP_ENABLE_FATFS
    return f_unlink("0:/cloud_cache.csv") == FR_OK || f_unlink("0:/cloud_cache.csv") == FR_NO_FILE;
#else
    return 0;
#endif
}
