#include "app.h"
#include "app_alarm.h"
#include "app_config.h"
#include "app_ds18b20.h"
#include "app_esp8266.h"
#include "app_keys.h"
#include "app_oled.h"
#include "app_pt100.h"
#include "app_settings.h"
#include "app_storage.h"
#include <stdio.h>
#include <string.h>

extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* 运行时状态：当前温度、最高最低值、传感器是否有效。 */
static float ds_temp_c = 0.0f;
static float pt_temp_c = 0.0f;
static float ds_min_c = 0.0f;
static float ds_max_c = 0.0f;
static float pt_min_c = 0.0f;
static float pt_max_c = 0.0f;
static uint8_t ds_valid = 0;
static uint8_t pt_valid = 0;
static uint8_t ds_range_valid = 0;
static uint8_t pt_range_valid = 0;

/* 阈值默认来自 app_config.h，开机后会优先读取 Flash 中保存的值。 */
static float ds_low_limit_c = DS18B20_LOW_DEFAULT_C;
static float ds_high_limit_c = DS18B20_HIGH_DEFAULT_C;
static float pt_low_limit_c = PT100_LOW_DEFAULT_C;
static float pt_high_limit_c = PT100_HIGH_DEFAULT_C;

/* 报警状态拆分为 DS 报警、PT 报警、总报警和手动静音。 */
static uint8_t ds_alarm = 0;
static uint8_t pt_alarm = 0;
static uint8_t alarm_condition = 0;
static uint8_t alarm_muted = 0;
static uint8_t sd_ok = 0;
static uint8_t wifi_ok = 0;
static uint8_t selected_limit = 0;
static uint8_t ds_alarm_count = 0;
static uint8_t pt_alarm_count = 0;
static uint16_t sd_error_count = 0;

/* 各任务的非阻塞定时器，主循环中按时间片执行。 */
static uint32_t last_sample_ms = 0;
static uint32_t last_display_ms = 0;
static uint32_t last_log_ms = 0;
#if APP_ENABLE_WIFI
static uint32_t last_wifi_ms = 0;
#endif
static uint32_t last_heartbeat_ms = 0;

static void Debug_Print(const char *msg)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)strlen(msg), 100);
}

/* 从内部 Flash 读取上次保存的阈值；没有有效记录时继续使用默认值。 */
static void LoadThresholds(void)
{
    AppSettings settings;

    if (Settings_Load(&settings))
    {
        ds_low_limit_c = settings.ds_low;
        ds_high_limit_c = settings.ds_high;
        pt_low_limit_c = settings.pt_low;
        pt_high_limit_c = settings.pt_high;
    }
}

/* 保存当前 4 个阈值到内部 Flash，实现掉电不丢失。 */
static void SaveThresholds(void)
{
    AppSettings settings;

    settings.ds_low = ds_low_limit_c;
    settings.ds_high = ds_high_limit_c;
    settings.pt_low = pt_low_limit_c;
    settings.pt_high = pt_high_limit_c;
    (void)Settings_Save(&settings);
}

/* 更新开机以来的最低/最高温度。 */
static void UpdateRange(float value, uint8_t *valid, float *min_value, float *max_value)
{
    if (!*valid)
    {
        *min_value = value;
        *max_value = value;
        *valid = 1;
        return;
    }
    if (value < *min_value)
        *min_value = value;
    if (value > *max_value)
        *max_value = value;
}

/* 根据当前选中的阈值项执行加/减 1 C。 */
static void ApplyThresholdStep(int8_t direction)
{
    float step = (float)direction * TEMP_STEP_C;

    if (selected_limit == 0U)
        ds_low_limit_c += step;
    else if (selected_limit == 1U)
        ds_high_limit_c += step;
    else if (selected_limit == 2U)
        pt_low_limit_c += step;
    else
        pt_high_limit_c += step;
}

