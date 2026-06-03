/*
 *  ============ task_flash.c =============
 *  W25Q128 Flash 读写测试 (CPU 轮询)
 */

#include "task_flash.h"

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <string.h>

#include "bsp_uart.h"
#include "hw_w25q128.h"

static void prvFlashTask(void *pvParameters)
{
    (void)pvParameters;
    char msg[80];

    vTaskDelay(pdMS_TO_TICKS(500));

    /* ── 1. ID ── */
    uint16_t id = HW_W25Q128_readID();
    snprintf(msg, sizeof(msg), "[Flash] ID: 0x%04X %s\r\n",
             id, (id == 0xEF17) ? "OK" : "?");
    BSP_UART_tx_str(msg);

    /* ── 2. 擦除 + 写入 + 回读 ── */
    const uint8_t wr[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    uint8_t rd[8];

    BSP_UART_tx_str("[Flash] Erase... ");
    BSP_UART_tx_str(HW_W25Q128_eraseSector(0) ? "OK\r\n" : "FAIL\r\n");

    snprintf(msg, sizeof(msg),
        "[Flash] Write: %02X%02X%02X%02X%02X%02X%02X%02X... ",
        wr[0],wr[1],wr[2],wr[3],wr[4],wr[5],wr[6],wr[7]);
    BSP_UART_tx_str(msg);
    BSP_UART_tx_str(HW_W25Q128_write((uint8_t *)wr, 0, 8) ? "OK\r\n" : "FAIL\r\n");

    memset(rd, 0, 8);
    HW_W25Q128_read(rd, 0, 8);
    snprintf(msg, sizeof(msg),
        "[Flash] Read : %02X%02X%02X%02X%02X%02X%02X%02X %s\r\n",
        rd[0],rd[1],rd[2],rd[3],rd[4],rd[5],rd[6],rd[7],
        (memcmp(rd, wr, 8)==0) ? "MATCH" : "MISMATCH");
    BSP_UART_tx_str(msg);

    BSP_UART_tx_str("[Flash] Done\r\n");

    for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}

void TaskFlash_create(void)
{
    BaseType_t ret = xTaskCreate(
        prvFlashTask, "Flash", 256, NULL,
        tskIDLE_PRIORITY + 5, NULL);
    if (ret != pdPASS) {
        BSP_UART_tx_str("[Flash] Failed to create task!\r\n");
    }
}
