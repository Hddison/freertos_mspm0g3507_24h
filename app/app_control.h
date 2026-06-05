/*
 * ============ app_control.h =============
 * 3 层级联控制 + 竞赛状态机
 *
 * 控制模式:
 *   CTRL_IDLE         — 等待指令
 *   CTRL_POSITION      — 位置环 (→ target_speed + curvature)
 *   CTRL_LINE_TRACK    — 弧线循迹 (灰度质心 → steer)
 *   CTRL_VERTEX_PAUSE  — 顶点停车 + 声光提示
 *   CTRL_COMPLETE      — 任务完成
 *   CTRL_ESTOP         — 急停
 */

#ifndef APP_CONTROL_H
#define APP_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include "app_pid.h"
#include "app_kalman.h"
#include "app_flash.h"      /* flash_config_t */
#include "task_button.h"    /* ctrl_cmd_t */

/* ── 控制模式 ── */
typedef enum {
    CTRL_IDLE = 0,
    CTRL_POSITION,
    CTRL_LINE_TRACK,
    CTRL_VERTEX_PAUSE,
    CTRL_COMPLETE,
    CTRL_ESTOP
} ctrl_mode_t;

/* ── 路径段 ── */
typedef struct {
    ctrl_mode_t mode;
    float target_x;          /* 目标 X (mm) — POSITION 模式 */
    float target_y;          /* 目标 Y (mm) */
    float target_heading;    /* 参考航向 (rad) — LINE_TRACK 入线前 */
    int   arc_dir;           /* 弧线方向: +1=逆时针(右弧), -1=顺时针(左弧) */
    int   vertex_id;         /* 顶点编号: 0=A,1=B,2=C,3=D, -1=无 */
} path_segment_t;

/* ── 竞赛状态 ── */
typedef struct {
    uint8_t           task_id;
    const path_segment_t *segments;
    uint8_t           total_segments;
    uint8_t           active_segment;
    uint8_t           lap;
    uint8_t           total_laps;      /* Task4: 4 laps */
    ctrl_mode_t       mode;
    uint32_t          elapsed_ms;
    uint32_t          segment_start_tick;
    float             segment_start_dist;  /* 段起始编码器平均距离 */
    float             segment_target_hdg;  /* 段目标航向 (°, 段入口锁死) */
    float             segment_distance;    /* 段总距离 (mm, 编码器) */
    uint32_t          vertex_pause_start;  /* 顶点停车起始 tick */
    uint8_t           line_lost_cnt;       /* 出线计数器 */
    bool              line_detected;       /* 已检测到黑线 */
} competition_t;

/* ── 全局 PID 实例 ── */
extern pid_t g_pid_speed;
extern pid_t g_pid_pos;
extern pid_t g_pid_heading;
extern pid_t g_pid_steer;
extern pid_t g_pid_speed_l;       /* 左轮速度 PID */
extern pid_t g_pid_speed_r;       /* 右轮速度 PID */
extern float g_target_speed;

/* ── 全局竞赛状态 ── */
extern competition_t g_comp;

/* ── 当前控制模式 (volatile, Sensor 任务读取) ── */
extern volatile int g_ctrl_mode;

/* ── API ── */
void control_init(void);
void control_load_from_flash(const flash_config_t *cfg);
void control_run(const kalman5_t *kf, float line_position, uint16_t gray_raw,
                 float yaw_deg);   /* JY61P 当前航向 (°), 用于航向保持 */
void control_start_task(uint8_t task_id);
void control_estop(void);

/* Getters (LCD 用) */
ctrl_mode_t control_get_mode(void);
uint8_t     control_get_lap(void);
uint32_t    control_get_elapsed_ms(void);
float       control_get_cur_speed(void);

#endif /* APP_CONTROL_H */
