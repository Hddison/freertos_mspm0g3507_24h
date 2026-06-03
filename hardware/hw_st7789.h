/*
 *  ============ hw_st7789.h =============
 *  ST7789 1.9" LCD 驱动 (170x320)
 */

#ifndef HW_ST7789_H
#define HW_ST7789_H

#include <stdint.h>

#define ST7789_WIDTH          170
#define ST7789_HEIGHT         320
#define ST7789_COLUMN_OFFSET  35    /* 170px panel centered in 240px native frame */

typedef enum { ST7789_HORIZONTAL = 0, ST7789_VERTICAL = 1, ST7789_CCW90 = 2 } ST7789_DIR;

void ST7789_init(ST7789_DIR dir);
void ST7789_clear(uint16_t color);
void ST7789_clearRaw(uint16_t color, uint16_t w, uint16_t h);   /* 填充已设窗口 */
void ST7789_clearRawDMA(uint16_t color, uint16_t w, uint16_t h); /* DMA 填充 */
void ST7789_drawStringFast(uint16_t x, uint16_t y, const char *str,
                           const uint8_t *font_table, uint16_t font_w, uint16_t font_h,
                           uint16_t fg, uint16_t bg);
void ST7789_setWindows(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ST7789_drawPoint(uint16_t x, uint16_t y, uint16_t color);
void ST7789_backLight(uint8_t on);

#endif
