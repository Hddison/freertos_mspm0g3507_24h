/*
 *  ============ task_led.h =============
 *  LED 闪烁任务 — 最简单的 FreeRTOS 任务参考
 *
 *  模式: 周期性 (periodic task)
 *  - vTaskDelay 实现固定周期
 *  - 不依赖任何队列/信号量
 *  - 仅使用 GPIO 翻转
 */

#ifndef TASK_LED_H
#define TASK_LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* 创建 LED 任务, 在内部调用 xTaskCreate */
void TaskLed_create(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_LED_H */