/* 避免低阈值被调到高阈值上方。 */
static void NormalizeThresholds(void)
{
    float t;

    if (ds_low_limit_c > ds_high_limit_c)
    {
        t = ds_low_limit_c;
        ds_low_limit_c = ds_high_limit_c;
        ds_high_limit_c = t;
    }
    if (pt_low_limit_c > pt_high_limit_c)
    {
        t = pt_low_limit_c;
        pt_low_limit_c = pt_high_limit_c;
        pt_high_limit_c = t;
    }
}

/* 从串口命令中提取整数，例如 SET DSL 10 中的 10。 */
static int16_t ParseInt(const char *text)
{
    int16_t value = 0;
    int8_t sign = 1;

    while (*text && (*text < '0' || *text > '9') && *text != '-')
        text++;
    if (*text == '-')
    {
        sign = -1;
        text++;
    }
    while (*text >= '0' && *text <= '9')
    {
        value = (int16_t)(value * 10 + (*text - '0'));
        text++;
    }
    return (int16_t)(value * sign);
}

/* 串口 SET 命令入口，支持 DSL/DSH/PTL/PTH 四个阈值。 */
static uint8_t Serial_SetThreshold(const char *name, int16_t value)
{
    if (strncmp(name, "DSL", 3) == 0)
        ds_low_limit_c = (float)value;
    else if (strncmp(name, "DSH", 3) == 0)
        ds_high_limit_c = (float)value;
    else if (strncmp(name, "PTL", 3) == 0)
        pt_low_limit_c = (float)value;
    else if (strncmp(name, "PTH", 3) == 0)
        pt_high_limit_c = (float)value;
    else
        return 0;

    NormalizeThresholds();
    SaveThresholds();
    return 1;
}

/* 处理一整行串口命令。 */
static void Serial_HandleLine(char *line)
{
    char msg[160];

    for (uint8_t i = 0; line[i]; i++)
    {
        if (line[i] >= 'a' && line[i] <= 'z')
            line[i] = (char)(line[i] - 32);
    }

    if (strncmp(line, "GET", 3) == 0)
    {
        snprintf(msg, sizeof(msg), "DSL=%.0f DSH=%.0f PTL=%.0f PTH=%.0f SD=%u SE=%u DA=%u PA=%u AL=%u\r\n",
                 ds_low_limit_c, ds_high_limit_c, pt_low_limit_c, pt_high_limit_c,
                 sd_ok, sd_error_count, ds_alarm, pt_alarm, alarm_condition);
        Debug_Print(msg);
    }
    else if (strncmp(line, "SET ", 4) == 0)
    {
        if (Serial_SetThreshold(&line[4], ParseInt(&line[7])))
            Debug_Print("OK\r\n");
        else
            Debug_Print("ERR\r\n");
    }
    else if (strncmp(line, "RESET", 5) == 0)
    {
        ds_low_limit_c = DS18B20_LOW_DEFAULT_C;
        ds_high_limit_c = DS18B20_HIGH_DEFAULT_C;
        pt_low_limit_c = PT100_LOW_DEFAULT_C;
        pt_high_limit_c = PT100_HIGH_DEFAULT_C;
        SaveThresholds();
        Debug_Print("OK\r\n");
    }
    else if (strncmp(line, "MUTE", 4) == 0)
    {
        if (alarm_condition)
            alarm_muted = (uint8_t)!alarm_muted;
        Debug_Print("OK\r\n");
    }
    else
    {
        Debug_Print("CMD: GET | SET DSL 10 | SET DSH 60 | SET PTL 0 | SET PTH 100 | RESET | MUTE\r\n");
    }
}

