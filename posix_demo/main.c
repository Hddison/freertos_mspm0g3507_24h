/*
 * main.c — Test 7: IMU 100Hz DMA + LCD display (FreeRTOS)
 * 自带轻量 sprintf 替代 (无标准库依赖)
 */
#include <FreeRTOS.h>
#include <task.h>
#include "ti_msp_dl_config.h"
#include "bsp_uart.h"
#include "bsp_system.h"
#include "bsp_i2c.h"
#include "hal_spi.h"
#include "hw_jy61p.h"
#include "hw_st7789.h"
#include "gui_paint.h"

#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define MAGENTA 0xF81F

extern const uint8_t Font8_Table[];

#define FW 5   /* Font8 width  */
#define FH 10  /* Font8 height + spacing */

/* ── 轻量 sprintf 替代 ── */

/* itoa: 整数转字符串, 返回长度 */
static int itoa_simple(int val, char *buf)
{
    int len = 0;
    if (val < 0) { buf[len++] = '-'; val = -val; }
    if (val == 0) { buf[len++] = '0'; buf[len] = 0; return len; }
    int start = len;
    for (int t = val; t; t /= 10) len++;
    buf[len] = 0;
    for (int i = len - 1; i >= start; i--) {
        buf[i] = '0' + (val % 10);
        val /= 10;
    }
    return len;
}

/* ftoa: 浮点数转字符串, 保留 dec 位小数, 返回长度 */
static int ftoa_simple(float val, int dec, char *buf)
{
    int len = 0;
    if (val < 0) { buf[len++] = '-'; val = -val; }
    /* 乘以 10^dec 取整 */
    float scale = 1.0f;
    for (int i = 0; i < dec; i++) scale *= 10.0f;
    int ipart = (int)val;
    int fpart = (int)((val - (float)ipart) * scale + 0.5f);
    /* 处理四舍五入进位 */
    if (fpart >= (int)scale) { ipart++; fpart = 0; }

    len += itoa_simple(ipart, buf + len);
    if (dec > 0) {
        buf[len++] = '.';
        int div = 1;
        for (int i = 1; i < dec; i++) div *= 10;
        for (int i = 0; i < dec; i++) {
            buf[len++] = '0' + ((fpart / div) % 10);
            div /= 10;
        }
    }
    buf[len] = 0;
    return len;
}

/* ── Stack overflow ── */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    BSP_UART_tx_str("STACK: "); BSP_UART_tx_str(pcTaskName);
    for (;;) {}
}

/* ── 共享数据 ── */
static JY61P_RawAngle g_ra;
static JY61P_RawIMU   g_ri;
static volatile bool  g_data_ready = false;

