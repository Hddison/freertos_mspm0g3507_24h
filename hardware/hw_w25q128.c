/*
 * ============ hw_w25q128.c =============
 * W25Q128 SPI Flash 驱动
 *
 * 命令/地址: CPU 轮询 (BSP_SPI_txrx_byte)
 * 数据读:    DMA RX  (BSP_SPI_rx_dma)
 * 数据写:    CPU 轮询 (逐字节 _spi, 免 DMA TX 偏移问题)
 *
 * CS=PB6 由本模块控制。SPI1 与 LCD 共享, 通过 HAL_SPI_lock/unlock 互斥。
 */

#include "hw_w25q128.h"
#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>
#include "bsp_spi.h"
#include "hal_spi.h"

/* ── CS (PB6, 低有效) ── */
#define CS_LOW()   DL_GPIO_clearPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN)
#define CS_HIGH()  DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN)

#define BUSY_TIMEOUT  10000000UL  /* ~500ms @ 80MHz */

/* ── CPU 单字节收发 ── */
static uint8_t _spi(uint8_t tx) { return BSP_SPI_txrx_byte(tx); }

/* ── 写使能 ── */
static void _wren(void) { CS_LOW(); _spi(W25Q_CMD_WREN); CS_HIGH(); }

/* ── 等待忙清除 ── */
static bool _waitBusy(void) {
    uint32_t t = BUSY_TIMEOUT; uint8_t sr;
    do { CS_LOW(); _spi(W25Q_CMD_READ_SR1); sr=_spi(0xFF); CS_HIGH();
         if (--t==0) return false; } while (sr & 0x01);
    return true;
}

/* ══════ 公开 API (每函数自动加锁/解锁) ══════ */

uint8_t HW_W25Q128_readSR1(void)
{
    HAL_SPI_lock();
    uint8_t sr;
    CS_LOW(); _spi(W25Q_CMD_READ_SR1); sr=_spi(0xFF); CS_HIGH();
    HAL_SPI_unlock();
    return sr;
}

uint16_t HW_W25Q128_readID(void)
{
    HAL_SPI_lock();
    uint16_t id;
    CS_LOW();
    _spi(W25Q_CMD_RDID); _spi(0x00); _spi(0x00); _spi(0x00);
    id  = (uint16_t)_spi(0xFF) << 8;
    id |= (uint16_t)_spi(0xFF);
    CS_HIGH();
    HAL_SPI_unlock();
    return id;
}

bool HW_W25Q128_read(uint8_t *buf, uint32_t addr, uint16_t len)
{
    if (!buf || !len) return false;

    HAL_SPI_lock();
    CS_LOW();
    _spi(W25Q_CMD_READ_DATA);
    _spi((uint8_t)(addr >> 16));
    _spi((uint8_t)(addr >> 8));
    _spi((uint8_t)addr);
    bool ok = BSP_SPI_rx_dma(buf, len);
    CS_HIGH();
    HAL_SPI_unlock();
    return ok;
}

/* ── 内部无锁版本 (调用者已持有锁) ── */

static bool _eraseSector(uint16_t sector)
{
    uint32_t addr = (uint32_t)sector * W25Q_SECTOR_SIZE;
    _wren(); if (!_waitBusy()) return false;
    CS_LOW();
    _spi(W25Q_CMD_SECTOR_ERASE);
    _spi((uint8_t)(addr >> 16));
    _spi((uint8_t)(addr >> 8));
    _spi((uint8_t)addr);
    CS_HIGH();
    return _waitBusy();
}

/* ══════ 公开 API (每函数自动加锁/解锁) ══════ */

bool HW_W25Q128_write(const uint8_t *buf, uint32_t addr, uint16_t len)
{
    if (!buf || !len) return false;

    HAL_SPI_lock();

    uint16_t sector = (uint16_t)(addr / W25Q_SECTOR_SIZE);
    if (!_eraseSector(sector)) {
        HAL_SPI_unlock();
        return false;
    }

    _wren(); if (!_waitBusy()) { HAL_SPI_unlock(); return false; }

    CS_LOW();
    _spi(W25Q_CMD_PAGE_PROG);
    _spi((uint8_t)(addr >> 16));
    _spi((uint8_t)(addr >> 8));
    _spi((uint8_t)addr);
    for (uint16_t i = 0; i < len; i++) _spi(buf[i]);
    CS_HIGH();

    bool ok = _waitBusy();
    HAL_SPI_unlock();
    return ok;
}

bool HW_W25Q128_eraseSector(uint16_t sector)
{
    HAL_SPI_lock();
    bool ok = _eraseSector(sector);
    HAL_SPI_unlock();
    return ok;
}

bool HW_W25Q128_eraseChip(void)
{
    HAL_SPI_lock();
    _wren(); if (!_waitBusy()) { HAL_SPI_unlock(); return false; }
    CS_LOW(); _spi(W25Q_CMD_CHIP_ERASE); CS_HIGH();
    bool ok = _waitBusy();
    HAL_SPI_unlock();
    return ok;
}
