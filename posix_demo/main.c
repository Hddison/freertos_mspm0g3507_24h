/*
 * main.c — Test 9: Motor + Encoder + Timer + LCD
 */
#include <FreeRTOS.h>
#include <task.h>
#include "ti_msp_dl_config.h"
#include "bsp_uart.h"
#include "bsp_system.h"
#include "hal_spi.h"
#include "hw_motor.h"
#include "hw_st7789.h"
#include "gui_paint.h"
#include <ti/driverlib/dl_timera.h>

#define CYAN    0x07FF
#define YELLOW  0xFFE0

extern const uint8_t Font8_Table[];
#define FW 5
#define FH 10

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    BSP_UART_tx_str("STACK: "); BSP_UART_tx_str(pcTaskName);
    for (;;) {}
}

static int itoa_simple(int32_t val, char *buf)
{
    int len = 0;
    uint32_t u;
    if (val < 0) {
        buf[len++] = '-';
        u = (uint32_t)(-(val + 1)) + 1;  /* safe: -(INT32_MIN+1)+1 = INT32_MAX+1 */
    } else {
        u = (uint32_t)val;
    }
    if (u == 0) { buf[len++] = '0'; buf[len] = 0; return len; }
    int start = len;
    for (uint32_t t = u; t; t /= 10) len++;
    buf[len] = 0;
    for (int i = len - 1; i >= start; i--) { buf[i] = '0' + (u % 10); u /= 10; }
    return len;
}

static void draw_val(uint16_t x, uint16_t y, const char *val, uint16_t fg, uint16_t bg)
{
    ST7789_setWindows(0, y, ST7789_WIDTH - 1, y + FH - 1);
    ST7789_clearRawDMA(bg, ST7789_WIDTH, FH);
    ST7789_drawStringFast(x, y, val, Font8_Table, FW, 8, fg, bg);
}

/* ── LCD Task ── */
static void prvLcdTask(void *pvParameters)
{
    (void)pvParameters;
    ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);
    ST7789_drawStringFast(0, 0, "MOTOR", Font8_Table, FW, 8, CYAN, BLACK);

    for (;;) {
        int32_t e1 = Motor_enc1(), e2 = Motor_enc2();
        int32_t s1 = g_enc1_speed, s2 = g_enc2_speed;
        char buf[20]; int p;

        p = 0; buf[p++]='E';buf[p++]='1';buf[p++]=':'; buf[p]=0;
        itoa_simple(e1, buf + p);
        draw_val(0, FH*2, buf, WHITE, BLACK);

        p = 0; buf[p++]='E';buf[p++]='2';buf[p++]=':'; buf[p]=0;
        itoa_simple(e2, buf + p);
        draw_val(0, FH*3, buf, WHITE, BLACK);

        p = 0; buf[p++]='S';buf[p++]='1';buf[p++]=':'; buf[p]=0;
        itoa_simple(s1, buf + p);
        draw_val(0, FH*5, buf, YELLOW, BLACK);

        p = 0; buf[p++]='S';buf[p++]='2';buf[p++]=':'; buf[p]=0;
        itoa_simple(s2, buf + p);
        draw_val(0, FH*6, buf, YELLOW, BLACK);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* ── Motor Task ── */
static void prvMotorTask(void *pvParameters)
{
    (void)pvParameters;
    BSP_UART_tx_str("[MOTOR] started\r\n");
    vTaskDelay(500);

    for (;;) {
        BSP_UART_tx_str("FWD\r\n");
        Motor_set(250, 250);
        vTaskDelay(pdMS_TO_TICKS(1500));

        BSP_UART_tx_str("STOP\r\n");
        Motor_set(0, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));

        BSP_UART_tx_str("REV\r\n");
        Motor_set(-250, -250);
        vTaskDelay(pdMS_TO_TICKS(1500));

        BSP_UART_tx_str("STOP\r\n");
        Motor_set(0, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ── main ── */
int main(void)
{
    SYSCFG_DL_init();
    BSP_delay_ms(100);

    BSP_UART_tx_str("\r\n=== Test 9: Motor+Enc+LCD ===\r\n");

    Motor_init();
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
    NVIC_EnableIRQ(TIMER_10ms_INST_INT_IRQN);
    DL_TimerA_startCounter(TIMER_10ms_INST);
    NVIC_DisableIRQ(UART_0_INST_INT_IRQN);

    ST7789_init(ST7789_HORIZONTAL);
    ST7789_backLight(1);
    HAL_SPI_init();

    BSP_UART_tx_str("Init OK\r\n");

    if (xTaskCreate(prvLcdTask,   "LCD",   1024+256, NULL, 1, NULL) != pdPASS ||
        xTaskCreate(prvMotorTask, "MOTOR", 512, NULL, 2, NULL) != pdPASS) {
        BSP_UART_tx_str("FATAL\r\n"); for (;;) {}
    }

    BSP_UART_tx_str("FreeRTOS starting...\r\n");
    vTaskStartScheduler();
    for (;;) {}
}
