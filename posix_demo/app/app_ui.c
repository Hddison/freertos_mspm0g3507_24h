/*
 *  ============ app_ui.c =============
 *  UI 工具函数实现 — 从 main.c 提取
 */

#include "app_ui.h"

#include "hw_st7789.h"

/* ══════ float → string (1-2 位小数) ══════ */
int app_ftoa(char *buf, float val, uint8_t dec)
{
    int pos = 0;
    if (val < 0.0f) { buf[pos++] = '-'; val = -val; }
    int ip = (int)val;
    if (ip == 0) { buf[pos++] = '0'; }
    else {
        char t[8]; int tp = 0;
        while (ip) { t[tp++] = '0' + (ip % 10); ip /= 10; }
        while (tp) buf[pos++] = t[--tp];
    }
    buf[pos++] = '.';
    int fd = (int)((val - (int)val) * (dec == 2 ? 100.0f : 10.0f) + 0.5f);
    int limit = (dec == 2 ? 100 : 10);
    if (fd >= limit) fd = limit - 1;
    if (dec == 2) { buf[pos++] = '0' + (fd/10); buf[pos++] = '0' + (fd%10); }
    else          { buf[pos++] = '0' + fd; }
    buf[pos] = '\0';
    return pos;
}

/* ══════ 数值区域: DMA 擦 + 写 ══════ */
void app_draw_val(uint16_t x, uint16_t y, float val, uint8_t dec,
                  const uint8_t *tbl, uint16_t fw, uint16_t fh,
                  uint16_t fg)
{
    char buf[12];
    uint16_t w = (uint16_t)app_ftoa(buf, val, dec) * fw;
    ST7789_setWindows(x, y, x + w - 1, y + fh - 1);
    ST7789_clearRawDMA(BLACK, w, fh);
    ST7789_drawStringFast(x, y, buf, tbl, fw, fh, fg, BLACK);
}
