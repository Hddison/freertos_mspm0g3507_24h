/*
 * ============ bsp_uart.c =============
 */

#include "bsp_uart.h"
#include "ti_msp_dl_config.h"      /* UART_0_INST, DMA channel IDs */
#include <ti/driverlib/dl_uart.h>  /* DL_UART_* */
#include <ti/driverlib/dl_dma.h>   /* DL_DMA_* */

/* 每 ms 轮询大约 8000 次 (80MHz / 10000), 足够捕捉 115200 bps 的数据 */
#define UART_POLL_PER_MS 8000

/* ── BSP_UART_tx_byte ── */
void BSP_UART_tx_byte(uint8_t data)
{
    DL_UART_transmitDataBlocking(UART_0_INST, data);
}

/* ── BSP_UART_tx_str ── */
void BSP_UART_tx_str(const char *str)
{
    while (*str) {
        DL_UART_transmitDataBlocking(UART_0_INST, (uint8_t)*str++);
    }
}

/* ── BSP_UART_rx_byte ── */
uint8_t BSP_UART_rx_byte(void)
{
    return DL_UART_receiveDataBlocking(UART_0_INST);
}

/* ── BSP_UART_rx_byte_timeout ── */
bool BSP_UART_rx_byte_timeout(uint8_t *data, uint32_t timeout_ms)
{
    if (!data) return false;

    uint32_t total = timeout_ms * UART_POLL_PER_MS;
    for (uint32_t i = 0; i < total; i++) {
        if (!DL_UART_isRXFIFOEmpty(UART_0_INST)) {
            *data = DL_UART_receiveDataBlocking(UART_0_INST);
            return true;
        }
    }
    return false;
}

/* ── BSP_UART_rx_ready ── */
bool BSP_UART_rx_ready(void)
{
    return !DL_UART_isRXFIFOEmpty(UART_0_INST);
}

/* ══════ DMA 收发 ══════ */

/* ── BSP_UART_tx_dma ──
 * DMA_UART_TX: src=增量(buf), dst=固定(TXDATA)
 * SysConfig 已正确配置, 无需重新 initChannel。 */
bool BSP_UART_tx_dma(const uint8_t *buf, uint16_t len)
{
    if (!buf || !len) return false;

    DL_DMA_setSrcAddr(DMA, DMA_UART_TX_CHAN_ID, (uint32_t)buf);
    DL_DMA_setDestAddr(DMA, DMA_UART_TX_CHAN_ID, (uint32_t)&UART_0_INST->TXDATA);
    DL_DMA_setTransferSize(DMA, DMA_UART_TX_CHAN_ID, len);
    DL_DMA_enableChannel(DMA, DMA_UART_TX_CHAN_ID);

    uint32_t tout = 10000000;
    while (DL_DMA_getTransferSize(DMA, DMA_UART_TX_CHAN_ID) != 0) {
        if (--tout == 0) {
            DL_DMA_disableChannel(DMA, DMA_UART_TX_CHAN_ID);
            return false;
        }
    }
    DL_DMA_disableChannel(DMA, DMA_UART_TX_CHAN_ID);
    return true;
}

/* ── BSP_UART_rx_dma ──
 * DMA_UART_RX: src=固定(RXDATA), dst=增量(buf)
 * SysConfig 已正确配置, 无需重新 initChannel。 */
uint16_t BSP_UART_rx_dma(uint8_t *buf, uint16_t len, uint32_t timeout_ms)
{
    if (!buf || !len) return 0;

    DL_DMA_setSrcAddr(DMA, DMA_UART_RX_CHAN_ID, (uint32_t)&UART_0_INST->RXDATA);
    DL_DMA_setDestAddr(DMA, DMA_UART_RX_CHAN_ID, (uint32_t)buf);
    DL_DMA_setTransferSize(DMA, DMA_UART_RX_CHAN_ID, len);
    DL_DMA_enableChannel(DMA, DMA_UART_RX_CHAN_ID);

    /* 轮询等待 (同时检查超时) */
    uint32_t tout = timeout_ms * 8000;
    uint16_t rxfer;
    do {
        rxfer = DL_DMA_getTransferSize(DMA, DMA_UART_RX_CHAN_ID);
    } while (rxfer != 0 && --tout);

    DL_DMA_disableChannel(DMA, DMA_UART_RX_CHAN_ID);

    /* 已接收字节数 = 请求数 - 剩余数 */
    return len - rxfer;
}
