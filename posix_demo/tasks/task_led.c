/*
 *  ============ task_led.c =============
 *  LED 闪烁任务实现
 *
 *  这是一个周期任务的标准模板:
 *    1. 初始化 (一次)
 *    2. for(;;) 循环
 *    3. 执行周期操作
 *    4. vTaskDelay 等待下一个周期
 */

#include "task_led.h"

#include <FreeRTOS.h>
#include <task.h>

#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>

#include "app/app_config.h"
#include "bsp_uart.h"

/* ══════ 任务函数 ══════ */
static void prvLedTask(void *pvParameters)
{
    (void)pvParameters;
    BSP_UART_tx_str("[LED] Started\r\n");

    for (;;) {
        DL_GPIO_togglePins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
        vTaskDelay(pdMS_TO_TICKS(TASK_LED_PERIOD_MS));
    }
}

/* ══════ 创建任务 ══════ */
void TaskLed_create(void)
{
    BaseType_t ret = xTaskCreate(
        prvLedTask,
        "LED",
        TASK_LED_STACK_SIZE,
        NULL,
        TASK_LED_PRIO,
        NULL
    );
    if (ret != pdPASS) {
        BSP_UART_tx_str("[LED] Failed to create task!\r\n");
    }
}
