/*
 * ============ task_button.h =============
 * 按键扫描任务 (50 Hz, idle+3)
 *
 * 消费: BSP_Button_Scan() (bsp_button 状态机)
 * 生产: button_queue → TaskLcd
 */

#ifndef TASK_BUTTON_H
#define TASK_BUTTON_H

#include <stdint.h>
#include <stdbool.h>

/* ── 按键事件 (队列元素) ── */
typedef struct {
    uint8_t  physical;      /* 物理按键 ID (BTN_ID_UP ~ BTN_ID_BUTTON) */
    uint8_t  logical;       /* 逻辑方向 ID (BTN_DIR_UP ~ BTN_DIR_BACK) */
    uint8_t  event_type;    /* BTN_EVT_PRESS_DOWN / SHORT / LONG / HOLD / RELEASE */
    uint32_t tick;          /* 事件时间戳 (xTaskGetTickCount) */
} button_event_t;

/* ── 控制命令 (cmd_queue 元素, LCD → Control) ── */
typedef enum {
    CMD_NONE = 0,
    CMD_START_TASK1,        /* 竞赛任务 1: A→B */
    CMD_START_TASK2,        /* 竞赛任务 2: 1圈  */
    CMD_START_TASK3,        /* 竞赛任务 3: 对角 */
    CMD_START_TASK4,        /* 竞赛任务 4: 4圈  */
    CMD_ESTOP,              /* 急停 */
    CMD_RESUME,             /* 恢复运行 */
} ctrl_cmd_t;

/* ── 传感器数据 (sensor_queue 元素, Sensor → LCD) ── */
typedef struct {
    bool     jy61p_ok, nchd12_ok;
    float    roll, pitch, yaw, total_yaw;
    float    ax, ay, az;           /* 加速度 (m/s²)               */
    float    gx, gy, gz;           /* 角速度 (°/s)                */
    float    enc1_dist, enc2_dist; /* 编码器距离 (mm)             */
    float    enc_speed;            /* 瞬时速度 (mm/s)             */
    uint16_t grayscale;            /* 12 位灰度位图               */
    float    line_position;        /* 灰度质心 (-5.5 ~ +5.5)     */
} sensor_data_t;

/* ── 按键重映射表 (Flash 可持久化) ── */
extern uint8_t g_btn_remap[6];

/* ── 队列句柄 (main.c 创建, 任务使用) ── */
extern void *g_button_queue;
extern void *g_cmd_queue;
extern void *g_sensor_queue;

/* ── 任务入口 ── */
void vTaskButton(void *pvParameters);
void vTaskLcd(void *pvParameters);
void vTaskSensor(void *pvParameters);
void vTaskControl(void *pvParameters);

#endif /* TASK_BUTTON_H */
