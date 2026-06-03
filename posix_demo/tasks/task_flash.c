/*
 *  ============ task_flash.c =============
 *  W25Q128 Flash 任务 — DMA 读写参考模板
 */

#include "task_flash.h"

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>

#include "bsp_uart.h"
#include "hw_w25q128.h"

static void prvFlashTask(void *pvParameters)
{
    (void)pvParameters;
    char msg[64];

    vTaskDelay(pdMS_TO_TICKS(500));

    /* ── Flash ID ── */
    uint16_t id = HW_W25Q128_readID();
    snprintf(msg, sizeof(msg), "[Flash] ID: 0x%04X %s\r\n",
             id, (id == 0xEF17) ? "W25Q128 OK" : "UNKNOWN");
    BSP_UART_tx_str(msg);

    /* ── SR1 ── */
    uint8_t sr1 = HW_W25Q128_readSR1();
    snprintf(msg, sizeof(msg), "[Flash] SR1: 0x%02X\r\n", sr1);
    BSP_UART_tx_str(msg);

    BSP_UART_tx_str("[Flash] Ready\r\n");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskFlash_create(void)
{
    BaseType_t ret = xTaskCreate(
        prvFlashTask, "Flash", 512, NULL,
        tskIDLE_PRIORITY + 5, NULL);
    if (ret != pdPASS) {
        BSP_UART_tx_str("[Flash] Failed to create task!\r\n");
    }
}
