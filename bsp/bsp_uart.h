/*
 * ============ bsp_uart.h =============
 * 调试串口 (UART0: PA10 TX / PA11 RX, 115200-8N1)
 *
 * SysConfig 已配置硬件, 本模块仅封装收发 API。
 * 发送均为阻塞; 接收提供阻塞版和超时版。
 */

#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 发送单字节 (阻塞) */
void BSP_UART_tx_byte(uint8_t data);

/* 发送 C 字符串 (阻塞, 逐字节) */
void BSP_UART_tx_str(const char *str);

/* 接收单字节 (阻塞, 无超时 — 调用者确保数据可达) */
uint8_t BSP_UART_rx_byte(void);

/* 接收单字节 (超时版)。timeout_ms 毫秒内无数据返回 false。
 * 轮询间隔约 1/8000 ms (80MHz/10000) */
bool BSP_UART_rx_byte_timeout(uint8_t *data, uint32_t timeout_ms);

/* RX FIFO 是否有数据可读 */
bool BSP_UART_rx_ready(void);

/* ── DMA 收发 ── */

/* DMA 发送: buf → UART TXDATA, 阻塞等待完成, 超时保护。
 * 通道: DMA_CH2_CHAN_ID (UART TX 触发) */
bool BSP_UART_tx_dma(const uint8_t *buf, uint16_t len);

/* DMA 接收: UART RXDATA → buf, 阻塞等待 len 字节或超时。
 * 通道: DMA_CH1_CHAN_ID (UART RX 触发)
 * 返回实际接收字节数，超时返回已接收数 */
uint16_t BSP_UART_rx_dma(uint8_t *buf, uint16_t len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
