/*
 * ============ util.h ============
 * 轻量工具函数 (无标准库依赖, 线程安全)
 */
#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 整数转字符串, 返回长度 */
int util_itoa(int32_t val, char *buf);

/* 浮点数转字符串, dec=小数位数 */
int util_ftoa(float val, int dec, char *buf);

/* 12-bit → 二进制字符串 "000000000000" */
void util_bits12(uint16_t bits, char *out);

#ifdef __cplusplus
}
#endif

#endif
