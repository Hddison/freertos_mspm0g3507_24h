/*
 * ============ bsp_spi.c =============
 */

#include "bsp_spi.h"
#include "ti_msp_dl_config.h"      /* SPI_LCD_INST, DMA channel IDs */
#include <ti/driverlib/dl_spi.h>   /* DL_SPI_* */
#include <ti/driverlib/dl_dma.h>   /* DL_DMA_* */
#include <string.h>               /* memset */

/* ── BSP_SPI_txrx_byte ──
 * 无超时 — 用于 Flash 驱动 (已通过读 ID 确认外设存在) */
uint8_t BSP_SPI_txrx_byte(uint8_t data)
{
    DL_SPI_transmitData8(SPI_LCD_INST, data);
    while (DL_SPI_isBusy(SPI_LCD_INST));
    uint8_t r = DL_SPI_receiveData8(SPI_LCD_INST);
    while (DL_SPI_isBusy(SPI_LCD_INST));
    return r;
}

/* ── BSP_SPI_tx_byte ──
 * 有超时 — 用于 LCD 等可能出问题的外设 */
bool BSP_SPI_tx_byte(uint8_t data)
{
    uint32_t tout = BSP_SPI_TIMEOUT;

    DL_SPI_transmitData8(SPI_LCD_INST, data);
    while (DL_SPI_isBusy(SPI_LCD_INST)) {
        if (--tout == 0) return false;
    }
    (void)DL_SPI_receiveData8(SPI_LCD_INST);  /* 读走 RX, 防止 FIFO 溢出 */
    tout = BSP_SPI_TIMEOUT;
    while (DL_SPI_isBusy(SPI_LCD_INST)) {
        if (--tout == 0) return false;
    }
    return true;
}

/* ── BSP_SPI_tx ── */
bool BSP_SPI_tx(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (!BSP_SPI_tx_byte(buf[i])) return false;
    }
    return true;
}

/* ══════ DMA 收发 ══════ */

/* ── BSP_SPI_tx_dma ── */
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
 * RX DMA 需要 TX 提供时钟: 内部发 0xFF 字节。
 * 用 256 字节 dummy buffer, TX DMA 分块循环发送。 */
bool BSP_SPI_rx_dma(uint8_t *buf, uint16_t len)
{
    if (!buf || !len) return false;

    /* 初始化 dummy TX buffer (一次) */
    static uint8_t dummy[256];
    static bool   dummy_init = false;
    if (!dummy_init) {
        memset(dummy, 0xFF, sizeof(dummy));
        dummy_init = true;
    }

    /* 配置 RX DMA */
    DL_DMA_setSrcAddr(DMA, DMA_SPI_LCD_RX_CHAN_ID,
                      (uint32_t)&SPI_LCD_INST->RXDATA);
    DL_DMA_setDestAddr(DMA, DMA_SPI_LCD_RX_CHAN_ID, (uint32_t)buf);
    DL_DMA_setTransferSize(DMA, DMA_SPI_LCD_RX_CHAN_ID, len);
    DL_DMA_enableChannel(DMA, DMA_SPI_LCD_RX_CHAN_ID);

    /* 配置 TX DMA: 分块发送 dummy 字节提供时钟 */
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

    /* 等待 RX DMA 完成 */
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
