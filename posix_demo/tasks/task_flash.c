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

#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>

#include "app/app_config.h"
#include "bsp_uart.h"
#include "bsp_spi.h"
#include "hw_w25q128.h"

static void prvFlashTask(void *pvParameters)
{
    (void)pvParameters;

    /* ── 上电等待外设稳定 ── */
    vTaskDelay(pdMS_TO_TICKS(500));

    /* ── 测试 1: 纯 CPU 轮询读 Flash ID (绕开 DMA) ── */
    {
        uint16_t id_cpu;
        uint8_t  mfr, dev;

        /* 清除 RX FIFO 中 LCD DMA 残留数据 */
        while (!DL_SPI_isRXFIFOEmpty(SPI_LCD_INST)) {
            (void)DL_SPI_receiveData8(SPI_LCD_INST);
        }

        DL_GPIO_clearPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);   /* CS LOW */
        BSP_SPI_txrx_byte(0x90);        /* Read ID cmd */
        BSP_SPI_txrx_byte(0x00);        /* addr[23:16] */
        BSP_SPI_txrx_byte(0x00);        /* addr[15:8]  */
        BSP_SPI_txrx_byte(0x00);        /* addr[7:0]   */
        mfr = BSP_SPI_txrx_byte(0xFF);  /* Mfr  ID     */
        dev = BSP_SPI_txrx_byte(0xFF);  /* Dev  ID     */
        DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);     /* CS HIGH */
        id_cpu = ((uint16_t)mfr << 8) | dev;

        char msg[64];
        snprintf(msg, sizeof(msg), "[Flash] CPU read ID: 0x%04X (MFR=%02X DEV=%02X)\r\n",
                 id_cpu, mfr, dev);
        BSP_UART_tx_str(msg);
    }

    /* ── 测试 2: DMA 驱动读 Flash ID ── */
    {
        uint16_t id = HW_W25Q128_readID();
        char msg[64];
        snprintf(msg, sizeof(msg), "[Flash] DMA read ID: 0x%04X\r\n", id);
        BSP_UART_tx_str(msg);
    }

    /* ── 读 SR1 ── */
    {
        uint8_t sr1 = HW_W25Q128_readSR1();
        char msg[32];
        snprintf(msg, sizeof(msg), "[Flash] SR1: 0x%02X\r\n", sr1);
        BSP_UART_tx_str(msg);
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
