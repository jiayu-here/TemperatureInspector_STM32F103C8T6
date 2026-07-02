#include "diskio.h"
#include "app_config.h"

extern SPI_HandleTypeDef hspi2;

#define CARD_TYPE_UNKNOWN 0x00U
#define CARD_TYPE_MMC     0x01U
#define CARD_TYPE_SDV1    0x02U
#define CARD_TYPE_SDV2    0x04U
#define CARD_TYPE_BLOCK   0x08U

#define CMD0   (0x40U + 0U)
#define CMD1   (0x40U + 1U)
#define CMD8   (0x40U + 8U)
#define CMD9   (0x40U + 9U)
#define CMD12  (0x40U + 12U)
#define CMD16  (0x40U + 16U)
#define CMD17  (0x40U + 17U)
#define CMD24  (0x40U + 24U)
#define CMD55  (0x40U + 55U)
#define CMD58  (0x40U + 58U)
#define ACMD41 (0xC0U + 41U)

static DSTATUS sd_status = STA_NOINIT;
static BYTE card_type = CARD_TYPE_UNKNOWN;

/* SPI2 读写 1 字节，SD 卡 SPI 通信读写同时发生。 */
static BYTE spi_rw(BYTE data)
{
    BYTE rx = 0xFFU;
    HAL_SPI_TransmitReceive(&hspi2, &data, &rx, 1, 100);
    return rx;
}

/* PB12 拉低，选中 SD 卡。 */
static void cs_low(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET);
}

/* 释放 SD 卡片选，并补一个空时钟。 */
static void cs_high(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
    spi_rw(0xFFU);
}

/* 等待 SD 卡从忙状态恢复。 */
static int wait_ready(UINT timeout_ms)
{
    uint32_t start = HAL_GetTick();
    BYTE resp;

    do
    {
        resp = spi_rw(0xFFU);
        if (resp == 0xFFU)
        {
            return 1;
        }
    } while ((HAL_GetTick() - start) < timeout_ms);

    return 0;
}

/* 接收一个 SD 数据块。读扇区时 len 为 512。 */
static int receive_data(BYTE *buff, UINT len)
{
    uint32_t start = HAL_GetTick();
    BYTE token;

    do
    {
        token = spi_rw(0xFFU);
        if (token == 0xFEU)
        {
            while (len--)
            {
                *buff++ = spi_rw(0xFFU);
            }
            spi_rw(0xFFU);
            spi_rw(0xFFU);
            return 1;
        }
    } while ((HAL_GetTick() - start) < 200U);

    return 0;
}

/* 发送一个 SD 数据块。写扇区时 token 为 0xFE。 */
static int transmit_data(const BYTE *buff, BYTE token)
{
    BYTE resp;

    if (!wait_ready(500U))
    {
        return 0;
    }

    spi_rw(token);
    if (token != 0xFDU)
    {
        for (UINT i = 0; i < 512U; i++)
        {
            spi_rw(buff[i]);
        }
        spi_rw(0xFFU);
        spi_rw(0xFFU);
        resp = spi_rw(0xFFU);
        if ((resp & 0x1FU) != 0x05U)
        {
            return 0;
        }
    }

    return 1;
}

/* 发送 SD 命令，并读取 R1 响应。 */
static BYTE send_cmd(BYTE cmd, DWORD arg)
{
    BYTE crc = 0x01U;
    BYTE resp;

    if (cmd & 0x80U)
    {
        cmd &= 0x7FU;
        resp = send_cmd(CMD55, 0);
        if (resp > 1U)
        {
            return resp;
        }
    }

    cs_high();
    cs_low();
    if (!wait_ready(500U))
    {
        cs_high();
        return 0xFFU;
    }

    spi_rw(cmd);
    spi_rw((BYTE)(arg >> 24));
    spi_rw((BYTE)(arg >> 16));
    spi_rw((BYTE)(arg >> 8));
    spi_rw((BYTE)arg);

    if (cmd == CMD0)
    {
        crc = 0x95U;
    }
    else if (cmd == CMD8)
    {
        crc = 0x87U;
    }
    spi_rw(crc);

    if (cmd == CMD12)
    {
        spi_rw(0xFFU);
    }

    for (UINT i = 0; i < 10U; i++)
    {
        resp = spi_rw(0xFFU);
        if ((resp & 0x80U) == 0U)
        {
            return resp;
        }
    }

    return 0xFFU;
}

/* FatFs 查询磁盘状态接口。 */
DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0U)
    {
        return STA_NOINIT;
    }
    return sd_status;
}

