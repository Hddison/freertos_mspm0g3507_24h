/*
 * ============ bsp_i2c.h =============
 * I2C 通信 (Controller 模式, 带超时 + 总线恢复)
 *
 * I2C0 (1MHz):  PA0 SDA / PA1 SCL — JY61P IMU
 * I2C1 (100kHz): PB3 SDA / PA29 SCL — NCHD12 灰度
 *
 * SysConfig 已配置硬件, 本模块封装读写 API。
 * DMA 读用于高性能连续读取 (IMU / 灰度传感器)。
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

/* 上电初始化: 检测 SDA 是否被锁低, 若锁低则执行总线恢复。
 * 需在 SYSCFG_DL_init() 后调用。 */
void BSP_I2C_init(void);

/* ── CPU 轮询读写 ── */

/* I2C0 写: 发送 reg + data[len] → addr。
 * 超时或总线异常自动恢复, 返回 false。 */
bool BSP_I2C_write(uint8_t addr, uint8_t reg,
                   const uint8_t *data, size_t len);

/* I2C0 读: 发送 reg → 接收 len 字节到 data。
 * 超时或总线异常自动恢复, 返回 false。 */
bool BSP_I2C_read(uint8_t addr, uint8_t reg,
                  uint8_t *data, size_t len);

/* ── DMA 读 ── */

/* I2C0 DMA 读 (快捷版, 固定用 DMA_CH3) */
bool BSP_I2C_read_dma(uint8_t addr, uint8_t reg,
                      uint8_t *data, size_t len);

/* 通用 DMA 读: 指定 I2C 实例和 DMA 通道。
 * I2C0 → DMA_I2C_RX_CHAN_ID (3), I2C1 → DMA_CH0_CHAN_ID (2) */
bool BSP_I2C_read_dma_ex(I2C_Regs *i2c, uint8_t dma_ch,
                         uint8_t addr, uint8_t reg,
                         uint8_t *data, size_t len);

/* ── 总线恢复 ── */

/* I2C0 总线恢复: SCL 发 100 个脉冲直到 SDA 释放, 然后重新初始化外设 */
void BSP_I2C_recover(void);

#ifdef __cplusplus
}
#endif

#endif
