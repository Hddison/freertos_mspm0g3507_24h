/*
 *  ============ task_lcd.c =============
 *  LCD 显示任务实现
 *
 *  这是一个消费者任务的标准模板:
 *    1. 外设初始化 (LCD, Paint)
 *    2. 绘制静态 UI 元素 (一次)
 *    3. for(;;) 循环:
 *       a. 阻塞等待队列数据 (xQueueReceive, 有超时)
 *       b. DMA 刷新动态数值区域
 *       c. 无需 vTaskDelay (由队列阻塞控制节奏)
 */

#include "task_lcd.h"
#include "task_sensor.h"

#include <FreeRTOS.h>
#include <task.h>

#include "app/app_config.h"
#include "app/app_ui.h"
#include "bsp_uart.h"
#include "hw_st7789.h"
#include "gui_paint.h"

/* ── 布局常量 ── */
#define VAL_X    80
#define VAL_W    85
#define ROLL_Y   38
#define PITCH_Y  68
#define YAW_Y    98
#define TOTAL_X  2
#define TOTAL_Y  150
#define TOTAL_W  166

#define ENC_X    24
#define ENC_Y    258

#define GS_X     19
#define GS_Y     282
#define GS_W     10
#define GS_H     14
#define GS_GAP   1
#define GS_STEP  (GS_W + GS_GAP)