/* SD 卡初始化，兼容 SDv1、SDv2 和 SDHC。 */
DSTATUS disk_initialize(BYTE pdrv)
{
    BYTE n;
    BYTE ocr[4];
    uint32_t start;
    BYTE type = CARD_TYPE_UNKNOWN;

    if (pdrv != 0U)
    {
        return STA_NOINIT;
    }

    cs_high();
    for (n = 0; n < 10U; n++)
    {
        spi_rw(0xFFU);
    }

    if (send_cmd(CMD0, 0) == 1U)
    {
        start = HAL_GetTick();
        if (send_cmd(CMD8, 0x1AAU) == 1U)
        {
            for (n = 0; n < 4U; n++)
            {
                ocr[n] = spi_rw(0xFFU);
            }
            if (ocr[2] == 0x01U && ocr[3] == 0xAAU)
            {
                while ((HAL_GetTick() - start) < 1000U)
                {
                    if (send_cmd(ACMD41, 1UL << 30) == 0U)
                    {
                        break;
                    }
                }
                if ((HAL_GetTick() - start) < 1000U && send_cmd(CMD58, 0) == 0U)
                {
                    for (n = 0; n < 4U; n++)
                    {
                        ocr[n] = spi_rw(0xFFU);
                    }
                    type = (ocr[0] & 0x40U) ? (CARD_TYPE_SDV2 | CARD_TYPE_BLOCK) : CARD_TYPE_SDV2;
                }
            }
        }
        else
        {
            if (send_cmd(ACMD41, 0) <= 1U)
            {
                type = CARD_TYPE_SDV1;
                start = HAL_GetTick();
                while ((HAL_GetTick() - start) < 1000U && send_cmd(ACMD41, 0) != 0U)
                {
                }
            }
            else
            {
                type = CARD_TYPE_MMC;
                start = HAL_GetTick();
                while ((HAL_GetTick() - start) < 1000U && send_cmd(CMD1, 0) != 0U)
                {
                }
            }

            if ((HAL_GetTick() - start) >= 1000U || send_cmd(CMD16, 512U) != 0U)
            {
                type = CARD_TYPE_UNKNOWN;
            }
        }
    }

    card_type = type;
    cs_high();

    if (type != CARD_TYPE_UNKNOWN)
    {
        sd_status = 0;
    }
    else
    {
        sd_status = STA_NOINIT;
    }

    return sd_status;
}

/* FatFs 扇区读取接口，按 512 字节扇区读取。 */
DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    UINT remaining = count;

    if (pdrv != 0U || count == 0U)
    {
        return RES_PARERR;
    }
    if (sd_status & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    if (!(card_type & CARD_TYPE_BLOCK))
    {
        sector *= 512U;
    }

    while (remaining > 0U)
    {
        if (send_cmd(CMD17, sector) == 0U && receive_data(buff, 512U))
        {
            buff += 512U;
            sector++;
            remaining--;
            continue;
        }
        break;
    }

    cs_high();
    return remaining ? RES_ERROR : RES_OK;
}

/* FatFs 扇区写入接口，按 512 字节扇区写入。 */
DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    UINT remaining = count;

    if (pdrv != 0U || count == 0U)
    {
        return RES_PARERR;
    }
    if (sd_status & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    if (!(card_type & CARD_TYPE_BLOCK))
    {
        sector *= 512U;
    }

    while (remaining > 0U)
    {
        if (send_cmd(CMD24, sector) == 0U && transmit_data(buff, 0xFEU))
        {
            buff += 512U;
            sector++;
            remaining--;
            continue;
        }
        break;
    }

    cs_high();
    return remaining ? RES_ERROR : RES_OK;
}

/* FatFs 控制接口，提供同步、扇区大小和卡类型查询。 */
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0U)
    {
        return RES_PARERR;
    }
    if (sd_status & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    switch (cmd)
    {
    case CTRL_SYNC:
        cs_low();
        if (wait_ready(500U))
        {
            cs_high();
            return RES_OK;
        }
        cs_high();
        return RES_ERROR;
    case GET_SECTOR_SIZE:
        *(WORD *)buff = 512U;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1U;
        return RES_OK;
    case MMC_GET_TYPE:
        *(BYTE *)buff = card_type;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}

/* 没有 RTC，给 FatFs 一个固定文件时间。 */
DWORD get_fattime(void)
{
    return ((DWORD)(2026U - 1980U) << 25) | ((DWORD)7U << 21) | ((DWORD)1U << 16);
}
