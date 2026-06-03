/*
 *  ============ task_flash.c =============
 *  W25Q128 Flash 测试任务实现
 *
 *  DMA 参考模板:
 *   1. 上电读取 ID → 打印
 *   2. 主循环通过队列接收读写请求 (TODO)
 *   3. 执行 Flash 操作 → 返回结果
 */

#include "task_flash.h"

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>

#include "app/app_config.h"
#include "bsp_uart.h"
#include "hw_w25q128.h"

static void prvFlashTask(void *pvParameters)
{
    (void)pvParameters;

    /* ── 上电等待外设稳定 ── */
    vTaskDelay(pdMS_TO_TICKS(500));

    /* ── 读取 Flash ID ── */
    uint16_t id = HW_W25Q128_readID();
    char msg[64];
    snprintf(msg, sizeof(msg), "[Flash] ID: 0x%04X %s\r\n",
             id, (id == 0xEF17) ? "W25Q128 OK" : "UNKNOWN");
    BSP_UART_tx_str(msg);

    /* ── 读 SR1 ── */
    uint8_t sr1 = HW_W25Q128_readSR1();
    snprintf(msg, sizeof(msg), "[Flash] SR1: 0x%02X\r\n", sr1);
    BSP_UART_tx_str(msg);

    /* ── 测试: 读 Sector 0 前 16 字节 ── */
    vTaskDelay(pdMS_TO_TICKS(100));
    {
        uint8_t buf[16];
        if (HW_W25Q128_read(buf, 0, 16)) {
            snprintf(msg, sizeof(msg),
                "[Flash] S0: %02X %02X %02X %02X %02X %02X %02X %02X "
                "%02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
                buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
            BSP_UART_tx_str(msg);
        } else {
            BSP_UART_tx_str("[Flash] Read FAILED!\r\n");
        }
    }

    BSP_UART_tx_str("[Flash] Task ready\r\n");

    /* ══════ 主循环 ══════ */
    for (;;) {
        /*
         * TODO: 接收读写请求
         *   - 从队列接收 FlashRequest
         *   - 执行对应 Flash 操作
         *   - 通过队列返回结果
         */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void TaskFlash_create(void)
{
    BaseType_t ret = xTaskCreate(
        prvFlashTask,
        "Flash",
        512,               /* 栈: 512 字 */
        NULL,
        tskIDLE_PRIORITY + 5,  /* 最低优先级 */
        NULL
    );
    if (ret != pdPASS) {
        BSP_UART_tx_str("[Flash] Failed to create task!\r\n");
    }
}
