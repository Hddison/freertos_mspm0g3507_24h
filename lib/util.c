/*
 * ============ util.c ============
 * 轻量工具函数 (无标准库依赖, 线程安全)
 */
#include "util.h"

int util_itoa(int32_t val, char *buf)
{
    int len = 0;
    uint32_t u;
    if (val < 0) {
        buf[len++] = '-';
        u = (uint32_t)(-(val + 1)) + 1;
    } else {
        u = (uint32_t)val;
    }
    if (u == 0) { buf[len++] = '0'; buf[len] = 0; return len; }
    int start = len;
    for (uint32_t t = u; t; t /= 10) len++;
    buf[len] = 0;
    for (int i = len - 1; i >= start; i--) { buf[i] = '0' + (u % 10); u /= 10; }
    return len;
}

int util_ftoa(float val, int dec, char *buf)
{
    int len = 0;
    if (val < 0) { buf[len++] = '-'; val = -val; }
    float scale = 1.0f;
    for (int i = 0; i < dec; i++) scale *= 10.0f;
    int ipart = (int)val;
    int fpart = (int)((val - (float)ipart) * scale + 0.5f);
    if (fpart >= (int)scale) { ipart++; fpart = 0; }

    len += util_itoa(ipart, buf + len);
    if (dec > 0) {
        buf[len++] = '.';
        int div = 1;
        for (int i = 1; i < dec; i++) div *= 10;
        for (int i = 0; i < dec; i++) {
            buf[len++] = '0' + ((fpart / div) % 10);
            div /= 10;
        }
    }
    buf[len] = 0;
    return len;
}

void util_bits12(uint16_t bits, char *out)
{
    for (int i = 0; i < 12; i++)
        out[i] = (bits & (1 << (11 - i))) ? '1' : '0';
    out[12] = 0;
}