/* 非阻塞串口接收任务；没有数据时立即返回，不影响测温。 */
static void Serial_Task(void)
{
    static char rx_line[48];
    static uint8_t rx_len = 0;
    uint8_t ch;

    while (HAL_UART_Receive(&huart1, &ch, 1, 0) == HAL_OK)
    {
        if (ch == '\r' || ch == '\n')
        {
            if (rx_len > 0U)
            {
                rx_line[rx_len] = '\0';
                Serial_HandleLine(rx_line);
                rx_len = 0;
            }
        }
        else if (rx_len < sizeof(rx_line) - 1U)
        {
            rx_line[rx_len++] = (char)ch;
        }
    }
}

void App_Init(void)
{
    float ds;
    float pt;

    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ALARM_LED_PORT, ALARM_LED_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BOARD_LED_PORT, BOARD_LED_PIN, GPIO_PIN_SET);
    LoadThresholds();

    /* 开机自检界面，便于现场确认各模块是否正常。 */
    OLED_Init(&hi2c1);
    OLED_Clear();
    OLED_ShowString(0, 0, "SELF TEST");
    OLED_ShowString(1, 0, "OLED OK");
    pt_valid = PT100_ReadTemperature(&hadc1, &pt);
    if (pt_valid)
    {
        pt_temp_c = pt;
        UpdateRange(pt_temp_c, &pt_range_valid, &pt_min_c, &pt_max_c);
    }
    OLED_ShowString(2, 0, pt_valid ? "PT OK" : "PT ERR");
    ds_valid = DS18B20_ReadTemperature(&ds);
    if (ds_valid)
    {
        ds_temp_c = ds;
        UpdateRange(ds_temp_c, &ds_range_valid, &ds_min_c, &ds_max_c);
    }
    OLED_ShowString(3, 0, ds_valid ? "DS OK" : "DS ERR");
    sd_ok = Storage_Init();
    OLED_ShowString(4, 0, sd_ok ? "SD OK" : "SD ERR");
#if APP_ENABLE_WIFI
    wifi_ok = ESP8266_Init(&huart2);
    OLED_ShowString(5, 0, wifi_ok ? "WIFI OK" : "WIFI ERR");
#else
    wifi_ok = 0;
    OLED_ShowString(5, 0, "WIFI OFF");
#endif
    HAL_Delay(1200);
    OLED_Clear();

    Debug_Print("Temperature inspector started\r\n");
}

