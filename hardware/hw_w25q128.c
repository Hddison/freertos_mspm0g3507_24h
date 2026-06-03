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
#include <ti/driverlib/dl_dma.h>

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

/* ── 内部: DMA TX (CH1, 与 LCD 共用) ── */
static void _dmaTx(const uint8_t *buf, uint16_t len)
{
    DL_DMA_disableChannel(DMA, W25Q_DMA_TX_CH);   /* 确保 LCD 没占用 */
    DL_DMA_setSrcAddr(DMA, W25Q_DMA_TX_CH, (uint32_t)buf);
    DL_DMA_setDestAddr(DMA, W25Q_DMA_TX_CH, (uint32_t)&SPI_LCD_INST->TXDATA);
    DL_DMA_setTransferSize(DMA, W25Q_DMA_TX_CH, len);
    DL_DMA_enableChannel(DMA, W25Q_DMA_TX_CH);
    while (DL_DMA_getTransferSize(DMA, W25Q_DMA_TX_CH) != 0);
    DL_DMA_disableChannel(DMA, W25Q_DMA_TX_CH);
}

/* ── 内部: DMA RX (CH0, 从 SPI1 接收) ── */
static void _dmaRx(uint8_t *buf, uint16_t len)
{
    DL_DMA_disableChannel(DMA, W25Q_DMA_RX_CH);   /* 确保干净状态 */
    DL_DMA_setSrcAddr(DMA, W25Q_DMA_RX_CH, (uint32_t)&SPI_LCD_INST->RXDATA);
    DL_DMA_setDestAddr(DMA, W25Q_DMA_RX_CH, (uint32_t)buf);
    DL_DMA_setTransferSize(DMA, W25Q_DMA_RX_CH, len);
    DL_DMA_enableChannel(DMA, W25Q_DMA_RX_CH);
    while (DL_DMA_getTransferSize(DMA, W25Q_DMA_RX_CH) != 0);
    DL_DMA_disableChannel(DMA, W25Q_DMA_RX_CH);
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

/* DMA 读取: 命令(CPU) + 数据(TX DMA 发 dummy + RX DMA 收) */
bool HW_W25Q128_read(uint8_t *buf, uint32_t addr, uint16_t len)
{
    if (!buf || !len) return false;

    _flushRxFifo();

    /* 1. CPU 发送读命令 */
    W25Q_CS_LOW();
    _spi(W25Q_CMD_READ_DATA);
    _spi((uint8_t)(addr >> 16));
    _spi((uint8_t)(addr >> 8));
    _spi((uint8_t)addr);

    /* 2. 填充 dummy 缓冲区 (发 0xFF 以产生时钟) */
    /*    ST7789 的 dma_row[] 可借用, 但这里用栈上小缓冲 */
    {
        uint8_t dummy[64];
        for (int i = 0; i < 64; i++) dummy[i] = 0xFF;
        uint16_t remain = len;
        uint8_t *dst = buf;
        while (remain > 0) {
            uint16_t chunk = (remain > 64) ? 64 : remain;
            /* 同时启动 TX DMA (dummy) 和 RX DMA (data) */
            DL_DMA_setSrcAddr(DMA, W25Q_DMA_TX_CH, (uint32_t)dummy);
            DL_DMA_setDestAddr(DMA, W25Q_DMA_TX_CH, (uint32_t)&SPI_LCD_INST->TXDATA);
            DL_DMA_setTransferSize(DMA, W25Q_DMA_TX_CH, chunk);
            DL_DMA_setSrcAddr(DMA, W25Q_DMA_RX_CH, (uint32_t)&SPI_LCD_INST->RXDATA);
            DL_DMA_setDestAddr(DMA, W25Q_DMA_RX_CH, (uint32_t)dst);
            DL_DMA_setTransferSize(DMA, W25Q_DMA_RX_CH, chunk);
            DL_DMA_enableChannel(DMA, W25Q_DMA_RX_CH);
            DL_DMA_enableChannel(DMA, W25Q_DMA_TX_CH);
            while (DL_DMA_getTransferSize(DMA, W25Q_DMA_TX_CH) != 0);
            DL_DMA_disableChannel(DMA, W25Q_DMA_TX_CH);
            DL_DMA_disableChannel(DMA, W25Q_DMA_RX_CH);
            dst += chunk;
            remain -= chunk;
        }
    }

    W25Q_CS_HIGH();
    return true;
}

/* DMA 写入: 命令(CPU) + 数据(DMA TX), 自动擦除扇区 */
bool HW_W25Q128_write(uint8_t *buf, uint32_t addr, uint16_t len)
{
    if (!buf || !len) return false;

    /* 1. 擦除所在扇区 */
    uint8_t sec = (uint8_t)(addr / W25Q_SECTOR_SIZE);
    if (!HW_W25Q128_eraseSector(sec)) return false;

    /* 2. CPU 发送写命令 */
    _wren();
    if (!_waitBusy()) return false;

    W25Q_CS_LOW();
    _spi(W25Q_CMD_PAGE_PROG);
    _spi((uint8_t)(addr >> 16));
    _spi((uint8_t)(addr >> 8));
    _spi((uint8_t)addr);

    /* 3. DMA 发送数据 */
    _dmaTx(buf, len);

    W25Q_CS_HIGH();

    /* 4. 等待编程完成 */
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
