/*
 * ============ task_button.c =============
 * 按键扫描任务 (50 Hz, idle+3)
 *   调用 BSP_Button_Scan() → 按键重映射 → 发送 button_queue
 */

#include "task_button.h"
#include "bsp_button.h"
#include "app_config.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

/* ── 全局: 按键重映射 (默认 1:1, Flash 加载后覆盖) ── */
uint8_t g_btn_remap[6] = {
    BTN_DIR_UP,    /* BTN_ID_UP     → UP    */
    BTN_DIR_LEFT,  /* BTN_ID_LEFT   → LEFT  */
    BTN_DIR_DOWN,  /* BTN_ID_DOWN   → DOWN  */
    BTN_DIR_RIGHT, /* BTN_ID_RIGHT  → RIGHT */
    BTN_DIR_ENTER, /* BTN_ID_CENTER → ENTER */
    BTN_DIR_BACK   /* BTN_ID_BUTTON → BACK  */
};

/* ── 队列句柄 ── */
void *g_button_queue;
void *g_cmd_queue;
void *g_sensor_queue;

/* ══════════ 按键扫描任务 ══════════ */

void vTaskButton(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        /* 扫描按键 (20ms 去抖周期) */
        uint8_t evt = BSP_Button_Scan();

        if (evt != BTN_EVT_NONE) {
            uint8_t phys = BSP_Button_ID();

            button_event_t btn_evt = {
                .physical   = phys,
                .logical    = g_btn_remap[phys],  /* 应用重映射 */
                .event_type = evt,
                .tick       = xTaskGetTickCount()
            };

            /* 非阻塞发送 (队列满则丢弃) */
            xQueueSend(g_button_queue, &btn_evt, 0);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(PERIOD_BUTTON_MS));
    }
}
