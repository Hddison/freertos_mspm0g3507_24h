/*
 *  ============ task_flash.c =============
 *  W25Q128 Flash 任务 — DMA 读写测试
 */

#include "task_flash.h"

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <string.h>

#include "bsp_uart.h"
#include "hw_w25q128.h"

/* 测试数据 */
static const uint8_t test_wr[] = "Hello W25Q128 DMA Test!";
static uint8_t test_rd[32];

static void prvFlashTask(void *pvParameters)
{
    (void)pvParameters;
    char msg[80];
    bool ok;

    vTaskDelay(pdMS_TO_TICKS(500));

    /* ── 1. ID + SR1 ── */
    uint16_t id = HW_W25Q128_readID();
    snprintf(msg, sizeof(msg), "[Flash] ID: 0x%04X %s\r\n",
             id, (id == 0xEF17) ? "OK" : "?");
    BSP_UART_tx_str(msg);

    /* ── 2. 读 Sector 0 原始内容 ── */
    memset(test_rd, 0, sizeof(test_rd));
    HW_W25Q128_read(test_rd, 0, sizeof(test_rd));
    snprintf(msg, sizeof(msg), "[Flash] Before: %02X %02X %02X %02X %02X %02X %02X %02X ...\r\n",
             test_rd[0], test_rd[1], test_rd[2], test_rd[3],
             test_rd[4], test_rd[5], test_rd[6], test_rd[7]);
    BSP_UART_tx_str(msg);

    /* ── 3. 擦除 Sector 0 ── */
    BSP_UART_tx_str("[Flash] Erasing Sector 0... ");
    ok = HW_W25Q128_eraseSector(0);
    BSP_UART_tx_str(ok ? "OK\r\n" : "FAIL!\r\n");

    /* ── 4. 写入测试数据 ── */
    snprintf(msg, sizeof(msg), "[Flash] Writing %u bytes... ", (unsigned)sizeof(test_wr));
    BSP_UART_tx_str(msg);
    ok = HW_W25Q128_write((uint8_t *)test_wr, 0, sizeof(test_wr));
    BSP_UART_tx_str(ok ? "OK\r\n" : "FAIL!\r\n");

    /* ── 5. 回读验证 ── */
    memset(test_rd, 0, sizeof(test_rd));
    HW_W25Q128_read(test_rd, 0, sizeof(test_wr));
    snprintf(msg, sizeof(msg), "[Flash] After : %02X %02X %02X %02X %02X %02X %02X %02X ...\r\n",
             test_rd[0], test_rd[1], test_rd[2], test_rd[3],
             test_rd[4], test_rd[5], test_rd[6], test_rd[7]);
    BSP_UART_tx_str(msg);

    bool match = (memcmp(test_rd, test_wr, sizeof(test_wr)) == 0);
    snprintf(msg, sizeof(msg), "[Flash] Verify: %s\r\n", match ? "MATCH OK!" : "MISMATCH!");
    BSP_UART_tx_str(msg);

    BSP_UART_tx_str("[Flash] Test done\r\n");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskFlash_create(void)
{
    BaseType_t ret = xTaskCreate(
        prvFlashTask, "Flash", 768, NULL,
        tskIDLE_PRIORITY + 5, NULL);
    if (ret != pdPASS) {
        BSP_UART_tx_str("[Flash] Failed to create task!\r\n");
    }
}
