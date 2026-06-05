/*
 * ============ task_lcd.c =============
 * LCD 菜单渲染任务 (20 Hz, idle+4)
 *   消费 button_queue → menu_process_event()
 *   持有 SPI1 锁 → menu_render()
 *   menu 触发竞赛 → 发送 cmd_queue
 */

#include "task_button.h"
#include "app_menu.h"
#include "app_config.h"

#include "hal_spi.h"
#include "gui_paint.h"    /* BLACK, ... */
#include "hw_st7789.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

/* ══════════ 全局 Kalman 状态 (volatile, Sensor 任务写入) ══════════ */

/* 前向声明 — 定义在 task_sensor.c */
extern volatile float g_kf_x, g_kf_y, g_kf_theta;
extern volatile float g_kf_v, g_kf_omega;
extern volatile sensor_data_t g_sensor_data;

/* ══════════ LCD 任务 ══════════ */

void vTaskLcd(void *pvParameters)
{
    (void)pvParameters;

    menu_state_t menu;
    menu_init(&menu);

    /* 保留 splash, 等第一帧传感器数据到达后再渲染 STATUS */

    TickType_t xLastWakeTime = xTaskGetTickCount();

    /* 首次渲染: 显示启动提示 */
    {
        extern const uint8_t Font8_Table[];
        HAL_SPI_lock();
        ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
        ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);
        ST7789_drawStringFast(5, 50,  "MSPM0G3507 Car",  Font8_Table, 6, 10, GREEN, BLACK);
        ST7789_drawStringFast(5, 70,  "Sensor starting", Font8_Table, 6, 10, CYAN, BLACK);
        ST7789_drawStringFast(5, 100, "UART: 1-6=Motors", Font8_Table, 6, 10, WHITE, BLACK);
        ST7789_drawStringFast(5, 120, "CENTER=Menu",     Font8_Table, 6, 10, WHITE, BLACK);
        HAL_SPI_unlock();
        /* LCD 启动完成 */
    }
    sensor_data_t local_sensor;
    bool sensor_valid = false;
    uint16_t idle_cnt = 0;

    for (;;) {
        /* ── 1. 消费所有按键事件 ── */
        {
            button_event_t evt;
            while (xQueueReceive(g_button_queue, &evt, 0) == pdPASS) {
                menu_process_event(&menu, &evt);
                idle_cnt = 0;
            }
        }

        /* ── 2. 检查 auto-return ── */
        /* TODO: auto-return 暂时禁用, 排查计数器异常 */
        (void)idle_cnt;
#if 0
        if (menu.screen != SCREEN_STATUS) {
            idle_cnt++;
            if (idle_cnt > (MENU_TIMEOUT_MS / PERIOD_LCD_MS)) {
                extern void BSP_UART_tx_str(const char*);
                BSP_UART_tx_str("[MENU] auto-return\r\n");
                idle_cnt = 0;
                menu.screen = SCREEN_STATUS;
                menu.needs_full_redraw = true;
            }
        } else {
            idle_cnt = 0;
        }
#endif

        /* ── 3. 获取最新传感器数据 ── */
        {
            if (xQueueReceive(g_sensor_queue, &local_sensor, 0) == pdPASS) {
                sensor_valid = true;
            }
        }

        /* ── 4. 处理竞赛命令 ── */
        {
            ctrl_cmd_t cmd = menu_get_pending_cmd(&menu);
            if (cmd != CMD_NONE) {
                xQueueSend(g_cmd_queue, &cmd, 0);
                menu_clear_pending_cmd(&menu);
            }
        }

        /* ── 5. 获取 SPI 锁并渲染 ── */
        HAL_SPI_lock();
        {
            /* 更新竞赛屏幕的实时数据 */
            if (menu.screen == SCREEN_CONTEST && sensor_valid) {
                menu.cur_speed   = local_sensor.enc_speed;
                menu.cur_heading = local_sensor.total_yaw;
            }

            menu_render(&menu,
                        sensor_valid ? &local_sensor : NULL,
                        g_kf_x, g_kf_y, g_kf_theta);
        }
        HAL_SPI_unlock();

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(PERIOD_LCD_MS));
    }
}
