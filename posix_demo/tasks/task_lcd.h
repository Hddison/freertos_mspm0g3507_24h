/*
 *  ============ task_lcd.h =============
 *  LCD 显示任务 — 接收传感器数据并渲染
 *
 *  模式: 消费者任务 (consumer task)
 *  - 初始化时绘制静态 UI 元素
 *  - 从队列接收传感器数据 (阻塞, 有超时)
 *  - 每次收到数据后刷新动态数值区域
 *
 *  布局 (170×320 竖屏):
 *   ┌──────────────┐
 *   │ JY61P IMU    │ y=5   Font16
 *   │──────────────│ y=26
 *   │ R: -12.34    │ y=38  Font20
 *   │ P:  5.67     │ y=68  Font20
 *   │ Y:  89.01    │ y=98  Font20
 *   │──────────────│ y=135
 *   │ Total: 720°  │ y=148  Font12+Font24
 *   │──────────────│ y=255
 *   │ Enc: M1:...  │ y=258  Font8
 *   │──────────────│ y=275
 *   │ GS: ████     │ y=278  12 方块
 *   │──────────────│ y=305
 *   │ MSPM0G3507   │ y=310  Font8 (状态)
 *   └──────────────┘
 */

#ifndef TASK_LCD_H
#define TASK_LCD_H

#include <FreeRTOS.h>
#include <queue.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 创建 LCD 任务
 *
 * 参数: sensorQueue — 传感器数据队列 (由 TaskSensor_create 返回)
 *       阻塞等待队列数据, 收到后渲染更新
 */
void TaskLcd_create(QueueHandle_t sensorQueue);

#ifdef __cplusplus
}
#endif

#endif /* TASK_LCD_H */
