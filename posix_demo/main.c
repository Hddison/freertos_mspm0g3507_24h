/*
 * main.c — Test 8: NCHD12 Grayscale sensor
 */
#include <FreeRTOS.h>
#include <task.h>
#include "ti_msp_dl_config.h"
#include "bsp_uart.h"
#include "bsp_system.h"
#include "bsp_i2c.h"
#include "hal_spi.h"
#include "hw_nchd12.h"
#include "hw_st7789.h"
#include "gui_paint.h"

#define CYAN    0x07FF

extern const uint8_t Font8_Table[];

#define FW 5
#define FH 10

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    BSP_UART_tx_str("STACK: "); BSP_UART_tx_str(pcTaskName);
    for (;;) {}
}

/* itoa 轻量 */
static int itoa_simple(int val, char *buf)
{
    int len = 0;
    if (val < 0) { buf[len++] = '-'; val = -val; }
    if (val == 0) { buf[len++] = '0'; buf[len] = 0; return len; }
    int start = len;
    for (int t = val; t; t /= 10) len++;
    buf[len] = 0;
    for (int i = len - 1; i >= start; i--) { buf[i] = '0' + (val % 10); val /= 10; }
    return len;
}

/* 12-bit → 二进制字符串 "000000000000" */
static void bits12_str(uint16_t bits, char *out)
{
    for (int i = 0; i < 12; i++) {
        out[i] = (bits & (1 << (11 - i))) ? '1' : '0';
    }
    out[12] = 0;
}

/* ── LCD helper ── */
static void draw_val(uint16_t x, uint16_t y, const char *val, uint16_t fg, uint16_t bg)
{
    ST7789_setWindows(0, y, ST7789_WIDTH - 1, y + FH - 1);
    ST7789_clearRawDMA(bg, ST7789_WIDTH, FH);
    ST7789_drawStringFast(x, y, val, Font8_Table, FW, 8, fg, bg);
}

/* ── 共享数据 ── */
static uint16_t g_bits = 0;
static volatile bool g_ready = false;

/* ── Grayscale Task: 50Hz DMA read ── */
static void prvGrayTask(void *pvParameters)
{
    (void)pvParameters;
    BSP_UART_tx_str("[GRAY] started\r\n");

    uint32_t sample = 0;
    for (;;) {
        uint16_t bits;
        if (NCHD12_read(&bits)) {
            g_bits = bits;
            g_ready = true;
        }

        if (sample % 20 == 0 && g_ready) {
            char buf[48];
            int p = 0;
            p += itoa_simple((int)sample, buf + p);
            buf[p++] = ' '; buf[p] = 0;
            BSP_UART_tx_str(buf);
            BSP_UART_tx_str("samples\r\n");
        }

        sample++;
        vTaskDelay(pdMS_TO_TICKS(10));  /* 50Hz */
    }
}

/* ── LCD Task ── */
static void prvLcdTask(void *pvParameters)
{
    (void)pvParameters;
    while (!g_ready) vTaskDelay(10);

    /* 首帧 */
    ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);
    ST7789_drawStringFast(0, 0, "GRAYSCALE", Font8_Table, FW, 8, CYAN, BLACK);
    BSP_UART_tx_str("[LCD] started\r\n");

    for (;;) {
        uint16_t bits = g_bits;

        /* 12 路 1/0 条状图 */
        for (int i = 0; i < 12; i++) {
            int x = i * 14;
            uint16_t on = (bits & (1 << (11 - i))) ? WHITE : DARKBLUE;
            ST7789_setWindows(x, FH*2, x + 12, FH*2 + 7);
            ST7789_clearRawDMA(on, 13, 8);
            /* 通道号 */
            char ch[2]; ch[0] = (i < 9) ? ('1'+i) : 'A'-9+i; ch[1]=0;
            ST7789_drawStringFast(x+3, FH*2, ch, Font8_Table, FW, 8,
                                  on == WHITE ? BLACK : WHITE, on);
        }

        /* Hex + 二进制 */
        char hex[8], bin[16];
        int p = 0;
        p += itoa_simple(bits >> 8, hex);
        hex[p] = 0;
        bits12_str(bits, bin);

        draw_val(0, FH*4, "Hex:", WHITE, BLACK);
        draw_val(30, FH*4, hex, CYAN, BLACK);
        draw_val(0, FH*5, bin, WHITE, BLACK);

        /* 通路计数 */
        int cnt = 0;
        for (int i = 0; i < 12; i++) if (bits & (1 << (11 - i))) cnt++;
        char cnt_str[8];
        itoa_simple(cnt, cnt_str);
        draw_val(0, FH*6, "Count:", WHITE, BLACK);
        draw_val(50, FH*6, cnt_str, YELLOW, BLACK);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ── main ── */
int main(void)
{
    SYSCFG_DL_init();
    BSP_I2C_init();
    BSP_delay_ms(200);

    BSP_UART_tx_str("\r\n=== Test 8: NCHD12 Grayscale ===\r\n");

    ST7789_init(ST7789_HORIZONTAL);
    ST7789_backLight(1);
    ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);
    BSP_UART_tx_str("Init OK\r\n");

    HAL_SPI_init();
    NVIC_DisableIRQ(UART_0_INST_INT_IRQN);
    NVIC_DisableIRQ(GPIOA_INT_IRQn);

    if (xTaskCreate(prvGrayTask, "GRAY", 256, NULL, 2, NULL) != pdPASS ||
        xTaskCreate(prvLcdTask,  "LCD",  768, NULL, 1, NULL) != pdPASS) {
        BSP_UART_tx_str("FATAL\r\n"); for (;;) {}
    }

    vTaskStartScheduler();
    for (;;) {}
}
