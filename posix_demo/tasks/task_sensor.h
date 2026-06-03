/*
 *  ============ task_sensor.h =============
 *  传感器读取任务 — 周期性读取 JY61P + NCHD12
 *
 *  模式: 生产者任务 (producer task)
 *  - 初始化时重试外设连接 (最多 N 次)
 *  - 固定周期读取传感器
 *  - 通过队列发送数据给 LCD 任务
 */

#ifndef TASK_SENSOR_H
#define TASK_SENSOR_H

#include <FreeRTOS.h>
#include <queue.h>
#include <stdbool.h>
#include <stdint.h>
#include "app/app_flash.h"

/* ── 传感器数据包 (通过队列传输) ── */
typedef struct {
    bool     jy61p_ok;        /* JY61P 连接状态                      */
    bool     nchd12_ok;       /* NCHD12 连接状态                     */
    float    roll;            /* 横滚角 (°)                          */
    float    pitch;           /* 俯仰角 (°)                          */
    float    yaw;             /* 当前偏航角 (°)                      */
    float    total_yaw;       /* 累积偏航角 (°, 已解卷绕)            */
    float    enc1_dist;       /* 编码器 1 累计距离 (mm)              */
    float    enc2_dist;       /* 编码器 2 累计距离 (mm)              */
    uint16_t grayscale;       /* 12 路灰度 (bit0-11)                 */
} sensor_data_t;

/* ── 当前偏航角 (传感器任务 20Hz 更新, 电机任务读取) ── */
extern volatile float g_current_yaw;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 创建传感器任务, 返回数据队列句柄
 *
 * 初始化 JY61P (最多 10 次重试) 和 NCHD12 (最多 3 次重试),
 * 然后以 TASK_SENSOR_PERIOD_MS 周期读取数据并推入队列。
 *
 * 返回: 队列句柄 (LCD 任务通过此队列接收数据), NULL 表示失败
 */
QueueHandle_t TaskSensor_create(const app_flash_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* TASK_SENSOR_H */