void App_Loop(void)
{
    uint32_t now = HAL_GetTick();
    KeyEvent key = Keys_Scan();
    char line[128];
    uint8_t thresholds_changed = 0;

    /* 串口命令、按键、采样、显示、记录都在主循环中分时执行。 */
    Serial_Task();

    if (key == KEY_1_2)
    {
        if (alarm_condition)
            alarm_muted = (uint8_t)!alarm_muted;
    }
    else if (key == KEY_3)
    {
        selected_limit = (uint8_t)((selected_limit + 1U) % 4U);
    }
    else if (key == KEY_1)
    {
        ApplyThresholdStep(-1);
        thresholds_changed = 1;
    }
    else if (key == KEY_2)
    {
        ApplyThresholdStep(1);
        thresholds_changed = 1;
    }
    if (thresholds_changed)
    {
        NormalizeThresholds();
        SaveThresholds();
    }

    /* 15 秒采样一次，并完成阈值判断、防误报警计数、串口输出。 */
    if ((now - last_sample_ms) >= SAMPLE_PERIOD_MS)
    {
        float ds;
        float pt;
        pt_valid = PT100_ReadTemperature(&hadc1, &pt);
        if (pt_valid)
        {
            pt_temp_c = pt;
            UpdateRange(pt_temp_c, &pt_range_valid, &pt_min_c, &pt_max_c);
        }

        ds_valid = DS18B20_ReadTemperature(&ds);
        if (ds_valid)
        {
            ds_temp_c = ds;
            UpdateRange(ds_temp_c, &ds_range_valid, &ds_min_c, &ds_max_c);
        }

        if (ds_valid && (ds_temp_c < ds_low_limit_c || ds_temp_c > ds_high_limit_c))
        {
            if (ds_alarm_count < ALARM_CONFIRM_COUNT)
                ds_alarm_count++;
        }
        else
        {
            ds_alarm_count = 0;
        }

        if (pt_valid && (pt_temp_c < pt_low_limit_c || pt_temp_c > pt_high_limit_c))
        {
            if (pt_alarm_count < ALARM_CONFIRM_COUNT)
                pt_alarm_count++;
        }
        else
        {
            pt_alarm_count = 0;
        }

        ds_alarm = (uint8_t)(ds_alarm_count >= ALARM_CONFIRM_COUNT);
        pt_alarm = (uint8_t)(pt_alarm_count >= ALARM_CONFIRM_COUNT);
        alarm_condition = (uint8_t)(ds_alarm || pt_alarm);
        if (!alarm_condition)
            alarm_muted = 0;
        last_sample_ms = now;

        snprintf(line, sizeof(line), "DS=%.2f %.1f/%.1f DA=%u PT=%.2f %.1f/%.1f PA=%u AL=%u M=%u\r\n",
                 ds_temp_c, ds_low_limit_c, ds_high_limit_c,
                 ds_alarm, pt_temp_c, pt_low_limit_c, pt_high_limit_c, pt_alarm, alarm_condition, alarm_muted);
        Debug_Print(line);
    }

    /* PC13 心跳灯，表示主循环仍在运行。 */
    if ((now - last_heartbeat_ms) >= 500U)
    {
        HAL_GPIO_TogglePin(BOARD_LED_PORT, BOARD_LED_PIN);
        last_heartbeat_ms = now;
    }

    /* 手动静音只关闭声光输出，不影响 OLED/SD 中的报警状态记录。 */
    Alarm_Task((uint8_t)(alarm_condition && !alarm_muted));

    /* OLED 内部会比较显示内容，内容不变就不会刷新，避免闪屏。 */
    if ((now - last_display_ms) >= DISPLAY_PERIOD_MS)
    {
        OLED_UpdateStatus(ds_temp_c, pt_temp_c,
                          ds_valid, pt_valid,
                          ds_low_limit_c, ds_high_limit_c,
                          pt_low_limit_c, pt_high_limit_c,
                          ds_alarm, pt_alarm, alarm_condition,
                          alarm_muted, selected_limit, sd_ok, wifi_ok, sd_error_count,
                          ds_min_c, ds_max_c, pt_min_c, pt_max_c);
        last_display_ms = now;
    }

    /* SD 记录失败后统计错误次数，并在下一周期尝试重新挂载。 */
    if ((now - last_log_ms) >= LOG_PERIOD_MS)
    {
        if (!sd_ok)
            sd_ok = Storage_Init();
        if (sd_ok && !Storage_Log(ds_temp_c, pt_temp_c,
                                  ds_low_limit_c, ds_high_limit_c,
                                  pt_low_limit_c, pt_high_limit_c,
                                  ds_alarm, pt_alarm, alarm_condition))
        {
            sd_error_count++;
            sd_ok = 0;
        }
        last_log_ms = now;
    }

#if APP_ENABLE_WIFI
    /* 云服务流程默认关闭；开启后上传失败会缓存到 SD 卡。 */
    if ((now - last_wifi_ms) >= WIFI_PERIOD_MS)
    {
        if (!wifi_ok)
            wifi_ok = ESP8266_Init(&huart2);

        if (wifi_ok && ESP8266_Upload(ds_temp_c, pt_temp_c,
                                      ds_low_limit_c, ds_high_limit_c,
                                      pt_low_limit_c, pt_high_limit_c,
                                      alarm_condition))
        {
            (void)Storage_ClearCloudCache();
            wifi_ok = 1;
        }
        else
        {
            (void)Storage_CacheCloud(ds_temp_c, pt_temp_c, alarm_condition);
            wifi_ok = 0;
        }
        last_wifi_ms = now;
    }
#endif
}
