/* app_config.h */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include <FreeRTOS.h>
#include <task.h>

#define TASK_SENSOR_PERIOD_MS   10
#define TASK_CONTROL_PERIOD_MS  10
#define TASK_BUTTON_PERIOD_MS   20
#define TASK_LCD_PERIOD_MS      50

#define TASK_SENSOR_PRIO        (tskIDLE_PRIORITY + 6)
#define TASK_CONTROL_PRIO       (tskIDLE_PRIORITY + 5)
#define TASK_LCD_PRIO           (tskIDLE_PRIORITY + 4)
#define TASK_BUTTON_PRIO        (tskIDLE_PRIORITY + 3)

#define TASK_SENSOR_STACK_SIZE  512
#define TASK_CONTROL_STACK_SIZE 256
#define TASK_LCD_STACK_SIZE     1024
#define TASK_BUTTON_STACK_SIZE  128

#define SENSOR_QUEUE_LENGTH     1
#define BUTTON_QUEUE_LENGTH     8

#endif
