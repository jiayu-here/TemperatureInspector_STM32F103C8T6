#include "app_oled.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>

static I2C_HandleTypeDef *oled_i2c;
static uint8_t buffer[1024];

/* 5x7 ASCII 字库，只保留本项目 OLED 会用到的字符，节省 Flash 空间。 */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},{0x14,0x7F,0x14,0x7F,0x14},
    {0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},
    {0x00,0x1C,0x22,0x41,0x00},{0x00,0x41,0x22,0x1C,0x00},{0x14,0x08,0x3E,0x08,0x14},{0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},{0x20,0x10,0x08,0x04,0x02},
    {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},{0x00,0x56,0x36,0x00,0x00},
    {0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},{0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},
    {0x32,0x49,0x79,0x41,0x3E},{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},{0x3E,0x41,0x49,0x49,0x7A},
    {0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40},{0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63},{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43}
};

static void WriteCommand(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};
    HAL_I2C_Master_Transmit(oled_i2c, OLED_I2C_ADDR, data, 2, 100);
}

/* 将 1024 字节显存一次性写入 OLED，避免多次清屏导致闪烁。 */
static void Flush(void)
{
    for (uint8_t page = 0; page < 8; page++)
    {
        WriteCommand(0xB0 + page);
        WriteCommand(0x00);
        WriteCommand(0x10);
        uint8_t data[129];
        data[0] = 0x40;
        memcpy(&data[1], &buffer[page * 128], 128);
        HAL_I2C_Master_Transmit(oled_i2c, OLED_I2C_ADDR, data, 129, 200);
    }
}

/* SSD1306 初始化序列，OLED 地址在 app_config.h 中配置。 */
void OLED_Init(I2C_HandleTypeDef *hi2c)
{
    oled_i2c = hi2c;
    HAL_Delay(100);
    WriteCommand(0xAE);
    WriteCommand(0x20); WriteCommand(0x00);
    WriteCommand(0xB0);
    WriteCommand(0xC8);
    WriteCommand(0x00);
    WriteCommand(0x10);
    WriteCommand(0x40);
    WriteCommand(0x81); WriteCommand(0x7F);
    WriteCommand(0xA1);
    WriteCommand(0xA6);
    WriteCommand(0xA8); WriteCommand(0x3F);
    WriteCommand(0xA4);
    WriteCommand(0xD3); WriteCommand(0x00);
    WriteCommand(0xD5); WriteCommand(0x80);
    WriteCommand(0xD9); WriteCommand(0xF1);
    WriteCommand(0xDA); WriteCommand(0x12);
    WriteCommand(0xDB); WriteCommand(0x40);
    WriteCommand(0x8D); WriteCommand(0x14);
    WriteCommand(0xAF);
    OLED_Clear();
}

void OLED_Clear(void)
{
    memset(buffer, 0, sizeof(buffer));
    Flush();
}

/* 只修改显存，不立刻刷新；状态界面最后统一 Flush。 */
static void DrawString(uint8_t row, uint8_t col, const char *text)
{
    uint16_t x = col * 6U;
    uint16_t page = row;
    while (*text && x < 123U && page < 8U)
    {
        char c = *text++;
        if (c < 32 || c > 90) c = ' ';
        const uint8_t *glyph = font5x7[c - 32];
        for (uint8_t i = 0; i < 5; i++)
            buffer[page * 128U + x + i] = glyph[i];
        buffer[page * 128U + x + 5U] = 0x00;
        x += 6U;
    }
}

/* 单独显示字符串时使用，会立即刷新一次。 */
void OLED_ShowString(uint8_t row, uint8_t col, const char *text)
{
    DrawString(row, col, text);
    Flush();
}

/* 将浮点温度转为 0.1 C 单位，避免依赖 printf 浮点格式。 */
static int16_t TempToTenth(float value)
{
    if (value >= 0.0f)
        return (int16_t)(value * 10.0f + 0.5f);
    return (int16_t)(value * 10.0f - 0.5f);
}

/* 格式化 DS/PT 当前温度；传感器无效时显示 ERR。 */
static void FormatTemp(char *line, size_t size, const char *name, float value, uint8_t valid)
{
    int16_t t;
    int16_t abs_t;

    if (!valid)
    {
        snprintf(line, size, "%s:ERR", name);
        return;
    }

    t = TempToTenth(value);
    abs_t = (t < 0) ? (int16_t)-t : t;
    snprintf(line, size, "%s:%c%03d.%dC", name, (t < 0) ? '-' : '+', abs_t / 10, abs_t % 10);
}

/* 格式化上下限阈值。 */
static void FormatLimit(char *line, size_t size, const char *low_name, float low, const char *high_name, float high)
{
    snprintf(line, size, "%s:%03d %s:%03d", low_name, (int)low, high_name, (int)high);
}

/* 格式化开机以来的最低/最高温。 */
static void FormatRange(char *line, size_t size, const char *name, float min, float max)
{
    snprintf(line, size, "%s:%03d/%03d", name, (int)min, (int)max);
}

/* 返回当前按键正在调节的阈值名称。 */
static const char *SelectedName(uint8_t selected_limit)
{
    if (selected_limit == 0U)
        return "DSL";
    if (selected_limit == 1U)
        return "DSH";
    if (selected_limit == 2U)
        return "PTL";
    return "PTH";
}

/* 更新状态界面；内容没变化时直接返回，不刷新 OLED。 */
void OLED_UpdateStatus(float ds_temp, float pt_temp,
                       uint8_t ds_valid, uint8_t pt_valid,
                       float ds_low, float ds_high,
                       float pt_low, float pt_high,
                       uint8_t ds_alarm, uint8_t pt_alarm, uint8_t alarm,
                       uint8_t muted, uint8_t selected_limit,
                       uint8_t sd_ok, uint8_t wifi_ok, uint16_t sd_errors,
                       float ds_min, float ds_max, float pt_min, float pt_max)
{
    char lines[8][22];
    static char last_lines[8][22];

    FormatTemp(lines[0], sizeof(lines[0]), "DS", ds_temp, ds_valid);
    FormatTemp(lines[1], sizeof(lines[1]), "PT", pt_temp, pt_valid);
    FormatLimit(lines[2], sizeof(lines[2]), "DSL", ds_low, "DSH", ds_high);
    FormatLimit(lines[3], sizeof(lines[3]), "PTL", pt_low, "PTH", pt_high);
    snprintf(lines[4], sizeof(lines[4]), "DA:%u PA:%u AL:%u", ds_alarm, pt_alarm, alarm);
    snprintf(lines[5], sizeof(lines[5]), "M:%u %s SD:%u E:%u", muted, SelectedName(selected_limit), sd_ok, sd_errors);
    FormatRange(lines[6], sizeof(lines[6]), "D", ds_min, ds_max);
    FormatRange(lines[7], sizeof(lines[7]), "P", pt_min, pt_max);

    if (memcmp(lines, last_lines, sizeof(lines)) == 0)
    {
        return;
    }
    memcpy(last_lines, lines, sizeof(lines));

    memset(buffer, 0, sizeof(buffer));
    DrawString(0, 0, lines[0]);
    DrawString(1, 0, lines[1]);
    DrawString(2, 0, lines[2]);
    DrawString(3, 0, lines[3]);
    DrawString(4, 0, lines[4]);
    DrawString(5, 0, lines[5]);
    DrawString(6, 0, lines[6]);
    DrawString(7, 0, lines[7]);
    Flush();
}
