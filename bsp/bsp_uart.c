/*
 *  ============ bsp_uart.c =============
 *  BSP UART — DL_UART_* 直调封装
 *
 *  接收提供超时版本, 防止无数据时永久阻塞
 */

#include "bsp_uart.h"
#include "bsp_system.h"

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

bool BSP_UART_rx_byte_timeout(uint8_t *data, uint32_t timeout_ms)
{
    if (!data) return false;

    /* 每 ms 检查约 8000 次 (80MHz / 10000), 足够轮询 115200 bps */
    uint32_t checks_per_ms = 8000;
    uint32_t total = timeout_ms * checks_per_ms;

    for (uint32_t i = 0; i < total; i++) {
        if (!DL_UART_isRXFIFOEmpty(UART_0_INST)) {
            *data = DL_UART_receiveDataBlocking(UART_0_INST);
            return true;
        }
    }
    return false;  /* timeout */
}

bool BSP_UART_rx_ready(void)
{
    return !DL_UART_isRXFIFOEmpty(UART_0_INST);
}
