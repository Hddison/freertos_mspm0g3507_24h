/*
 * main.c — Test 2: Buttons (bare-metal, raw GPIO polling)
 */
#include "ti_msp_dl_config.h"
#include "bsp_uart.h"
#include "bsp_system.h"
#include <ti/driverlib/dl_gpio.h>
#include <stdio.h>

int main(void)
{
    SYSCFG_DL_init();
    BSP_UART_tx_str("\r\n=== Test 2: Buttons (raw poll) ===\r\n");

    GPIO_Regs *ports[] = {
        GPIO_KEY_PIN_UP_PORT,    GPIO_KEY_PIN_LEFT_PORT,
        GPIO_KEY_PIN_DOWN_PORT,  GPIO_KEY_PIN_RIGHT_PORT,
        GPIO_KEY_PIN_CENTER_PORT,GPIO_KEY_PIN_BUTTON_PORT,
    };
    uint32_t pins[] = {
        GPIO_KEY_PIN_UP_PIN,     GPIO_KEY_PIN_LEFT_PIN,
        GPIO_KEY_PIN_DOWN_PIN,   GPIO_KEY_PIN_RIGHT_PIN,
        GPIO_KEY_PIN_CENTER_PIN, GPIO_KEY_PIN_BUTTON_PIN,
    };
    const char *names[] = {
        "UP", "LEFT", "DOWN", "RIGHT", "CENTER", "BTN",
    };

    uint8_t prev[6] = {0};
    for (;;) {
        for (int i = 0; i < 6; i++) {
            uint8_t now = (DL_GPIO_readPins(ports[i], pins[i]) == 0);
            if (now && !prev[i]) {
                char buf[24];
                int n = snprintf(buf, sizeof(buf), "%s down\r\n", names[i]);
                if (n > 0) BSP_UART_tx_dma((const uint8_t *)buf, (uint16_t)n);
            }
            if (!now && prev[i]) {
                char buf[24];
                int n = snprintf(buf, sizeof(buf), "%s up\r\n", names[i]);
                if (n > 0) BSP_UART_tx_dma((const uint8_t *)buf, (uint16_t)n);
            }
            prev[i] = now;
        }
        BSP_delay_ms(20);
    }
}
