/*
 *  ============ hw_st7789.c =============
 *  ST7789 1.9" LCD 驱动 — 参考 STM32 HAL 版本移植
 *
 *  SPI1: PB9(SCLK)/PB8(MOSI), 10MHz Mode0
 *  GPIO: PB14(CS)/PB11(DC)/PB10(RST)/PB26(BLK)
 */

#include "hw_st7789.h"

#include "ti_msp_dl_config.h"
#include "bsp_spi.h"
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_dma.h>
#include <string.h>

#include "bsp_system.h"

/* ── DMA 行缓冲: 单行像素 (最大满足 Font24 × 10 字符 = 170px) ── */
#define DMA_ROW_MAX 256
static uint16_t dma_row[DMA_ROW_MAX];

/* ── DMA 行发送 ── */
static inline void spi_dma_row(const uint16_t *buf, uint16_t pixels)
{
    uint16_t bytes = pixels * 2;
    DL_DMA_setSrcAddr(DMA, DMA_SPI_LCD_TX_CHAN_ID, (uint32_t)buf);
    DL_DMA_setDestAddr(DMA, DMA_SPI_LCD_TX_CHAN_ID,
                       (uint32_t)&SPI_LCD_INST->TXDATA);
    DL_DMA_setTransferSize(DMA, DMA_SPI_LCD_TX_CHAN_ID, bytes);
    DL_DMA_enableChannel(DMA, DMA_SPI_LCD_TX_CHAN_ID);
    while (DL_DMA_getTransferSize(DMA, DMA_SPI_LCD_TX_CHAN_ID) != 0);
    DL_DMA_disableChannel(DMA, DMA_SPI_LCD_TX_CHAN_ID);
}

/* ── GPIO 控制宏 ── */
#define CS_0  DL_GPIO_clearPins(GPIOB, GPIO_LCD_LCD_CS_PIN)
#define CS_1  DL_GPIO_setPins(GPIOB, GPIO_LCD_LCD_CS_PIN)
#define DC_0  DL_GPIO_clearPins(GPIOB, GPIO_LCD_LCD_DC_PIN)
#define DC_1  DL_GPIO_setPins(GPIOB, GPIO_LCD_LCD_DC_PIN)
#define RST_0 DL_GPIO_clearPins(GPIOB, GPIO_LCD_LCD_RES_PIN)
#define RST_1 DL_GPIO_setPins(GPIOB, GPIO_LCD_LCD_RES_PIN)
#define BLK_0 DL_GPIO_clearPins(GPIOB, GPIO_LCD_LCD_BLK_PIN)
#define BLK_1 DL_GPIO_setPins(GPIOB, GPIO_LCD_LCD_BLK_PIN)

static ST7789_DIR lcd_dir;

/* ── 底层 SPI 发送 ── */

static void send_cmd(uint8_t cmd)
{
    DC_0; CS_0;
    BSP_SPI_tx_byte(cmd);
    CS_1;
}

static void send_data8(uint8_t data)
{
    DC_1; CS_0;
    BSP_SPI_tx_byte(data);
    CS_1;
}

static void send_data16(uint16_t data)
{
    DC_1; CS_0;
    BSP_SPI_tx_byte((uint8_t)(data >> 8));
    BSP_SPI_tx_byte((uint8_t)data);
    CS_1;
}

/* ── 硬件复位 ── */

static void reset(void)
{
    RST_1; BSP_delay_ms(200);
    RST_0; BSP_delay_ms(200);
    RST_1; BSP_delay_ms(200);
}

/* ── 寄存器初始化序列 ── */

static void init_reg(void)
{
    send_cmd(0x3A); send_data8(0x05);  /* 16-bit/pixel */
    send_cmd(0xC5); send_data8(0x1A); 
    send_cmd(0x36); send_data8(0x00); 

    send_cmd(0xB2);
    send_data8(0x05); send_data8(0x05); send_data8(0x00);
    send_data8(0x33); send_data8(0x33);

    send_cmd(0xB7); send_data8(0x05);
    send_cmd(0xBB); send_data8(0x3F);
    send_cmd(0xC0); send_data8(0x2C);
    send_cmd(0xC2); send_data8(0x01);
    send_cmd(0xC3); send_data8(0x0F);
    send_cmd(0xC4); send_data8(0x20);
    send_cmd(0xC6); send_data8(0x01);
    send_cmd(0xD0); send_data8(0xA4); send_data8(0xA1);
    send_cmd(0xE8); send_data8(0x03);
    send_cmd(0xE9); send_data8(0x09);send_data8(0x09);send_data8(0x08);

    /* Gamma + */
    send_cmd(0xE0);
    send_data8(0xD0); send_data8(0x05); send_data8(0x09); send_data8(0x09);
    send_data8(0x08); send_data8(0x14); send_data8(0x28); send_data8(0x33);
    send_data8(0x3F); send_data8(0x07); send_data8(0x13); send_data8(0x14);
    send_data8(0x28); send_data8(0x30);

    /* Gamma - */
    send_cmd(0xE1);
    send_data8(0xD0); send_data8(0x05); send_data8(0x09); send_data8(0x09);
    send_data8(0x08); send_data8(0x03); send_data8(0x24); send_data8(0x32);
    send_data8(0x32); send_data8(0x3B); send_data8(0x14); send_data8(0x13);
    send_data8(0x28); send_data8(0x2F);

    send_cmd(0x21);                    /* display inversion on */
    send_cmd(0x11); BSP_delay_ms(120); /* sleep out */
    send_cmd(0x29);                    /* display on */
}

