#include "app_esp8266.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef *esp_uart;

/* 发送 ESP-01S AT 命令。当前实现用于完整流程演示。 */
static uint8_t SendCommand(const char *cmd, uint32_t delay_ms)
{
    HAL_UART_Transmit(esp_uart, (uint8_t *)cmd, (uint16_t)strlen(cmd), 1000);
    HAL_Delay(delay_ms);
    return 1;
}

/* 通过 AT+CIPSEND 发送 MQTT 原始报文。 */
static void SendRaw(const uint8_t *data, uint16_t len)
{
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u\r\n", len);
    SendCommand(cmd, 200);
    HAL_UART_Transmit(esp_uart, (uint8_t *)data, len, 3000);
    HAL_Delay(500);
}

/* MQTT 字符串字段编码：2 字节长度 + 字符串内容。 */
static uint16_t MqttWriteString(uint8_t *buf, uint16_t pos, const char *text)
{
    uint16_t len = (uint16_t)strlen(text);
    buf[pos++] = (uint8_t)(len >> 8);
    buf[pos++] = (uint8_t)(len & 0xff);
    memcpy(&buf[pos], text, len);
    return (uint16_t)(pos + len);
}

/* MQTT 剩余长度字段编码。 */
static uint16_t MqttWriteRemainingLength(uint8_t *buf, uint16_t pos, uint16_t len)
{
    uint8_t encoded;

    do
    {
        encoded = (uint8_t)(len % 128U);
        len = (uint16_t)(len / 128U);
        if (len > 0U)
            encoded |= 0x80U;
        buf[pos++] = encoded;
    } while (len > 0U);
    return pos;
}

/* 发送 MQTT CONNECT 报文。 */
static void MQTT_Connect(void)
{
    uint8_t packet[256];
    uint8_t body[220];
    uint16_t body_len = 0;
    uint16_t packet_len = 0;

    body_len = MqttWriteString(body, body_len, "MQTT");
    body[body_len++] = 4;
    body[body_len++] = 0xC2;
    body[body_len++] = 0;
    body[body_len++] = 60;
    body_len = MqttWriteString(body, body_len, ONENET_DEVICE_NAME);
    body_len = MqttWriteString(body, body_len, ONENET_PRODUCT_ID);
    body_len = MqttWriteString(body, body_len, ONENET_DEVICE_TOKEN);

    packet[packet_len++] = 0x10;
    packet_len = MqttWriteRemainingLength(packet, packet_len, body_len);
    memcpy(&packet[packet_len], body, body_len);
    SendRaw(packet, (uint16_t)(packet_len + body_len));
}

/* ESP-01S 初始化：AT、关回显、连 WiFi、连 MQTT 服务器。 */
uint8_t ESP8266_Init(UART_HandleTypeDef *huart)
{
    char cmd[160];

    esp_uart = huart;
    SendCommand("AT\r\n", 300);
    SendCommand("ATE0\r\n", 300);
    SendCommand("AT+CWMODE=1\r\n", 300);

    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
    SendCommand(cmd, 5000);
    SendCommand("AT+CIPMUX=0\r\n", 300);
    SendCommand("AT+CIPSTART=\"TCP\",\"" ONENET_MQTT_HOST "\"," ONENET_MQTT_PORT_STR "\r\n", 3000);
    MQTT_Connect();

    return 1;
}

/* 上传一帧温度和报警状态；云服务默认关闭，打开后由 app.c 周期调用。 */
uint8_t ESP8266_Upload(float ds_temp, float pt_temp,
                       float ds_low, float ds_high,
                       float pt_low, float pt_high,
                       uint8_t alarm)
{
    uint8_t packet[300];
    char topic[96];
    char payload[160];
    uint16_t topic_len;
    uint16_t payload_len;
    uint16_t remain_len;
    uint16_t packet_len = 0;

    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post",
             ONENET_PRODUCT_ID, ONENET_DEVICE_NAME);
    snprintf(payload, sizeof(payload),
             "{\"ds18b20\":%.2f,\"pt100\":%.2f,\"ds_low\":%.2f,\"ds_high\":%.2f,\"pt_low\":%.2f,\"pt_high\":%.2f,\"alarm\":%u}",
             ds_temp, pt_temp, ds_low, ds_high, pt_low, pt_high, alarm);

    topic_len = (uint16_t)strlen(topic);
    payload_len = (uint16_t)strlen(payload);
    remain_len = (uint16_t)(2U + topic_len + payload_len);

    packet[packet_len++] = 0x30;
    packet_len = MqttWriteRemainingLength(packet, packet_len, remain_len);
    packet_len = MqttWriteString(packet, packet_len, topic);
    memcpy(&packet[packet_len], payload, payload_len);
    SendRaw(packet, (uint16_t)(packet_len + payload_len));
    return 1;
}
