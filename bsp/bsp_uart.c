/*
 *  ============ bsp_uart.c =============
 *  BSP UART — DL_UART_* 直调封装
 */

#include "bsp_uart.h"

#include "ti_msp_dl_config.h"      /* UART_0_INST */
#include <ti/driverlib/dl_uart.h>  /* DL_UART_* */

void BSP_UART_tx_byte(uint8_t data)
{
    DL_UART_transmitDataBlocking(UART_0_INST, data);
}

void BSP_UART_tx_str(const char *str)
{
    while (*str) {
        DL_UART_transmitDataBlocking(UART_0_INST, (uint8_t)*str++);
    }
}

uint8_t BSP_UART_rx_byte(void)
{
    return DL_UART_receiveDataBlocking(UART_0_INST);
}

bool BSP_UART_rx_ready(void)
{
    return !DL_UART_isRXFIFOEmpty(UART_0_INST);
}
