/*
 *  ============ bsp_spi.c =============
 *  BSP SPI — DL_SPI_* 直调封装 (参考 Keil 11_spi 的 spi_read_write_byte)
 *
 *  所有阻塞等待均包含超时保护, 防止总线异常卡死系统
 */

#include "bsp_spi.h"

#include "ti_msp_dl_config.h"      /* SPI_LCD_INST */
#include <ti/driverlib/dl_spi.h>   /* DL_SPI_* */

uint8_t BSP_SPI_txrx_byte(uint8_t data)
{
    DL_SPI_transmitData8(SPI_LCD_INST, data);
    while (DL_SPI_isBusy(SPI_LCD_INST));
    uint8_t r = DL_SPI_receiveData8(SPI_LCD_INST);
    while (DL_SPI_isBusy(SPI_LCD_INST));
    return r;
}

bool BSP_SPI_tx_byte(uint8_t data)
{
    uint32_t tout = BSP_SPI_TIMEOUT;

    DL_SPI_transmitData8(SPI_LCD_INST, data);
    while (DL_SPI_isBusy(SPI_LCD_INST)) {
        if (--tout == 0) return false;
    }
    (void)DL_SPI_receiveData8(SPI_LCD_INST);  /* 读 RX FIFO, 防止溢出 */
    tout = BSP_SPI_TIMEOUT;
    while (DL_SPI_isBusy(SPI_LCD_INST)) {
        if (--tout == 0) return false;
    }
    return true;
}

bool BSP_SPI_tx(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (!BSP_SPI_tx_byte(buf[i])) return false;
    }
    return true;
}