/* ══════ 绘制静态 UI (仅一次) ══════ */
static void prvDrawStaticUI(bool jy61p_ok)
{
    Paint_Clear(BLACK);

    /* 标题栏 */
    Paint_DrawString_EN(2, 5, "JY61P IMU", &Font16, BLACK, CYAN);
    Paint_DrawLine(2, 26, 167, 26, LGRAY, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    /* 角度标签 */
    Paint_DrawString_EN(2, ROLL_Y,  "R:", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(2, PITCH_Y, "P:", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(2, YAW_Y,   "Y:", &Font20, BLACK, WHITE);

    /* Total Yaw 分隔线 + 标签 */
    Paint_DrawLine(2, 135, 167, 135, LGRAY, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawString_EN(2, 148, "Total:", &Font12, BLACK, LGRAY);

    /* 编码器区域 */
    Paint_DrawLine(2, 255, 167, 255, LGRAY, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawString_EN(2, 258, "Enc:", &Font8, BLACK, LGRAY);

    /* 灰度传感器区域 */
    Paint_DrawLine(2, 275, 167, 275, LGRAY, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawString_EN(2, 278, "GS:", &Font8, BLACK, LGRAY);

    /* 底部状态 */
    Paint_DrawLine(2, 305, 167, 305, LGRAY, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawString_EN(2,  310, "MSPM0G3507", &Font8, BLACK, LGRAY);
    Paint_DrawString_EN(80, 310, jy61p_ok  ? "IMU:OK" : "IMU:FAIL", &Font8, BLACK,
                        jy61p_ok ? GREEN : RED);
}

/* ══════ 任务函数 ══════ */
static void prvLcdTask(void *pvParameters)
{
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    sensor_data_t data;
    bool          uiReady = false;

    BSP_UART_tx_str("[LCD] Started\r\n");

    /* ── LCD 初始化 ── */
    vTaskDelay(pdMS_TO_TICKS(200));
    ST7789_init(ST7789_HORIZONTAL);
    ST7789_backLight(1);
    Paint_NewImage(ST7789_WIDTH, ST7789_HEIGHT, ROTATE_0, BLACK);
    Paint_SetClearFuntion(ST7789_clear);
    Paint_SetDisplayFuntion(ST7789_drawPoint);

    BSP_UART_tx_str("[LCD] Hardware ready\r\n");

    /* ══════ 主循环 ══════ */
    for (;;) {
        /*
         * 阻塞等待传感器数据 (带超时, 防止卡死)
         * 超时时间 = 传感器周期 * 3 (允许丢 2 帧)
         */
        if (xQueueReceive(queue, &data,
                pdMS_TO_TICKS(TASK_SENSOR_PERIOD_MS * 3)) != pdTRUE) {
            /* 超时: 继续等待, 不渲染 */
            continue;
        }

        /* ── 首次收到数据时绘制静态 UI ── */
        if (!uiReady) {
            prvDrawStaticUI(data.jy61p_ok);
            uiReady = true;
        }

        /* ── 动态数值区域 ── */
        if (data.jy61p_ok) {
            /* Roll / Pitch / Yaw (Font20: 14×20) */
            app_draw_val(VAL_X, ROLL_Y,  data.roll,  2, Font20.table, 14, 20, GREEN);
            app_draw_val(VAL_X, PITCH_Y, data.pitch, 2, Font20.table, 14, 20, GREEN);
            app_draw_val(VAL_X, YAW_Y,   data.yaw,   2, Font20.table, 14, 20, GREEN);

            /* Total Yaw (Font24: 17×24) */
            char tbuf[12];
            int pos = app_ftoa(tbuf, data.total_yaw, 1);
            uint16_t tw = (uint16_t)pos * 17;
            ST7789_setWindows(TOTAL_X, TOTAL_Y, TOTAL_X + tw - 1, TOTAL_Y + 23);
            ST7789_clearRawDMA(BLACK, tw, 24);
            ST7789_drawStringFast(TOTAL_X, TOTAL_Y, tbuf, Font24.table, 17, 24,
                                  YELLOW, BLACK);
        }

        /* ── 编码器距离 ── */
        {
            char ebuf[32]; int p = 0;
            ebuf[p++]='M'; ebuf[p++]='1'; ebuf[p++]=':';
            p += app_ftoa(ebuf + p, data.enc1_dist, 1); ebuf[p++]=' ';
            ebuf[p++]='M'; ebuf[p++]='2'; ebuf[p++]=':';
            p += app_ftoa(ebuf + p, data.enc2_dist, 1);
            ebuf[p]='\0';

            ST7789_setWindows(ENC_X, ENC_Y, ENC_X + 119, ENC_Y + 7);
            ST7789_clearRawDMA(BLACK, 120, 8);
            ST7789_drawStringFast(ENC_X, ENC_Y, ebuf, Font8.table, 5, 8, CYAN, BLACK);
        }

        /* ── 灰度传感器: 12 方块 ── */
        if (data.nchd12_ok) {
            for (int i = 0; i < 12; i++) {
                uint16_t sx = GS_X + (uint16_t)i * GS_STEP;
                /* bit(11-i): 左→右对应通道 1→12 */
                uint16_t color = (data.grayscale & (1 << (11 - i))) ? WHITE : BLACK;
                ST7789_setWindows(sx, GS_Y, sx + GS_W - 1, GS_Y + GS_H - 1);
                ST7789_clearRaw(color, GS_W, GS_H);
            }
        }
    }
}

/* ── 用于 Flash 操作时暂停 LCD ── */
static TaskHandle_t g_lcdHandle;

/* ══════ 创建任务 ══════ */
void TaskLcd_create(QueueHandle_t sensorQueue)
{
    if (!sensorQueue) {
        BSP_UART_tx_str("[LCD] Invalid queue handle!\r\n");
        return;
    }

    BaseType_t ret = xTaskCreate(
        prvLcdTask,
        "LCD",
        TASK_LCD_STACK_SIZE,
        (void *)sensorQueue,
        TASK_LCD_PRIO,
        &g_lcdHandle
    );
    if (ret != pdPASS) {
        BSP_UART_tx_str("[LCD] Failed to create task!\r\n");
    }
}

void TaskLcd_suspend(void)
{
    if (g_lcdHandle) {
        vTaskSuspend(g_lcdHandle);
        vTaskDelay(pdMS_TO_TICKS(5));  /* 等 DMA 完成 */
    }
}

void TaskLcd_resume(void)
{
    if (g_lcdHandle) vTaskResume(g_lcdHandle);
}
