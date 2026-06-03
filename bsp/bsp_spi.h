/*
 *  ============ bsp_spi.h =============
 *  BSP SPI — DL_SPI_* 直调封装 (参考 Keil 11_spi)
 *
 *  SysConfig 已配置 SPI_LCD (SPI1): PB9(SCLK)/PB8(MOSI)/PB7(MISO), 10MHz, Mode0
 */

#ifndef BSP_SPI_H
#define BSP_SPI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t BSP_SPI_txrx_byte(uint8_t data);
void    BSP_SPI_tx_byte(uint8_t data);
void    BSP_SPI_tx(const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
