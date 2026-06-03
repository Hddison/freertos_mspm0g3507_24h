/*
 * FreeRTOS MSPM0G3507 — JY61P 角度 LCD 显示 + LED 闪烁
 *
 * SPI DMA 快速刷新: 数值直接绘制到窗口, 零 Paint 逐像素开销
 */

#include <FreeRTOS.h>
#include <task.h>

#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>

#include "bsp_uart.h"
#include "bsp_system.h"
#include "hw_st7789.h"
#include "gui_paint.h"
#include "hw_jy61p.h"
#include "hw_nchd12.h"
#include "hw_motor.h"
#include "bsp_button.h"

/*
 * 布局 (170×320 竖屏)
 *   ┌──────────────┐
 *   │ JY61P IMU    │ y=5   Font16
 *   │──────────────│ y=26
 *   │ R: -12.34    │ y=38  Font20
 *   │ P:  5.67     │ y=68  Font20
 *   │ Y:  89.01    │ y=98  Font20
 *   │──────────────│ y=135
 *   │ Total: 720°  │ y=148  Font12+Font24
 *   │──────────────│ y=305
 *   │ MSPM0G3507   │ y=310  Font8
 *   └──────────────┘
 */

/* ── 数值区域 ── */
#define VAL_X  80
#define VAL_W  85
#define ROLL_Y  38
#define PITCH_Y 68
#define YAW_Y   98
#define TOTAL_X   2
#define TOTAL_Y   150
#define TOTAL_W  166

