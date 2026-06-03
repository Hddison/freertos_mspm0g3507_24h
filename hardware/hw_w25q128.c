/*
 *  ============ hw_w25q128.c =============
 *  W25Q128JVSIQ SPI NOR Flash 驱动实现 (DMA 版本)
 *
 *  命令阶段: CPU 轮询 (BSP_SPI_txrx_byte) — 简单可靠
 *  数据阶段: DMA TX/RX — 与 LCD 共用 CH0 TX, 单独 CH3 RX
 *
 *  关键教训:
 *   - 不调用 DL_SPI_setBitRateSerialClockDivider (会破坏运行时 SPI 状态)
 *   - 不发送 0xAB (醒着的 Flash 会将其解释为 Read Device ID)
 *   - Flash 操作期间暂停 LCD 任务 (vTaskSuspend)
 */

#include "hw_w25q128.h"

#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>

#include "bsp_spi.h"

/* ── 清除 SPI RX FIFO 中的残留数据 (LCD DMA 留下的) ── */
static void _flushRxFifo(void)
{
    while (!DL_SPI_isRXFIFOEmpty(SPI_LCD_INST)) {
        (void)DL_SPI_receiveData8(SPI_LCD_INST);
    }
}

/* ── CS 控制 (PB6, 低有效) ── */
#define W25Q_CS_LOW()   DL_GPIO_clearPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN)
#define W25Q_CS_HIGH()  DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN)

/* ── 超时 ── */
#define W25Q_BUSY_TIMEOUT  10000000UL   /* ~500ms @ 80MHz */

/* ── 内部: CPU 发送单字节 (全双工, 无额外操作) ── */
static uint8_t _spi(uint8_t tx)
{
    return BSP_SPI_txrx_byte(tx);
}

/* ── 内部: 写使能 ── */
static void _wren(void)
{
    W25Q_CS_LOW();  _spi(W25Q_CMD_WREN);  W25Q_CS_HIGH();
}

/* ── 内部: 等待 BUSY 清除 ── */
static bool _waitBusy(void)
{
    uint32_t tout = W25Q_BUSY_TIMEOUT;
    uint8_t  sr;
    do {
        W25Q_CS_LOW();  _spi(W25Q_CMD_READ_SR1);  sr = _spi(0xFF);  W25Q_CS_HIGH();
        if (--tout == 0) return false;
    } while (sr & 0x01);
    return true;
}

/* ══════ API ══════ */

uint8_t HW_W25Q128_readSR1(void)
{
    uint8_t sr;
    _flushRxFifo();
    W25Q_CS_LOW();  _spi(W25Q_CMD_READ_SR1);  sr = _spi(0xFF);  W25Q_CS_HIGH();
    return sr;
}

uint16_t HW_W25Q128_readID(void)
{
    uint16_t id;
    _flushRxFifo();
    W25Q_CS_LOW();
    _spi(W25Q_CMD_RDID);        /* 0x90 */
    _spi(0x00);                 /* addr[23:16] */
    _spi(0x00);                 /* addr[15:8]  */
    _spi(0x00);                 /* addr[7:0]   */
    id  = (uint16_t)_spi(0xFF) << 8;
    id |= (uint16_t)_spi(0xFF);
    W25Q_CS_HIGH();
    return id;
}

/* CPU 读取: 完全对齐 Keil demo, 纯轮询 */
bool HW_W25Q128_read(uint8_t *buf, uint32_t addr, uint16_t len)
{
    if (!buf || !len) return false;

    _flushRxFifo();

    W25Q_CS_LOW();
    _spi(W25Q_CMD_READ_DATA);
    _spi((uint8_t)(addr >> 16));
    _spi((uint8_t)(addr >> 8));
    _spi((uint8_t)addr);
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = _spi(0xFF);
    }
    W25Q_CS_HIGH();
    return true;
}

/* CPU 写入: 完全对齐 Keil demo, 纯轮询 */
bool HW_W25Q128_write(uint8_t *buf, uint32_t addr, uint16_t len)
{
    if (!buf || !len) return false;

    uint8_t sec = (uint8_t)(addr / W25Q_SECTOR_SIZE);
    if (!HW_W25Q128_eraseSector(sec)) return false;

    _wren();
    if (!_waitBusy()) return false;

    W25Q_CS_LOW();
    _spi(W25Q_CMD_PAGE_PROG);
    _spi((uint8_t)(addr >> 16));
    _spi((uint8_t)(addr >> 8));
    _spi((uint8_t)addr);
    for (uint16_t i = 0; i < len; i++) {
        _spi(buf[i]);
    }
    W25Q_CS_HIGH();

    return _waitBusy();
}

/* 扇区擦除 (4KB) */
bool HW_W25Q128_eraseSector(uint8_t sector)
{
    uint32_t addr = (uint32_t)sector * W25Q_SECTOR_SIZE;

    _wren();
    if (!_waitBusy()) return false;

    W25Q_CS_LOW();
    _spi(W25Q_CMD_SECTOR_ERASE);
    _spi((uint8_t)(addr >> 16));
    _spi((uint8_t)(addr >> 8));
    _spi((uint8_t)addr);
    W25Q_CS_HIGH();

    return _waitBusy();   /* 典型 45ms, 最大 400ms */
}

/* 全片擦除 */
bool HW_W25Q128_eraseChip(void)
{
    _wren();
    if (!_waitBusy()) return false;

    W25Q_CS_LOW();
    _spi(W25Q_CMD_CHIP_ERASE);
    W25Q_CS_HIGH();

    return _waitBusy();   /* 典型 40s, 最大 80s */
}