/* ── IMU Task: 100Hz DMA ── */
static void prvImuTask(void *pvParameters)
{
    (void)pvParameters;
    BSP_UART_tx_str("[IMU] started\r\n");
    uint32_t sample = 0;
    for (;;) {
        JY61P_RawAngle ra;
        JY61P_RawIMU   ri;
        if (JY61P_readAngle(&ra) && JY61P_readIMU_dma(&ri)) {
            g_ra = ra;
            g_ri = ri;
            g_data_ready = true;
        }
        sample++;
        if (sample % 50 == 0) {
            JY61P_Angle a; JY61P_convAngle(&g_ra, &a);
            char buf[80]; int p = 0;
            p += itoa_simple((int)sample, buf + p);
            buf[p++] = ' '; buf[p] = 0;
            BSP_UART_tx_str(buf);
            BSP_UART_tx_str(" samples\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* ── 画一行数字: 擦全宽再写 (防重叠) ── */
static void draw_val(uint16_t x, uint16_t y, const char *val,
                     uint16_t fg, uint16_t bg)
{
    ST7789_setWindows(0, y, ST7789_WIDTH - 1, y + FH - 1);
    ST7789_clearRawDMA(bg, ST7789_WIDTH, FH);
    ST7789_drawStringFast(x, y, val, Font8_Table, FW, 8, fg, bg);
}

/* ── LCD Task ── */
static void prvLcdTask(void *pvParameters)
{
    (void)pvParameters;
    while (!g_data_ready) vTaskDelay(10);

    /* 首帧 + 静态标签 */
    ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);
    ST7789_drawStringFast(0, 0,  "IMU",   Font8_Table, FW, 8, GREEN,   BLACK);
    ST7789_drawStringFast(0, FH*7, "GYRO",  Font8_Table, FW, 8, CYAN,   BLACK);
    ST7789_drawStringFast(0, FH*13,"ACCEL", Font8_Table, FW, 8, MAGENTA, BLACK);
    BSP_UART_tx_str("[LCD] started\r\n");

    for (;;) {
        JY61P_RawAngle ra = g_ra;
        JY61P_RawIMU   ri = g_ri;
        JY61P_Angle a; JY61P_convAngle(&ra, &a);

        char line[16];

        /* Angle: R/P/Y */
        ftoa_simple(a.roll,  1, line); draw_val(0, FH*1, line, WHITE,  BLACK);
        ftoa_simple(a.pitch, 1, line); draw_val(0, FH*2, line, WHITE,  BLACK);
        ftoa_simple(a.yaw,   1, line); draw_val(0, FH*3, line, YELLOW, BLACK);

        /* Gyro: Gx/Gy/Gz */
        ftoa_simple((float)(ri.gx * JY61P_GYRO_SCALE), 0, line);
        draw_val(0, FH*8,  line, CYAN, BLACK);
        ftoa_simple((float)(ri.gy * JY61P_GYRO_SCALE), 0, line);
        draw_val(0, FH*9,  line, CYAN, BLACK);
        ftoa_simple((float)(ri.gz * JY61P_GYRO_SCALE), 0, line);
        draw_val(0, FH*10, line, CYAN, BLACK);

        /* Accel: Ax/Ay/Az */
        ftoa_simple((float)(ri.ax * JY61P_ACC_SCALE), 1, line);
        draw_val(0, FH*14, line, MAGENTA, BLACK);
        ftoa_simple((float)(ri.ay * JY61P_ACC_SCALE), 1, line);
        draw_val(0, FH*15, line, MAGENTA, BLACK);
        ftoa_simple((float)(ri.az * JY61P_ACC_SCALE), 1, line);
        draw_val(0, FH*16, line, MAGENTA, BLACK);

        vTaskDelay(pdMS_TO_TICKS(33));  /* ~30Hz */
    }
}

/* ── main ── */
int main(void)
{
    SYSCFG_DL_init();
    BSP_I2C_init();
    BSP_delay_ms(200);

    BSP_UART_tx_str("\r\n=== IMU + LCD (RTOS) ===\r\n");

    bool ok = false;
    for (int i = 0; i < 10; i++) {
        if (JY61P_init()) { ok = true; break; }
        BSP_UART_tx_str("retry...\r\n"); BSP_delay_ms(500);
    }
    if (!ok) { BSP_UART_tx_str("IMU FAIL\r\n"); for (;;) {} }

    ST7789_init(ST7789_HORIZONTAL);
    ST7789_backLight(1);
    ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);
    BSP_UART_tx_str("Init OK\r\n");

    HAL_SPI_init();
    NVIC_DisableIRQ(UART_0_INST_INT_IRQN);
    NVIC_DisableIRQ(GPIOA_INT_IRQn);

    if (xTaskCreate(prvImuTask, "IMU", 512, NULL, 3, NULL) != pdPASS ||
        xTaskCreate(prvLcdTask, "LCD", 1024, NULL, 1, NULL) != pdPASS) {
        BSP_UART_tx_str("FATAL\r\n"); for (;;) {}
    }

    vTaskStartScheduler();
    for (;;) {}
}
