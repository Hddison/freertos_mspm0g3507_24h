/*
 * ============ hal_spi.h =============
 * HAL 层 — SPI 总线互斥 (FreeRTOS mutex)
 *
 * SPI1 共享于 W25Q128 Flash 和 ST7789 LCD。
 * HW 层调用 HAL_SPI_lock/unlock 包裹每个 SPI 事务。
 */

#ifndef HAL_SPI_H
#define HAL_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化 SPI 总线互斥锁 (调度器启动前调用) */
void HAL_SPI_init(void);

/* 获取 SPI 总线锁 (阻塞直到可用) */
void HAL_SPI_lock(void);

/* 释放 SPI 总线锁 */
void HAL_SPI_unlock(void);

#ifdef __cplusplus
}
#endif

#endif
