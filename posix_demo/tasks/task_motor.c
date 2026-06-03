/*
 *  ============ task_motor.c =============
 *  电机控制任务 — 偏航角恢复 P 控制器
 *
 *  状态机: IDLE → ROTATING → DONE → IDLE
 *  - IDLE:     等待 Task Notification (目标 yaw)
 *  - ROTATING: P 控制差速旋转
 *  - DONE:     停止, 回到 IDLE
 */

#include "task_motor.h"

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "app/app_config.h"
#include "bsp_uart.h"
#include "hw_motor.h"

/* ── 当前偏航角 (传感器任务每 20Hz 更新) ── */
extern volatile float g_current_yaw;

/* ── P 控制器参数 ── */
#define YAW_KP          8.0f      /* 比例增益 */
#define YAW_DEAD_ZONE   2.0f      /* 死区 (°) */
#define YAW_DONE_CNT    5         /* 连续稳定次数 → 完成 */

/* ── 状态 ── */
typedef enum { ST_IDLE, ST_ROTATING, ST_DONE } motor_state_t;

static TaskHandle_t g_motorHandle;

/* 角度差归一化到 [-180, 180] */
static float _normAngle(float err)
{
    while (err >  180.0f) err -= 360.0f;
    while (err < -180.0f) err += 360.0f;
    return err;
}

/* ══════ 任务函数 ══════ */
static void prvMotorTask(void *pvParameters)
{
    (void)pvParameters;
    float    target = 0.0f;
    uint32_t notifyVal;
    motor_state_t state = ST_IDLE;
    uint8_t  doneCnt = 0;

    BSP_UART_tx_str("[Motor] Ready\r\n");

    for (;;) {
        switch (state) {

        case ST_IDLE:
            Motor_set(0, 0);
            /* 等待通知 (目标 yaw 的 float 位模式) */
            if (xTaskNotifyWait(0, 0xFFFFFFFF, &notifyVal, portMAX_DELAY) == pdTRUE) {
                memcpy(&target, &notifyVal, sizeof(float));
                char m[48];
                snprintf(m, sizeof(m), "[Motor] Yaw restore start: tgt=%.1f\r\n",
                         (double)target);
                BSP_UART_tx_str(m);
                state = ST_ROTATING;
            }
            break;

        case ST_ROTATING: {
            float cur   = g_current_yaw;
            float error = _normAngle(target - cur);
            float pwm   = YAW_KP * error;

            /* 死区判断 */
            if (fabsf(error) < YAW_DEAD_ZONE) {
                doneCnt++;
                if (doneCnt >= YAW_DONE_CNT) {
                    Motor_set(0, 0);
                    BSP_UART_tx_str("[Motor] Yaw restore done\r\n");
                    state = ST_IDLE;
                    doneCnt = 0;
                    break;
                }
                Motor_set(0, 0);
            } else {
                doneCnt = 0;
                /* 钳位 */
                if (pwm >  MOTOR_PWM_MAX) pwm =  MOTOR_PWM_MAX;
                if (pwm < -MOTOR_PWM_MAX) pwm = -MOTOR_PWM_MAX;
                /* 差速旋转: +pwm=A, -pwm=B */
                Motor_set((int16_t)pwm, (int16_t)(-pwm));
            }
            break;
        }

        case ST_DONE:
            Motor_set(0, 0);
            state = ST_IDLE;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_MOTOR_PERIOD_MS));
    }
}

/* ══════ 创建任务 ══════ */
void TaskMotor_create(void)
{
    BaseType_t ret = xTaskCreate(
        prvMotorTask, "Motor", TASK_MOTOR_STACK_SIZE, NULL,
        TASK_MOTOR_PRIO, &g_motorHandle
    );
    if (ret != pdPASS) {
        BSP_UART_tx_str("[Motor] Failed to create task!\r\n");
    }
}

/* ══════ 启动偏航角恢复 ══════ */
void TaskMotor_startYawRestore(float target_yaw)
{
    if (!g_motorHandle) return;
    uint32_t val;
    memcpy(&val, &target_yaw, sizeof(float));
    xTaskNotify(g_motorHandle, val, eSetValueWithOverwrite);
    BSP_UART_tx_str("[Motor] Restore cmd sent\r\n");
}
