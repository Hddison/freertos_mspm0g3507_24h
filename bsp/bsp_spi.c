/*
 * ============ bsp_spi.c =============
 */

#include "bsp_spi.h"
#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_spi.h>
#include <ti/driverlib/dl_dma.h>
#include <string.h>

/* ── CPU 收发 ── */

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
    (void)DL_SPI_receiveData8(SPI_LCD_INST);
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

/* ══════ DMA 收发 ══════ */

/* ── BSP_SPI_tx_dma ──
 * SysConfig 已初始化通道 (trigger + increment)。每次传输仅更新 src/dest/size。 */
bool BSP_SPI_tx_dma(const uint8_t *buf, uint16_t len)
{
    if (!buf || !len) return false;

    DL_DMA_setSrcAddr(DMA, DMA_SPI_LCD_TX_CHAN_ID, (uint32_t)buf);
    DL_DMA_setDestAddr(DMA, DMA_SPI_LCD_TX_CHAN_ID,
                       (uint32_t)&SPI_LCD_INST->TXDATA);
    DL_DMA_setTransferSize(DMA, DMA_SPI_LCD_TX_CHAN_ID, len);
    DL_DMA_enableChannel(DMA, DMA_SPI_LCD_TX_CHAN_ID);

    uint32_t tout = BSP_SPI_TIMEOUT;
    while (DL_DMA_getTransferSize(DMA, DMA_SPI_LCD_TX_CHAN_ID) != 0) {
        if (--tout == 0) {
            DL_DMA_disableChannel(DMA, DMA_SPI_LCD_TX_CHAN_ID);
            return false;
        }
    }

    DL_DMA_disableChannel(DMA, DMA_SPI_LCD_TX_CHAN_ID);
    return true;
}

/* ── BSP_SPI_rx_dma ──
 * RX 需要 TX dummy 字节提供时钟。分块发送 dummy。 */
bool BSP_SPI_rx_dma(uint8_t *buf, uint16_t len)
{
    if (!buf || !len) return false;

    static uint8_t dummy[256];
    static bool   dummy_init = false;
    if (!dummy_init) {
        memset(dummy, 0xFF, sizeof(dummy));
        dummy_init = true;
    }

    DL_DMA_setSrcAddr(DMA, DMA_SPI_LCD_RX_CHAN_ID,
                      (uint32_t)&SPI_LCD_INST->RXDATA);
    DL_DMA_setDestAddr(DMA, DMA_SPI_LCD_RX_CHAN_ID, (uint32_t)buf);
    DL_DMA_setTransferSize(DMA, DMA_SPI_LCD_RX_CHAN_ID, len);
    DL_DMA_enableChannel(DMA, DMA_SPI_LCD_RX_CHAN_ID);

    uint16_t rem = len;
    while (rem > 0) {
        uint16_t chunk = (rem > 256) ? 256 : rem;
        DL_DMA_setSrcAddr(DMA, DMA_SPI_LCD_TX_CHAN_ID, (uint32_t)dummy);
        DL_DMA_setDestAddr(DMA, DMA_SPI_LCD_TX_CHAN_ID,
                           (uint32_t)&SPI_LCD_INST->TXDATA);
        DL_DMA_setTransferSize(DMA, DMA_SPI_LCD_TX_CHAN_ID, chunk);
        DL_DMA_enableChannel(DMA, DMA_SPI_LCD_TX_CHAN_ID);

        uint32_t tout = BSP_SPI_TIMEOUT;
        while (DL_DMA_getTransferSize(DMA, DMA_SPI_LCD_TX_CHAN_ID) != 0) {
            if (--tout == 0) {
                DL_DMA_disableChannel(DMA, DMA_SPI_LCD_TX_CHAN_ID);
                goto rx_fail;
            }
        }
        DL_DMA_disableChannel(DMA, DMA_SPI_LCD_TX_CHAN_ID);
        rem -= chunk;
    }

    {
        uint32_t tout = BSP_SPI_TIMEOUT;
        while (DL_DMA_getTransferSize(DMA, DMA_SPI_LCD_RX_CHAN_ID) != 0) {
            if (--tout == 0) goto rx_fail;
        }
    }

    DL_DMA_disableChannel(DMA, DMA_SPI_LCD_RX_CHAN_ID);
    return true;

rx_fail:
    DL_DMA_disableChannel(DMA, DMA_SPI_LCD_RX_CHAN_ID);
    return false;
}
