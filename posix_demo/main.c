/*
 * main.c — Test 6: LCD + Flash concurrent (FreeRTOS, HAL mutex)
 */
#include <FreeRTOS.h>
#include <task.h>
#include "ti_msp_dl_config.h"
#include "bsp_uart.h"
#include "bsp_system.h"
#include "hal_spi.h"
#include "hw_w25q128.h"
#include "hw_st7789.h"
#include <ti/driverlib/dl_gpio.h>
#include "gui_paint.h"

extern const uint8_t Font20_Table[];

/* Stack overflow hook */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    BSP_UART_tx_str("STACK OVERFLOW: ");
    BSP_UART_tx_str(pcTaskName);
    BSP_UART_tx_str("\r\n");
    for (;;) {}
}

/* ── LCD Task: 循环填色 + 文字 ── */
static void prvLcdTask(void *pvParameters)
{
    (void)pvParameters;
    BSP_UART_tx_str("[LCD] started\r\n");

    uint16_t colors[] = { RED, GREEN, BLUE };
    const char *names[] = { "RED", "GREEN", "BLUE" };
    const uint16_t txt_colors[] = { WHITE, BLACK, WHITE };
    int idx = 0;
    for (;;) {
        /* 全屏填色 */
        ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
        ST7789_clearRawDMA(colors[idx], ST7789_WIDTH, ST7789_HEIGHT);

        /* 显示颜色名 */
        ST7789_drawStringFast(30, 140, names[idx], Font20_Table, 16, 20,
                              txt_colors[idx], colors[idx]);

        idx = (idx + 1) % 3;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* ── Flash Task: 反复写+读+校验 ── */
static void prvFlashTask(void *pvParameters)
{
    (void)pvParameters;
    BSP_UART_tx_str("[FLASH] started\r\n");

    uint8_t wbuf[32];
    for (int i = 0; i < 32; i++) wbuf[i] = (uint8_t)(0xA0 + i);

    uint32_t cycle = 0;
    for (;;) {
        if (!HW_W25Q128_write(wbuf, 0x5000, 32)) {
            BSP_UART_tx_str("[FLASH] write FAIL\r\n");
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        uint8_t rbuf[32];
        if (!HW_W25Q128_read(rbuf, 0x5000, 32)) {
            BSP_UART_tx_str("[FLASH] read FAIL\r\n");
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        bool ok = true;
        for (int i = 0; i < 32; i++) {
            if (wbuf[i] != rbuf[i]) { ok = false; break; }
        }

        cycle++;
        if (ok) {
            BSP_UART_tx_str("[FLASH] PASS\r\n");
        } else {
            BSP_UART_tx_str("[FLASH] FAIL\r\n");
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

    BSP_UART_tx_str("\r\n=== Test 6: LCD + Flash (RTOS) ===\r\n");

    ST7789_init(ST7789_HORIZONTAL);
    ST7789_backLight(1);
    ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);
    BSP_UART_tx_str("LCD init done\r\n");

    HAL_SPI_init();

    NVIC_DisableIRQ(UART_0_INST_INT_IRQN);
    NVIC_DisableIRQ(GPIOA_INT_IRQn);

    if (xTaskCreate(prvLcdTask,   "LCD",   1024, NULL, 1, NULL) != pdPASS ||
        xTaskCreate(prvFlashTask, "FLASH",  768, NULL, 2, NULL) != pdPASS) {
        BSP_UART_tx_str("FATAL\r\n"); for (;;) {}
    }

    BSP_UART_tx_str("FreeRTOS starting...\r\n");
    vTaskStartScheduler();
    for (;;) {}
}
