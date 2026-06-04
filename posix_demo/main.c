/*
 * main.c — Test 4b: Flash R/W stress under FreeRTOS (HAL mutex)
 */
#include <FreeRTOS.h>
#include <task.h>
#include "ti_msp_dl_config.h"
#include "bsp_uart.h"
#include "bsp_system.h"
#include "hal_spi.h"
#include "hw_w25q128.h"
#include <ti/driverlib/dl_gpio.h>

/* Stack overflow hook (configCHECK_FOR_STACK_OVERFLOW=2 需要) */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    BSP_UART_tx_str("STACK OVERFLOW: ");
    BSP_UART_tx_str(pcTaskName);
    BSP_UART_tx_str("\r\n");
    for (;;) {}
}

/* ── Writer Task ── */
static void prvWriterTask(void *pvParameters)
{
    (void)pvParameters;
    BSP_UART_tx_str("[WRITER] started\r\n");

    uint8_t wbuf[32];
    for (int i = 0; i < 32; i++) wbuf[i] = (uint8_t)(0x50 + i);

    uint32_t cycle = 0;
    for (;;) {
        if (!HW_W25Q128_write(wbuf, 0x5000, 32)) {
            BSP_UART_tx_str("[WRITER] write FAIL\r\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        uint8_t rbuf[32];
        if (!HW_W25Q128_read(rbuf, 0x5000, 32)) {
            BSP_UART_tx_str("[WRITER] read FAIL\r\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        bool ok = true;
        for (int i = 0; i < 32; i++) {
            if (wbuf[i] != rbuf[i]) { ok = false; break; }
        }

        BSP_UART_tx_str(ok ? "[WRITER] PASS\r\n" : "[WRITER] FAIL\r\n");

        cycle++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ── Reader Task ── */
static void prvReaderTask(void *pvParameters)
{
    (void)pvParameters;
    BSP_UART_tx_str("[READER] started\r\n");

    uint32_t count = 0;
    for (;;) {
        uint16_t id = HW_W25Q128_readID();
        if (id != 0xEF17) {
            BSP_UART_tx_str("[READER] BAD ID\r\n");
        }
        count++;
        if (count % 10 == 0) {
            BSP_UART_tx_str("[READER] 10 OK\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* ── main ── */
int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);
    BSP_delay_ms(100);

    BSP_UART_tx_str("\r\n=== Test 4b: Flash stress (RTOS) ===\r\n");

    HAL_SPI_init();

    /* 屏蔽所有外设中断 — 只留 FreeRTOS 的 SVC/PendSV/SysTick */
    for (IRQn_Type irq = GPIOA_INT_IRQn; irq <= DMA_INT_IRQn; irq++) {
        NVIC_DisableIRQ(irq);
    }

    /* stack >= configMINIMAL_STACK_SIZE (256) */
    if (xTaskCreate(prvWriterTask, "WRITER", 1024, NULL, 2, NULL) != pdPASS) {
        BSP_UART_tx_str("FATAL: WRITER\r\n"); for (;;) {}
    }
    if (xTaskCreate(prvReaderTask, "READER", 512, NULL, 1, NULL) != pdPASS) {
        BSP_UART_tx_str("FATAL: READER\r\n"); for (;;) {}
    }

    BSP_UART_tx_str("FreeRTOS starting...\r\n");
    vTaskStartScheduler();
    for (;;) {}
}
