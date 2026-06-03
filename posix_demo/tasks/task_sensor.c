/*
 *  ============ task_sensor.c =============
 *  传感器读取任务实现
 *
 *  这是一个生产者任务的标准模板:
 *    1. 外设初始化 (带重试)
 *    2. 创建数据队列
 *    3. for(;;) 循环:
 *       a. 读取所有传感器
 *       b. 通过队列发送数据 (非阻塞)
 *       c. vTaskDelay 等待下一个周期
 */

#include "task_sensor.h"

#include <FreeRTOS.h>
#include <task.h>

#include "app/app_config.h"
#include "bsp_uart.h"
#include "bsp_button.h"
#include "hw_jy61p.h"
#include "hw_nchd12.h"
#include "hw_motor.h"

/* ══════ 任务函数 ══════ */
static void prvSensorTask(void *pvParameters)
{
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    sensor_data_t data;
    JY61P_RawAngle raw;
    JY61P_Angle    ang;

    BSP_UART_tx_str("[Sensor] Started\r\n");

    /* ── 上电等待 ── */
    vTaskDelay(pdMS_TO_TICKS(200));

    /* ── JY61P 初始化 (最多 10 次重试) ── */
    data.jy61p_ok = false;
    vTaskDelay(pdMS_TO_TICKS(500));
    for (int i = 0; i < 10; i++) {
        if (JY61P_init()) { data.jy61p_ok = true; break; }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* ── NCHD12 初始化 (最多 3 次重试) ── */
    data.nchd12_ok = false;
    {
        uint16_t dummy;
        vTaskDelay(pdMS_TO_TICKS(100));
        for (int i = 0; i < 3; i++) {
            if (NCHD12_read(&dummy)) { data.nchd12_ok = true; break; }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    /* ── 电机初始化 ── */
    Motor_init();
    Motor_set(0, 0);

    BSP_UART_tx_str(data.jy61p_ok  ? "[Sensor] IMU OK\r\n" : "[Sensor] IMU FAIL\r\n");
    BSP_UART_tx_str(data.nchd12_ok ? "[Sensor] GS  OK\r\n" : "[Sensor] GS  FAIL\r\n");

    /* ══════ 主循环: 读取 → 发送 ══════ */
    for (;;) {
        /* ── JY61P 角度 ── */
        if (data.jy61p_ok && JY61P_readAngle(&raw)) {
            JY61P_convAngle(&raw, &ang);
            JY61P_updateTotalYaw(raw.yaw);
            data.roll      = ang.roll;
            data.pitch     = ang.pitch;
            data.yaw       = ang.yaw;
            data.total_yaw = JY61P_getTotalYaw();
        }

        /* ── 编码器距离 ── */
        data.enc1_dist = Motor_enc1Dist();
        data.enc2_dist = Motor_enc2Dist();

        /* ── 灰度传感器 ── */
        data.grayscale = 0;
        if (data.nchd12_ok) {
            NCHD12_read(&data.grayscale);
        }

        /* ── 按键扫描 (短按清零编码器) ── */
        {
            uint8_t evt = BSP_Button_Scan();
            if (evt == BTN_EVT_SHORT) {
                Motor_encReset();
            }
        }

        /* ── 发送到 LCD 任务 (队列满则丢弃旧数据) ── */
        xQueueOverwrite(queue, &data);

        vTaskDelay(pdMS_TO_TICKS(TASK_SENSOR_PERIOD_MS));
    }
}

/* ══════ 创建任务 ══════ */
QueueHandle_t TaskSensor_create(void)
{
    /* 创建数据队列 (深度 = SENSOR_QUEUE_LENGTH) */
    QueueHandle_t queue = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(sensor_data_t));
    if (!queue) {
        BSP_UART_tx_str("[Sensor] Failed to create queue!\r\n");
        return NULL;
    }

    BaseType_t ret = xTaskCreate(
        prvSensorTask,
        "Sensor",
        TASK_SENSOR_STACK_SIZE,
        (void *)queue,          /* 参数: 队列句柄 */
        TASK_SENSOR_PRIO,
        NULL
    );
    if (ret != pdPASS) {
        BSP_UART_tx_str("[Sensor] Failed to create task!\r\n");
        vQueueDelete(queue);
        return NULL;
    }

    return queue;
}
