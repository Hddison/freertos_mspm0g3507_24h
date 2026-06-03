/*
 *  ============ task_motor.c =============
 *  电机控制任务实现 (占位)
 *
 *  这是一个控制循环任务的标准模板:
 *    1. 初始化控制参数
 *    2. for(;;) 循环:
 *       a. 读取编码器反馈
 *       b. 执行 PID 计算 (TODO)
 *       c. 更新 PWM 输出
 *       d. vTaskDelay 固定控制周期
 *
 *  当前为占位实现: 电机保持停转, 为后续 PID 开发预留框架
 */

#include "task_motor.h"

#include <FreeRTOS.h>
#include <task.h>

#include "app/app_config.h"
#include "bsp_uart.h"
#include "hw_motor.h"

/* ══════ 任务函数 ══════ */
static void prvMotorTask(void *pvParameters)
{
    (void)pvParameters;
    BSP_UART_tx_str("[Motor] Started (placeholder)\r\n");

    /*
     * TODO: 后续开发项
     *   - 创建命令队列 (速度/位置/模式)
     *   - 实现 PID 控制器 (P / PI / PID)
     *   - 编码器速度计算 (d/dt)
     *   - 里程计融合 (IMU + 编码器)
     *   - 电机斜坡启停 (ramp)
     */

    for (;;) {
        /*
         * ── PID 控制循环 (待实现) ──
         *
         * 1. 从命令队列接收目标速度/位置 (非阻塞)
         * 2. 读取当前编码器值 → 计算速度
         * 3. PID 计算 → 更新 PWM 占空比
         * 4. Motor_set(pwma, pwmb)
         */

        /* 占位: 保持停转 */
        /* Motor_set(0, 0); */  /* 已由 Sensor 任务初始化 */

        vTaskDelay(pdMS_TO_TICKS(TASK_MOTOR_PERIOD_MS));
    }
}

/* ══════ 创建任务 ══════ */
void TaskMotor_create(void)
{
    BaseType_t ret = xTaskCreate(
        prvMotorTask,
        "Motor",
        TASK_MOTOR_STACK_SIZE,
        NULL,
        TASK_MOTOR_PRIO,
        NULL
    );
    if (ret != pdPASS) {
        BSP_UART_tx_str("[Motor] Failed to create task!\r\n");
    }
}
