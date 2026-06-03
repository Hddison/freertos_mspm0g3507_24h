/*
 *  ============ bsp_i2c.h =============
 *  BSP I2C — 参考 mpu6050-oled-hardware-i2c 工程
 *
 *  SysConfig 已配置 I2C0: PA0(SDA)/PA1(SCL), 400kHz, Controller 模式
 */

#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ti/driverlib/dl_i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

void BSP_I2C_init(void);
bool BSP_I2C_write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);
bool BSP_I2C_read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);
bool BSP_I2C_read_dma(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);
bool BSP_I2C_read_dma_ex(I2C_Regs *i2c, uint8_t dma_ch,
                         uint8_t addr, uint8_t reg,
                         uint8_t *data, size_t len);
void BSP_I2C_recover(void);

#ifdef __cplusplus
}
#endif

#endif
