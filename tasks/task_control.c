/*
 * ============ task_control.c =============
 * 控制循环任务 (idle+5)
 *   空闲时: 阻塞在 cmd_queue → 不消耗 CPU
 *   竞赛中: 100 Hz 控制循环 → vTaskDelayUntil
 */

#include "task_button.h"
#include "app_config.h"
#include "app_control.h"
#include "app_kalman.h"
#include "app_slip.h"

#include "hw_motor.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

/* ── 外部 volatile (Sensor 任务写入) ── */
extern volatile float g_kf_x, g_kf_y, g_kf_theta;
extern volatile float g_kf_v, g_kf_omega;
extern volatile sensor_data_t g_sensor_data;

/* ══════════ Control 任务 ══════════ */

void vTaskControl(void *pvParameters)
{
    (void)pvParameters;

    control_init();
    /* 加载 Flash 中保存的 PID 参数 (覆盖 control_init 的默认值) */
    {
        extern flash_config_t g_flash_cfg;
        control_load_from_flash(&g_flash_cfg);
    }

    kalman5_t  local_kf;
    ctrl_cmd_t cmd;

    for (;;) {
        /* ── 空闲/完成时: 阻塞等待竞赛指令, 不消耗 CPU ── */
        if (g_comp.mode == CTRL_IDLE ||
            g_comp.mode == CTRL_COMPLETE ||
            g_comp.mode == CTRL_ESTOP) {

            /* 阻塞直到收到 start 指令 (释放 CPU 给 LCD/Button) */
            while (xQueueReceive(g_cmd_queue, &cmd, portMAX_DELAY) == pdPASS) {
                switch (cmd) {
                case CMD_START_TASK1: control_start_task(1); break;
                case CMD_START_TASK2: control_start_task(2); break;
                case CMD_START_TASK3: control_start_task(3); break;
                case CMD_START_TASK4: control_start_task(4); break;
                default: break;
                }
                /* 收到启动指令 → 跳出, 进入控制循环 */
                if (g_comp.mode != CTRL_IDLE) break;
            }
        }

        /* ── 竞赛运行: 100 Hz 控制循环 ── */
        {
            TickType_t xLastWakeTime = xTaskGetTickCount();

            while (g_comp.mode != CTRL_IDLE &&
                   g_comp.mode != CTRL_COMPLETE &&
                   g_comp.mode != CTRL_ESTOP) {

                /* 检查命令 (非阻塞) — E-STOP 必须立即响应 */
                while (xQueueReceive(g_cmd_queue, &cmd, 0) == pdPASS) {
                    if (cmd == CMD_ESTOP) {
                        control_estop();
                        break;
                    }
                }

                /* 急停后跳出控制循环, 回到阻塞态 */
                if (g_comp.mode == CTRL_ESTOP) break;

                /* 临界区: 复制 volatile Kalman 状态 */
                taskENTER_CRITICAL();
                local_kf.x     = g_kf_x;
                local_kf.y     = g_kf_y;
                local_kf.theta = g_kf_theta;
                local_kf.v     = g_kf_v;
                local_kf.omega = g_kf_omega;
                float line_pos    = g_sensor_data.line_position;
                uint16_t gray_raw = g_sensor_data.grayscale;
                taskEXIT_CRITICAL();

                /* 运行控制循环 */
                control_run(&local_kf, line_pos, gray_raw,
                            g_sensor_data.total_yaw); /* 解卷绕连续航向 (°) */

                /* 竞赛完成 → 回到阻塞态等新指令 */
                if (g_comp.mode == CTRL_COMPLETE) break;

                vTaskDelayUntil(&xLastWakeTime,
                                pdMS_TO_TICKS(PERIOD_CONTROL_MS));
            }
        }
    }
}