/* ── API ── */

void ST7789_init(ST7789_DIR dir)
{
    lcd_dir = dir;
    BLK_0;
    reset();
    init_reg();

    /* memory access control */
    send_cmd(0x36);
    switch (dir) {
    case ST7789_HORIZONTAL: send_data8(0x00); break;
    case ST7789_VERTICAL:   send_data8(0x70); break;
    case ST7789_CCW90:      send_data8(0x20); break;  /* MV=1: row/col swap */
    default:                send_data8(0x00); break;
    }
}

void ST7789_setWindows(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    if (lcd_dir != ST7789_HORIZONTAL) {
        /* 旋转 90°: MV=1, CASET→逻辑Y(+35), RASET→逻辑X */
        send_cmd(0x2A);
        send_data16(y0 + ST7789_COLUMN_OFFSET);
        send_data16(y1 + ST7789_COLUMN_OFFSET);
        send_cmd(0x2B);
        send_data16(x0);
        send_data16(x1);
    } else {
        send_cmd(0x2A);
        send_data16(x0 + ST7789_COLUMN_OFFSET);
        send_data16(x1 + ST7789_COLUMN_OFFSET);
        send_cmd(0x2B);
        send_data16(y0);
        send_data16(y1);
    }
    send_cmd(0x2C);  /* memory write */
}

void ST7789_clear(uint16_t color)
{
    uint32_t total = (uint32_t)ST7789_WIDTH * ST7789_HEIGHT;
    ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    DC_1; CS_0;
    for (uint32_t i = 0; i < total; i++) {
        BSP_SPI_tx_byte((uint8_t)(color >> 8));
        BSP_SPI_tx_byte((uint8_t)color);
    }
    CS_1;
}

/* 高效区域填充: 调用者需先 setWindows, 本函数只写数据 */
void ST7789_clearRaw(uint16_t color, uint16_t w, uint16_t h)
{
    uint32_t total = (uint32_t)w * h;
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)color;
    DC_1; CS_0;
    for (uint32_t i = 0; i < total; i++) {
        BSP_SPI_tx_byte(hi);
        BSP_SPI_tx_byte(lo);
    }
    CS_1;
}

/* DMA 版区域填充 */
void ST7789_clearRawDMA(uint16_t color, uint16_t w, uint16_t h)
{
    /* 填充一行 DMA buffer, 逐行发送 */
    uint16_t pixels_per_dma = (w < DMA_ROW_MAX) ? w : DMA_ROW_MAX;
    for (uint16_t i = 0; i < pixels_per_dma; i++) dma_row[i] = color;

    DC_1; CS_0;
    for (uint16_t row = 0; row < h; row++) {
        spi_dma_row(dma_row, w);
    }
    CS_1;
}

/* 快速字符串绘制: 1 次 setWindows, DMA 逐行发送, 零 Paint 开销 */
void ST7789_drawStringFast(uint16_t x, uint16_t y, const char *str,
                           const uint8_t *font_table, uint16_t font_w, uint16_t font_h,
                           uint16_t fg, uint16_t bg)
{
    size_t len = strlen(str);
    uint16_t w = (uint16_t)len * font_w;
    uint16_t h = font_h;
    uint16_t bpc = font_w / 8 + (font_w % 8 ? 1 : 0);  /* bytes per char row */
    uint16_t char_bytes = font_h * bpc;                  /* bytes per char  */

    ST7789_setWindows(x, y, x + w - 1, y + h - 1);
    DC_1; CS_0;

    for (uint16_t row = 0; row < h; row++) {
        uint16_t *dst = dma_row;
        for (size_t ci = 0; ci < len; ci++) {
            const uint8_t *ptr = &font_table[
                ((uint8_t)str[ci] - ' ') * char_bytes + row * bpc
            ];
            for (uint16_t col = 0; col < font_w; col++) {
                *dst++ = (*ptr & (0x80 >> (col % 8))) ? fg : bg;
                if (col % 8 == 7) ptr++;
            }
            if (font_w % 8 != 0) ptr++;
        }
        spi_dma_row(dma_row, w);
    }
    CS_1;
}

void ST7789_drawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    ST7789_setWindows(x, y, x, y);
    send_data16(color);
}

void ST7789_backLight(uint8_t on)
{
    if (on) BLK_1; else BLK_0;
}
