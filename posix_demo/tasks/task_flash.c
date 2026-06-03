/*
 *  ============ task_flash.c =============
 *  W25Q128 Flash 任务 — DMA 读写测试
 */

#include "task_flash.h"

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <string.h>

#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_dma.h>

#include "bsp_uart.h"
#include "bsp_spi.h"
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

    /* ── 2a. CPU 轮询写入 ── */
    {
        const uint8_t cpu_data[] = {0xAA, 0x55, 0x12, 0x34, 0xAB, 0xCD, 0x98, 0x76};
        BSP_UART_tx_str("[Flash] CPU write test: ");
        HW_W25Q128_eraseSector(0);
        HW_W25Q128_write((uint8_t *)cpu_data, 0, sizeof(cpu_data));
        /* 回读 */
        uint8_t rd[8];
        HW_W25Q128_read(rd, 0, 8);
        snprintf(msg, sizeof(msg), "%02X%02X%02X%02X%02X%02X%02X%02X %s\r\n",
                 rd[0],rd[1],rd[2],rd[3],rd[4],rd[5],rd[6],rd[7],
                 (memcmp(rd, cpu_data, 8)==0) ? "CPU-OK" : "CPU-FAIL");
        BSP_UART_tx_str(msg);
    }

    /* ── 4b. DMA 写入 ── */
    {
        const uint8_t dma_data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
        BSP_UART_tx_str("[Flash] DMA write test: ");
        /* 直接用底层调用, 绕开 HW_W25Q128_write */
        HW_W25Q128_eraseSector(0);
        /* --- 手动 DMA 写过程 --- */
        /* wren + 命令用 CPU */
        DL_GPIO_clearPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);
        BSP_SPI_txrx_byte(0x06);  /* WREN */
        DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);
        /* wait busy */
        { uint8_t sr; do {
            DL_GPIO_clearPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);
            BSP_SPI_txrx_byte(0x05); sr = BSP_SPI_txrx_byte(0xFF);
            DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);
        } while (sr & 0x01); }
        /* 页编程命令 */
        DL_GPIO_clearPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);
        BSP_SPI_txrx_byte(0x02);
        BSP_SPI_txrx_byte(0x00); BSP_SPI_txrx_byte(0x00); BSP_SPI_txrx_byte(0x00);
        /* DMA 发送数据 */
        DL_DMA_disableChannel(DMA, 1);
        DL_DMA_setSrcAddr(DMA, 1, (uint32_t)dma_data);
        DL_DMA_setDestAddr(DMA, 1, (uint32_t)&SPI_LCD_INST->TXDATA);
        DL_DMA_setTransferSize(DMA, 1, 8);
        DL_DMA_enableChannel(DMA, 1);
        while (DL_DMA_getTransferSize(DMA, 1) != 0);
        DL_DMA_disableChannel(DMA, 1);
        DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);
        /* wait busy */
        { uint8_t sr; do {
            DL_GPIO_clearPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);
            BSP_SPI_txrx_byte(0x05); sr = BSP_SPI_txrx_byte(0xFF);
            DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);
        } while (sr & 0x01); }
        /* 回读 */
        uint8_t rd2[8];
        HW_W25Q128_read(rd2, 0, 8);
        snprintf(msg, sizeof(msg), "%02X%02X%02X%02X%02X%02X%02X%02X %s\r\n",
                 rd2[0],rd2[1],rd2[2],rd2[3],rd2[4],rd2[5],rd2[6],rd2[7],
                 (memcmp(rd2, dma_data, 8)==0) ? "DMA-OK" : "DMA-FAIL");
        BSP_UART_tx_str(msg);
    }

    BSP_UART_tx_str("[Flash] Test done\r\n");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
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
