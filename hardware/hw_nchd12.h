/*
 *  ============ hw_nchd12.h =============
 *  NC-HD12 12 路灰度传感器 (PCA9555 I2C GPIO 扩展)
 *
 *  I2C1: PB3(SDA)/PA29(SCL), 100kHz
 *  PCA9555: A0-A2 接地, 地址 0x20
 */

#ifndef HW_NCHD12_H
#define HW_NCHD12_H

#include <stdbool.h>
#include <stdint.h>

#define NCHD12_ADDR         0x20
#define NCHD12_REG_INPUT0   0x00

bool NCHD12_read(uint16_t *bits);

#endif
