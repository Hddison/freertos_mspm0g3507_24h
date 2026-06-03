/*
 *  ============ bsp_spi.c =============
 *  BSP SPI — DL_SPI_* 直调封装 (参考 Keil 11_spi 的 spi_read_write_byte)
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

void BSP_SPI_tx_byte(uint8_t data)
{
    DL_SPI_transmitData8(SPI_LCD_INST, data);
    while (DL_SPI_isBusy(SPI_LCD_INST));
    DL_SPI_receiveData8(SPI_LCD_INST);  /* 读 RX FIFO, 防止溢出 */
    while (DL_SPI_isBusy(SPI_LCD_INST));
}

void BSP_SPI_tx(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        DL_SPI_transmitData8(SPI_LCD_INST, buf[i]);
        while (DL_SPI_isBusy(SPI_LCD_INST));
        DL_SPI_receiveData8(SPI_LCD_INST);  /* 读 RX FIFO */
        while (DL_SPI_isBusy(SPI_LCD_INST));
    }
}
