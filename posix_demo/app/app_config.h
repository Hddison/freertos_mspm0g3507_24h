/*
 *  ============ app_config.h =============
 *  应用层配置 — 任务周期 / 优先级 / 栈大小 / 队列深度
 *
 *  所有可调参数集中于此, 方便后续维护和调优
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <FreeRTOS.h>
#include <task.h>

/* ── 任务周期 (ms) ── */
#define TASK_LED_PERIOD_MS      500
#define TASK_SENSOR_PERIOD_MS   50
#define TASK_LCD_PERIOD_MS      50
#define TASK_MOTOR_PERIOD_MS    10

/* ── 任务优先级 (数值越大优先级越高, 0 = idle) ── */
#define TASK_LED_PRIO           (tskIDLE_PRIORITY + 1)
#define TASK_SENSOR_PRIO        (tskIDLE_PRIORITY + 2)
#define TASK_LCD_PRIO           (tskIDLE_PRIORITY + 3)
#define TASK_MOTOR_PRIO         (tskIDLE_PRIORITY + 4)

/* ── 栈大小 (字, 4 bytes/word on Cortex-M0+) ── */
#define TASK_LED_STACK_SIZE     128
#define TASK_SENSOR_STACK_SIZE  512
#define TASK_LCD_STACK_SIZE     1024
#define TASK_MOTOR_STACK_SIZE   512

/* ── 传感器数据队列深度 (必须为 1: xQueueOverwrite 要求) ──
 * mailbox 模式: LCD 任务始终消费最新数据, 旧帧自动覆盖
 */
#define SENSOR_QUEUE_LENGTH     1

#endif /* APP_CONFIG_H */
