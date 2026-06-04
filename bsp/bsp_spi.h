/*
 * ============ bsp_spi.h =============
 * SPI 通信 (SPI1: PB9 SCLK / PB8 MOSI / PB7 MISO, 10MHz, Mode0)
 *
 * SysConfig 已配置硬件, 本模块封装收发 API。
 * 共享于 ST7789 LCD (CS=PB14) 和 W25Q128 Flash (CS=PB6)。
 * 互斥由上层 HAL 负责 (hal_spi.h)。
 */

#ifndef BSP_SPI_H
#define BSP_SPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 超时值: ~500ms @ 80MHz */
#define BSP_SPI_TIMEOUT 10000000UL

/* ── CPU 收发 ── */

/* 半双工收发单字节。无超时保护 — 仅用于 Flash 驱动等已确认外设存在的场景。 */
uint8_t BSP_SPI_txrx_byte(uint8_t data);

/* 发送单字节并丢弃接收。有超时保护, 成功返回 true。
 * 用于 LCD 命令/数据发送。 */
bool BSP_SPI_tx_byte(uint8_t data);

/* 批量发送。逐个调用 BSP_SPI_tx_byte, 任一失败返回 false。 */
bool BSP_SPI_tx(const uint8_t *buf, size_t len);

/* ── DMA 收发 ── */

/* DMA 发送: buf → SPI TXDATA, 阻塞等待完成, 有超时保护。 */
bool BSP_SPI_tx_dma(const uint8_t *buf, uint16_t len);

/* DMA 接收: 从 SPI RXDATA → buf, 同时自动发送 0xFF 提供时钟。 */
bool BSP_SPI_rx_dma(uint8_t *buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
