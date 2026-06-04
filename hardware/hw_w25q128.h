/*
 *  ============ hw_w25q128.h =============
 *  W25Q128JVSIQ SPI NOR Flash 驱动 (DMA 版本)
 *
 *  共享 SPI1 (PB9/SCLK, PB8/MOSI, PB7/MISO, 40MHz Mode0)
 *  CS: PB6 (GPIO_W25Q_W_CS)
 *
 *  架构: 命令→CPU轮询, 数据→DMA (TX=CH0, RX=CH3)
 */

#ifndef HW_W25Q128_H
#define HW_W25Q128_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── 命令码 ── */
#define W25Q_CMD_WREN           0x06
#define W25Q_CMD_READ_SR1       0x05
#define W25Q_CMD_READ_DATA      0x03
#define W25Q_CMD_PAGE_PROG      0x02
#define W25Q_CMD_SECTOR_ERASE   0x20
#define W25Q_CMD_CHIP_ERASE     0xC7
#define W25Q_CMD_RDID           0x90

/* ── 设备 ID ── */
#define W25Q128_MFR_ID          0xEF
#define W25Q128_DEV_ID          0x17

/* ── 几何参数 ── */
#define W25Q_SECTOR_SIZE        4096
#define W25Q_PAGE_SIZE          256

/* ── API ──
 * DMA 收发统一走 BSP SPI (bsp_spi.h)。
 * CS 控制由本模块负责 (PB6)。 */
uint16_t HW_W25Q128_readID(void);
bool     HW_W25Q128_read(uint8_t *buf, uint32_t addr, uint16_t len);
bool     HW_W25Q128_write(const uint8_t *buf, uint32_t addr, uint16_t len);
bool     HW_W25Q128_eraseSector(uint16_t sector);
bool     HW_W25Q128_eraseChip(void);
uint8_t  HW_W25Q128_readSR1(void);

#ifdef __cplusplus
}
#endif

#endif
