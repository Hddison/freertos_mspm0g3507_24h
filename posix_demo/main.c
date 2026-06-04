/*
 * main.c — Test 1: UART + LED + DMA TX+RX (FreeRTOS)
 */
#include <FreeRTOS.h>
#include <task.h>
#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>
#include "bsp_uart.h"
#include <stdio.h>

/* ── LED Task ── */
static void prvLedTask(void *pvParameters)
{
    (void)pvParameters;

    const char *start = "[LED] Task started (DMA)\r\n";
    BSP_UART_tx_dma((const uint8_t *)start, 24);

    uint32_t tick = 0;
    for (;;) {
        DL_GPIO_togglePins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);

        char buf[40];
        int n = snprintf(buf, sizeof(buf),
                         "[LED] tick=%lu DMA\r\n", (unsigned long)tick++);
        if (n > 0 && n < (int)sizeof(buf)) {
            BSP_UART_tx_dma((const uint8_t *)buf, (uint16_t)n);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ── main ── */
int main(void)
{
    SYSCFG_DL_init();

    /* ── 1. 启动横幅 (阻塞) ── */
    BSP_UART_tx_str("\r\n=== Test 1: UART+LED+DMA TX+RX (FreeRTOS) ===\r\n");

    /* ── 2. TX DMA 自检 ── */
    const char *dma_test = "[DMA TX] pre-check: Hello from DMA!\r\n";
    bool dma_ok = BSP_UART_tx_dma((const uint8_t *)dma_test, 36);
    BSP_UART_tx_str(dma_ok ? "[DMA TX] self-test PASS\r\n"
                            : "[DMA TX] self-test FAIL\r\n");

    /* ── 3. RX DMA 回环测试 ── */
    BSP_UART_tx_str("[DMA RX] Type something (5s timeout)...\r\n");

    uint8_t rxbuf[128] = {0};
    uint16_t n = BSP_UART_rx_dma(rxbuf, sizeof(rxbuf), 5000);

    if (n > 0) {
        /* ---- 回显字节数 ---- */
        char info[32];
        int ilen = snprintf(info, sizeof(info),
                            "[DMA RX] got %u bytes: \"", (unsigned int)n);
        BSP_UART_tx_dma((const uint8_t *)info, (uint16_t)ilen);

        /* ---- 回显内容 ---- */
        BSP_UART_tx_dma(rxbuf, n);

        /* ---- 闭合引号 ---- */
        BSP_UART_tx_dma((const uint8_t *)"\"\r\n", 3);
    } else {
        BSP_UART_tx_str("[DMA RX] timeout (no data)\r\n");
    }

    /* ── 4. FreeRTOS LED Task ── */
    if (xTaskCreate(prvLedTask, "LED", 256, NULL, 1, NULL) != pdPASS) {
        BSP_UART_tx_str("FATAL: LED task create failed\r\n");
        for (;;) {}
    }

    BSP_UART_tx_str("FreeRTOS starting...\r\n");
    vTaskStartScheduler();

    /* unreachable */
    for (;;) {}
}
