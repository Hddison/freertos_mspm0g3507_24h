/*
 *  ============ bsp_uart.h =============
 *  BSP UART — DL_UART_* 直调封装
 *
 *  SysConfig 已配置 UART0: PA10(TX)/PA11(RX), 9600bps, 8N1
 *  BSP 仅封装发送/接收 API, 不重新初始化硬件
 */

#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void    BSP_UART_tx_byte(uint8_t data);
void    BSP_UART_tx_str(const char *str);
uint8_t BSP_UART_rx_byte(void);
bool    BSP_UART_rx_ready(void);

#ifdef __cplusplus
}
#endif

#endif
