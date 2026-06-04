/*
 * main.c — Test 3: Buzzer (FreeRTOS, UART-triggered)
 */
#include <FreeRTOS.h>
#include <task.h>
#include "ti_msp_dl_config.h"
#include "bsp_uart.h"
#include "hw_buzzer.h"
#include <stdio.h>

static TaskHandle_t g_hBuzzer = NULL;

/* ── Buzzer Task ── */
static void prvBuzzerTask(void *pvParameters)
{
    (void)pvParameters;
    Buzzer_init();
    BSP_UART_tx_str("[BUZZ] Task started\r\n");

    for (;;) {
        uint32_t count;
        if (xTaskNotifyWait(0, 0, &count, portMAX_DELAY) != pdPASS)
            continue;

        char buf[32];
        int n = snprintf(buf, sizeof(buf),
                         "[BUZZ] beep x%lu\r\n", (unsigned long)count);
        if (n > 0) BSP_UART_tx_dma((const uint8_t *)buf, (uint16_t)n);

        for (uint32_t i = 0; i < count; i++) {
            Buzzer_set(1); vTaskDelay(pdMS_TO_TICKS(150));
            Buzzer_set(0); vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

/* ── UART RX Task ──
 * 接收一个字符: '1'~'9' → beep N 次, 其他字符 → beep 1 次 */
static void prvUartRxTask(void *pvParameters)
{
    (void)pvParameters;
    BSP_UART_tx_str("[UART] Rx task started, type to beep...\r\n");

    for (;;) {
        uint8_t c;
        if (BSP_UART_rx_byte_timeout(&c, 100)) {
            /* 回显 */
            BSP_UART_tx_byte(c);

            uint32_t count = 1;
            if (c >= '1' && c <= '9') {
                count = (uint32_t)(c - '0');
            }

            if (g_hBuzzer) {
                xTaskNotify(g_hBuzzer, count, eSetValueWithOverwrite);
            }
        }
        /* no delay needed — rx_byte_timeout already polls with delay */
    }
}

/* ── main ── */
int main(void)
{
    SYSCFG_DL_init();
    BSP_UART_tx_str("\r\n=== Test 3: Buzzer (UART trigger) ===\r\n");

    if (xTaskCreate(prvBuzzerTask, "BUZZ", 256, NULL, 3, &g_hBuzzer) != pdPASS) {
        BSP_UART_tx_str("FATAL: BUZZ\r\n"); for (;;) {}
    }
    if (xTaskCreate(prvUartRxTask, "URX", 256, NULL, 2, NULL) != pdPASS) {
        BSP_UART_tx_str("FATAL: URX\r\n"); for (;;) {}
    }

    BSP_UART_tx_str("FreeRTOS starting...\r\n");
    vTaskStartScheduler();
    for (;;) {}
}
