/*
 *  ============ bsp_spi.h =============
 *  BSP SPI — DL_SPI_* 直调封装 (参考 Keil 11_spi)
 *
 *  SysConfig 已配置 SPI_LCD (SPI1): PB9(SCLK)/PB8(MOSI)/PB7(MISO), 40MHz, Mode0
 *  所有函数均包含超时保护, 返回 bool 指示成功/失败
 */

#ifndef BSP_SPI_H
#define BSP_SPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_SPI_TIMEOUT  10000000UL   /* ~500ms @ 80MHz */

uint8_t BSP_SPI_txrx_byte(uint8_t data);
bool    BSP_SPI_tx_byte(uint8_t data);
bool    BSP_SPI_tx(const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
