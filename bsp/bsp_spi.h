/*
 * ============ bsp_spi.h =============
 * SPI 通信 (SPI1: PB9 SCLK / PB8 MOSI / PB7 MISO, 10MHz, Mode0)
 *
 * SysConfig 已配置硬件, 本模块封装收发 API。
 * 共享于 ST7789 LCD (CS=PB14) 和 W25Q128 Flash (CS=PB6)。
 * 调用者负责 CS 控制和互斥。
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

/* 半双工收发单字节。无超时保护 — 仅用于 Flash 驱动等已确认外设存在的场景。
 * 返回接收到的字节。 */
uint8_t BSP_SPI_txrx_byte(uint8_t data);

/* 发送单字节并丢弃接收。有超时保护, 成功返回 true。
 * 用于 LCD 命令/数据发送。 */
bool BSP_SPI_tx_byte(uint8_t data);

/* 批量发送。逐个调用 BSP_SPI_tx_byte, 任一失败返回 false。 */
bool BSP_SPI_tx(const uint8_t *buf, size_t len);

/* ── DMA 收发 ── */

/* DMA 发送: buf → SPI TXDATA, 阻塞等待完成, 有超时保护。
 * 用于 LCD 像素数据和 Flash 写数据。buf 长度可达 65535。
 * 调用者负责 CS 控制和填充 TX/RX FIFO 前的准备工作。 */
bool BSP_SPI_tx_dma(const uint8_t *buf, uint16_t len);

/* DMA 接收: 从 SPI RXDATA → buf, 同时自动发送 0xFF 提供时钟。
 * 用于 Flash 读数据。内部启用 TX DMA 发送 dummy 字节。
 * 调用者负责发送命令/地址 (CPU) 和 CS 控制。 */
bool BSP_SPI_rx_dma(uint8_t *buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