/* ════════════ LED 任务 ════════════ */
static void vLedTask(void *pv)
{
    
    (void)pv;
    BSP_UART_tx_str("\r\n=== LED Task Started ===\r\n");

    for (;;) {
        BSP_UART_tx_str("\r\n=== LED Toggle ===\r\n");
        DL_GPIO_togglePins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ════════════ 共享: float → string (1-2 位小数) ════════════ */
static int ftoa(char *buf, float val, uint8_t dec)
{
    int pos = 0;
    if (val < 0.0f) { buf[pos++] = '-'; val = -val; }
    int ip = (int)val;
    if (ip == 0) { buf[pos++] = '0'; }
    else {
        char t[8]; int tp = 0;
        while (ip) { t[tp++] = '0' + (ip % 10); ip /= 10; }
        while (tp) buf[pos++] = t[--tp];
    }
    buf[pos++] = '.';
    int fd = (int)((val - (int)val) * (dec == 2 ? 100.0f : 10.0f) + 0.5f);
    int limit = (dec == 2 ? 100 : 10);
    if (fd >= limit) fd = limit - 1;
    if (dec == 2) { buf[pos++] = '0' + (fd/10); buf[pos++] = '0' + (fd%10); }
    else          { buf[pos++] = '0' + fd; }
    buf[pos] = '\0';
    return pos;
}

/* ════════════ 数值更新: 擦 + 写 ════════════ */
static void draw_val(uint16_t x, uint16_t y, float val, uint8_t dec,
                     const uint8_t *tbl, uint16_t fw, uint16_t fh,
                     uint16_t fg)
{
    char buf[12];
    uint16_t w = (uint16_t)ftoa(buf, val, dec) * fw;
    ST7789_setWindows(x, y, x + w - 1, y + fh - 1);
    ST7789_clearRawDMA(BLACK, w, fh);
    ST7789_drawStringFast(x, y, buf, tbl, fw, fh, fg, BLACK);
}

/* ════════════ LCD 任务 ════════════ */
static void vLcdTask(void *pv)
{
    (void)pv;
    BSP_UART_tx_str("\r\n=== LCD Task Started ===\r\n");
    /* ── 上电等待外设稳定 ── */
    vTaskDelay(pdMS_TO_TICKS(200));

    /* ── JY61P 初始化 (最多 10 次) ── */
    bool jy61p_ok = false;
    vTaskDelay(pdMS_TO_TICKS(500));
    for (int i = 0; i < 10; i++) {
        BSP_UART_tx_str("\r\n=== JY61P Initialization Attempt ===\r\n");
        if (JY61P_init()) { jy61p_ok = true; break; }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* ── NCHD12 初始化 (最多 3 次) ── */
    bool nchd12_ok = false;
    {
        uint16_t dummy;
        vTaskDelay(pdMS_TO_TICKS(100));
        for (int i = 0; i < 3; i++) {
            BSP_UART_tx_str("\r\n=== NCHD12 Initialization Attempt ===\r\n");
            if (NCHD12_read(&dummy)) { nchd12_ok = true; break; }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    /* ── 静态元素 (Paint, 仅一次) ── */
    Paint_Clear(BLACK);
    Paint_DrawString_EN(2, 5, "JY61P IMU", &Font16, BLACK, CYAN);
    Paint_DrawLine(2, 26, 167, 26, LGRAY, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    Paint_DrawString_EN(2, ROLL_Y,  "R:", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(2, PITCH_Y, "P:", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(2, YAW_Y,   "Y:", &Font20, BLACK, WHITE);

    Paint_DrawLine(2, 135, 167, 135, LGRAY, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawString_EN(2, 148, "Total:", &Font12, BLACK, LGRAY);

    /* ── 编码器 + 灰度传感器区域 ── */
    Paint_DrawLine(2, 255, 167, 255, LGRAY, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawString_EN(2, 258, "Enc:", &Font8, BLACK, LGRAY);

    Paint_DrawLine(2, 275, 167, 275, LGRAY, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawString_EN(2, 278, "GS:", &Font8, BLACK, LGRAY);

    Paint_DrawLine(2, 305, 167, 305, LGRAY, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    /* 底部状态 */
    Paint_DrawString_EN(2,  310, "MSPM0G3507", &Font8, BLACK, LGRAY);
    Paint_DrawString_EN(80, 310, jy61p_ok  ? "IMU:OK" : "IMU:FAIL", &Font8, BLACK,
                        jy61p_ok ? GREEN : RED);

    /* ── 电机初始化 ── */
    Motor_init();

    JY61P_RawAngle raw;
    JY61P_Angle    ang;

    /* 电机停转, 手动测试编码器 */
    Motor_set(0, 0);

    for (;;) {
        if (jy61p_ok && JY61P_readAngle(&raw)) {
            JY61P_convAngle(&raw, &ang);
            JY61P_updateTotalYaw(raw.yaw);

            /* DMA 擦 + 绘: Roll / Pitch / Yaw */
            draw_val(VAL_X, ROLL_Y,  ang.roll,  2, Font20.table, 14, 20, GREEN);
            draw_val(VAL_X, PITCH_Y, ang.pitch, 2, Font20.table, 14, 20, GREEN);
            draw_val(VAL_X, YAW_Y,   ang.yaw,   2, Font20.table, 14, 20, GREEN);

            /* Total Yaw (Font24: 17×24) */
            char tbuf[12];
            int pos = ftoa(tbuf, JY61P_getTotalYaw(), 1);
            uint16_t tw = (uint16_t)pos * 17;   /* Font24 宽度 17 */
            ST7789_setWindows(TOTAL_X, TOTAL_Y, TOTAL_X + tw - 1, TOTAL_Y + 23);
            ST7789_clearRawDMA(BLACK, tw, 24);
            ST7789_drawStringFast(TOTAL_X, TOTAL_Y, tbuf, Font24.table, 17, 24,
                                  YELLOW, BLACK);
        }

        /* ── 编码器距离 (mm) ── */
        {
            char ebuf[32]; int p = 0;
            ebuf[p++]='M'; ebuf[p++]='1'; ebuf[p++]=':';
            p += ftoa(ebuf + p, Motor_enc1Dist(), 1); ebuf[p++]=' ';
            ebuf[p++]='M'; ebuf[p++]='2'; ebuf[p++]=':';
            p += ftoa(ebuf + p, Motor_enc2Dist(), 1);
            ebuf[p]='\0';

            ST7789_setWindows(24, 258, 24 + 119, 265);
            ST7789_clearRawDMA(BLACK, 120, 8);
            ST7789_drawStringFast(24, 258, ebuf, Font8.table, 5, 8, CYAN, BLACK);
        }

        /* ── 灰度传感器: 12 方块 ── */
        if (nchd12_ok) {
            uint16_t gs;
            if (NCHD12_read(&gs)) {
                #define GS_X    19
                #define GS_Y    282
                #define GS_W    10
                #define GS_H    14
                #define GS_GAP  1
                #define GS_STEP (GS_W + GS_GAP)

                for (int i = 0; i < 12; i++) {
                    uint16_t sx = GS_X + (uint16_t)i * GS_STEP;
                    /* bit(11-i): 左→右对应通道 1→12 */
                    uint16_t color = (gs & (1 << (11 - i))) ? WHITE : BLACK;
                    ST7789_setWindows(sx, GS_Y, sx + GS_W - 1, GS_Y + GS_H - 1);
                    ST7789_clearRaw(color, GS_W, GS_H);
                }
            }
        }

        /* ── 按键扫描 ── */
        {
            uint8_t evt = BSP_Button_Scan();
            if (evt == BTN_EVT_SHORT) {
                Motor_encReset();       /* 短按: 编码器清零 */
            } else if (evt == BTN_EVT_LONG) {
                                       /* 长按: 暂不处理 */
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));   /* 20Hz */
    }
}

/* ════════════ main ════════════ */
int main(void)
{
    SYSCFG_DL_init();
    BSP_UART_tx_str("\r\n=== SYSCFG_DL_init ===\r\n");
    /* 禁用 UART 中断, 防止上电噪声触发 Default_Handler */
    DL_UART_Main_disableInterrupt(UART_0_INST, DL_UART_MAIN_INTERRUPT_RX);

    BSP_delay_ms(1000);  /* 硬件稳定 */

    ST7789_init(ST7789_HORIZONTAL);
    ST7789_backLight(1);
    BSP_UART_tx_str("\r\n=== LCD Initialized ===\r\n");
    Paint_NewImage(ST7789_WIDTH, ST7789_HEIGHT, ROTATE_0, BLACK);
    Paint_SetClearFuntion(ST7789_clear);
    Paint_SetDisplayFuntion(ST7789_drawPoint);
    BSP_UART_tx_str("\r\n=== Paint Initialized ===\r\n");
    xTaskCreate(vLedTask,  "LED", 128,  NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(vLcdTask,  "LCD", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
    BSP_UART_tx_str("\r\n=== Tasks Created ===\r\n");
    

    vTaskStartScheduler();
    
    for (;;) {}
}

/* ════════════ 中断桩: 防止上电瞬态触发 Default_Handler ════════════ */

void UART0_IRQHandler(void)
{
    /* 清除 UART 中断标志, 防止噪声触发后卡死 */
    DL_UART_Main_clearInterruptStatus(UART_0_INST,
        DL_UART_MAIN_INTERRUPT_RX);
}

void DMA_IRQHandler(void)
{
    /* 清除所有 DMA 通道中断标志 */
    for (uint8_t ch = 0; ch < 4; ch++) {
        DL_DMA_clearInterruptStatus(DMA, ch);
    }
}

void I2C0_IRQHandler(void) { }
void I2C1_IRQHandler(void) { }
void SPI1_IRQHandler(void) { }

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pxTask;
    (void)pcTaskName;
    for (;;) {}
}
#endif
