/*
 *  ============ hw_nchd12.c =============
 *  NC-HD12 12 路灰度传感器 — PCA9555 驱动
 *
 *  读 Input Port 0 + Port 1 → 12-bit 位图
 *  上位对应传感器通道 1 (最左), 下位对应通道 12 (最右)
 */

#include "hw_nchd12.h"

#include "ti_msp_dl_config.h"
#include "bsp_i2c.h"

bool NCHD12_read(uint16_t *bits)
{
    if (!bits) return false;

    uint8_t buf[2];
    if (!BSP_I2C_read_dma_ex(I2C_NCHD12_INST, DMA_CH0_CHAN_ID,
                             NCHD12_ADDR, NCHD12_REG_INPUT0, buf, 2))
        return false;

    *bits = ((uint16_t)buf[1] << 8 | buf[0]) & 0x0FFF;
    return true;
}
