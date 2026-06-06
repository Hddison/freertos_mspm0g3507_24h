/*
 * ============ app_control.c =============
 * 3 层级联控制 + 竞赛状态机实现
 */

#include "app_control.h"
#include "app_config.h"
#include "app_kalman.h"
#include "app_motor.h"
#include "hw_motor.h"

#include <math.h>    /* sqrtf, sinf, cosf, atan2f, fabsf */
#include <stdlib.h>   /* abs */
#include <string.h>

/* ══════════ PID 实例定义 ══════════ */

pid_t g_pid_speed;
pid_t g_pid_pos;
pid_t g_pid_heading;
pid_t g_pid_steer;
pid_t g_pid_speed_l;          /* 左轮速度 PID */
pid_t g_pid_speed_r;          /* 右轮速度 PID */
float g_target_speed = DEFAULT_TARGET_SPEED;

/* ══════════ 竞赛状态 ══════════ */

competition_t g_comp;
volatile int g_ctrl_mode = CTRL_IDLE;
volatile int g_kf_vertex_trigger = -1;  /* 顶点修正触发: -1=无, 0=A,1=B,2=C,3=D */
bool    g_debug_step_mode = false;        /* debug 单步模式 */
volatile bool g_debug_step_continue = false; /* 长按继续信号 */

/* ══════════ 路径段表 ══════════ */

/* Task 1: A→B (8段, 但只需前2段) */
static const path_segment_t path_task1[] = {
    /* A(-500,400) → B(500,400): dx=1000,dy=0, heading=0, dist=1000 */
    {CTRL_POSITION,     VERTEX_B_X, VERTEX_B_Y, 1000, 0.0f,     0, 1},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 1},
    {CTRL_COMPLETE,     0, 0, 0, 0, 0, -1},
};

/* Task 2: A→B→右弧→C→D→左弧→A (顺时针1圈, 8段) */
static const path_segment_t path_task2[] = {
    {CTRL_POSITION,     VERTEX_B_X, VERTEX_B_Y, 1000, 0.0f,     0, 1},  /* A→B */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 1},
    {CTRL_LINE_TRACK,   VERTEX_C_X, VERTEX_C_Y, 0,    0.0f,     1, -1},  /* B→C 右弧 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 2},
    {CTRL_POSITION,     VERTEX_D_X, VERTEX_D_Y, 1000, 3.14159f, 0, 3},  /* C→D 西 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 3},
    {CTRL_LINE_TRACK,   VERTEX_A_X, VERTEX_A_Y, 0,    0.0f,    -1, -1},  /* D→A 左弧 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 0},
    {CTRL_COMPLETE,     0, 0, 0, 0, 0, -1},
};

/* Task 3: A→C(对角)→右弧上行→B→D(对角)→左弧上行→A (8段) */
static const path_segment_t path_task3[] = {
    {CTRL_POSITION,     VERTEX_C_X, VERTEX_C_Y, 1280, -0.6747f,  0, 2},  /* A→C */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 2},
    {CTRL_LINE_TRACK,   VERTEX_B_X, VERTEX_B_Y, 0,    0.0f,      1, -1},  /* C→B 右弧 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 1},
    {CTRL_POSITION,     VERTEX_D_X, VERTEX_D_Y, 1290, -2.4069f,  0, 3},  /* B→D */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 3},
    {CTRL_LINE_TRACK,   VERTEX_A_X, VERTEX_A_Y, 0,    0.0f,     -1, -1},  /* D→A 左弧 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 0},
    {CTRL_COMPLETE,     0, 0, 0, 0, 0, -1},
};

/* Task 4: 4圈展开, 每圈可独立微调航向/距离 */
static const path_segment_t path_task4[] = {
    /* ── Lap 1 ── */
    {CTRL_POSITION,     VERTEX_C_X, VERTEX_C_Y, 1280, -0.6747f,  0, 2},  /* A→C */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 2},
    {CTRL_LINE_TRACK,   VERTEX_B_X, VERTEX_B_Y, 0,    0.0f,      1, -1},  /* C→B 右弧 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 1},
    {CTRL_POSITION,     VERTEX_D_X, VERTEX_D_Y, 1290, -2.4069f,  0, 3},  /* B→D */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 3},
    {CTRL_LINE_TRACK,   VERTEX_A_X, VERTEX_A_Y, 0,    0.0f,     -1, -1},  /* D→A 左弧 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 0},
    /* ── Lap 2 ── */
    {CTRL_POSITION,     VERTEX_C_X, VERTEX_C_Y, 1280, -0.6047f,  0, 2},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 2},
    {CTRL_LINE_TRACK,   VERTEX_B_X, VERTEX_B_Y, 0,    0.0f,      1, -1},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 1},
    {CTRL_POSITION,     VERTEX_D_X, VERTEX_D_Y, 1290, -2.3600f,  0, 3},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 3},
    {CTRL_LINE_TRACK,   VERTEX_A_X, VERTEX_A_Y, 0,    0.0f,     -1, -1},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 0},
    /* ── Lap 3 ── */
    {CTRL_POSITION,     VERTEX_C_X, VERTEX_C_Y, 1280, -0.5750f,  0, 2},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 2},
    {CTRL_LINE_TRACK,   VERTEX_B_X, VERTEX_B_Y, 0,    0.0f,      1, -1},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 1},
    {CTRL_POSITION,     VERTEX_D_X, VERTEX_D_Y, 1300, -2.3500f,  0, 3},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 3},
    {CTRL_LINE_TRACK,   VERTEX_A_X, VERTEX_A_Y, 0,    0.0f,     -1, -1},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 0},
    /* ── Lap 4 ── */
    {CTRL_POSITION,     VERTEX_C_X, VERTEX_C_Y, 1280, -0.5550f,  0, 2},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 2},
    {CTRL_LINE_TRACK,   VERTEX_B_X, VERTEX_B_Y, 0,    0.0f,      1, -1},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 1},
    {CTRL_POSITION,     VERTEX_D_X, VERTEX_D_Y, 1350, -2.3200f,  0, 3},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 3},
    {CTRL_LINE_TRACK,   VERTEX_A_X, VERTEX_A_Y, 0,    0.0f,     -1, -1},
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0, 0},
};
#define TASK4_LAPS 1  /* 不再用循环, 表已展开4圈 */

/* ══════════ 段切换 ══════════ */

static void advance_segment(void)
{
    g_comp.active_segment++;
    const path_segment_t *seg = &g_comp.segments[g_comp.active_segment];

    /* CTRL_COMPLETE: 多圈检查 — 在加载段之前判, 否则 COMPLETE 段吞掉多圈 */
    if (seg->mode == CTRL_COMPLETE) {
        if (g_comp.task_id == 4 && g_comp.lap < g_comp.total_laps - 1) {
            g_comp.lap++;
            g_comp.active_segment = 0;
            seg = &g_comp.segments[0];
        } else {
            g_comp.mode = CTRL_COMPLETE;
            g_ctrl_mode = CTRL_COMPLETE;
            Motor_set(0, 0);
            return;
        }
    }

    g_comp.mode           = seg->mode;
    g_ctrl_mode            = seg->mode;
    g_comp.segment_start_tick = g_comp.elapsed_ms;
    g_comp.segment_start_dist = (Motor_enc1Dist()+Motor_enc2Dist())*0.5f;
    g_comp.line_lost_cnt  = 0;
    g_comp.line_detected  = false;

    /* CTRL_POSITION: 查表获取航向+距离 */
    if (seg->mode == CTRL_POSITION) {
        g_comp.segment_target_hdg = seg->target_heading * 57.29578f;  /* rad→° */
        g_comp.segment_distance   = seg->seg_dist;
    }
}

/* ══════════ 初始化 ══════════ */

void control_init(void)
{
    /* PID 实例初始化 (默认值, flash_config 加载后会覆盖) */
    pid_init(&g_pid_speed_l, DEFAULT_SPEED_L_KP, DEFAULT_SPEED_L_KI,
             DEFAULT_SPEED_L_KD, DEFAULT_SPEED_I_LIM, PWM_MAX);
    pid_init(&g_pid_speed_r, DEFAULT_SPEED_R_KP, DEFAULT_SPEED_R_KI,
             DEFAULT_SPEED_R_KD, DEFAULT_SPEED_I_LIM, PWM_MAX);
    pid_init(&g_pid_pos,     DEFAULT_POS_KP,     0.0f,
             0.0f,            DEFAULT_POS_I_LIM,  DEFAULT_TARGET_SPEED);
    pid_init(&g_pid_heading, DEFAULT_HEADING_KP,  DEFAULT_HEADING_KI,
             DEFAULT_HEADING_KD, DEFAULT_HEADING_I_LIM, PWM_MAX);
    pid_init(&g_pid_steer,   DEFAULT_STEER_KP,    0.0f,
             DEFAULT_STEER_KD, DEFAULT_STEER_I_LIM, PWM_MAX);

    g_target_speed = DEFAULT_TARGET_SPEED;

    memset(&g_comp, 0, sizeof(g_comp));
    g_comp.mode  = CTRL_IDLE;
    g_ctrl_mode  = CTRL_IDLE;
}

/* 从 Flash 配置加载 PID 参数 */
void control_load_from_flash(const flash_config_t *cfg)
{
    pid_set_gains(&g_pid_speed_l, cfg->speed_l_kp, cfg->speed_l_ki, cfg->speed_l_kd);
    pid_set_gains(&g_pid_speed_r, cfg->speed_r_kp, cfg->speed_r_ki, cfg->speed_r_kd);
    pid_set_gains(&g_pid_pos,     cfg->pos_kp,    0.0f,            0.0f);
    pid_set_gains(&g_pid_heading, cfg->heading_kp, cfg->heading_ki, cfg->heading_kd);
    pid_set_gains(&g_pid_steer,   cfg->steer_kp,  0.0f,            cfg->steer_kd);
    g_target_speed = cfg->target_speed;
}

/* ══════════ 启动竞赛任务 ══════════ */

void control_start_task(uint8_t task_id)
{
    g_comp.task_id = task_id;
    g_comp.lap     = 0;
    g_comp.active_segment = 0;
    g_comp.elapsed_ms = 0;
    g_comp.line_lost_cnt = 0;
    g_comp.line_detected = false;

    switch (task_id) {
    case 1:
        g_comp.segments      = path_task1;
        g_comp.total_segments = sizeof(path_task1) / sizeof(path_segment_t);
        g_comp.total_laps    = 1;
        break;
    case 2:
        g_comp.segments      = path_task2;
        g_comp.total_segments = sizeof(path_task2) / sizeof(path_segment_t);
        g_comp.total_laps    = 1;
        break;
    case 3:
        g_comp.segments      = path_task3;
        g_comp.total_segments = sizeof(path_task3) / sizeof(path_segment_t);
        g_comp.total_laps    = 1;
        break;
    case 4:
        g_comp.segments      = path_task4;
        g_comp.total_segments = sizeof(path_task4) / sizeof(path_segment_t);
        g_comp.total_laps    = TASK4_LAPS;
        break;
    default:
        g_comp.mode = CTRL_IDLE;
        g_ctrl_mode = CTRL_IDLE;
        return;
    }

    /* 启动第一个段 */
    const path_segment_t *seg = &g_comp.segments[0];
    g_comp.mode = seg->mode;
    g_ctrl_mode = seg->mode;
    g_comp.segment_start_tick = g_comp.elapsed_ms;
    g_comp.segment_start_dist = (Motor_enc1Dist()+Motor_enc2Dist())*0.5f;
    if (seg->mode == CTRL_POSITION) {
        extern volatile float g_kf_x, g_kf_y;
        float dx = seg->target_x - g_kf_x;
        float dy = seg->target_y - g_kf_y;
        g_comp.segment_target_hdg = seg->target_heading * 57.29578f;  /* rad→° */
        g_comp.segment_distance   = sqrtf(dx*dx + dy*dy);
    }

    /* 重置 PID 积分 */
    pid_reset(&g_pid_speed_l);
    pid_reset(&g_pid_speed_r);
    pid_reset(&g_pid_pos);
    pid_reset(&g_pid_heading);
    pid_reset(&g_pid_steer);
}

void control_estop(void)
{
    g_comp.mode  = CTRL_ESTOP;
    g_ctrl_mode  = CTRL_ESTOP;
    Motor_set(0, 0);
}

/* ══════════ 控制循环 (每 10ms) ══════════ */

void control_run(const kalman5_t *kf, float line_position, uint16_t gray_raw,
                 float yaw_deg)
{
    (void)kf;  /* 航向用 yaw_deg, 不用 kf->theta */
    /* 更新计时 */
    g_comp.elapsed_ms += 10;

    const path_segment_t *seg = &g_comp.segments[g_comp.active_segment];

    switch (g_comp.mode) {

    /* ── 位置闭环: 目标点导航 ── */
    case CTRL_POSITION: {
        /* 航向误差 (°), 和 Heading Hold 一样 */
        float heading_err = g_comp.segment_target_hdg - yaw_deg;
        while (heading_err >  180.0f) heading_err -= 360.0f;
        while (heading_err < -180.0f) heading_err += 360.0f;

        /* 距离: 编码器已走 → 剩余 */
        float enc_avg = (Motor_enc1Dist() + Motor_enc2Dist()) * 0.5f;
        float traveled = enc_avg - g_comp.segment_start_dist;
        float remain   = g_comp.segment_distance - traveled;

        /* 速度曲线 */
        float target_spd;
        if (remain > SPEED_RAMP_DIST)       target_spd = g_target_speed;
        else if (remain < VERTEX_ARRIVAL_DIST) target_spd = 0.0f;
        else                                target_spd = g_target_speed * remain / SPEED_RAMP_DIST;
        if (target_spd < MIN_CRUISE_SPEED && remain > VERTEX_ARRIVAL_DIST)
            target_spd = MIN_CRUISE_SPEED;

        /* 航向 PID → 速度差 */
        float dt = 0.01f;
        float speed_diff = pid_compute(&g_pid_heading, 0.0f, heading_err, dt);
        if (speed_diff >  500.0f) speed_diff =  500.0f;
        if (speed_diff < -500.0f) speed_diff = -500.0f;

        motor_speed_apply(target_spd + speed_diff,
                          target_spd - speed_diff, dt, NULL, NULL);

        /* 到点: 编码器走够 + 最小段时长 500ms (防误触发) */
        if (remain <= 0.0f &&
            (g_comp.elapsed_ms - g_comp.segment_start_tick) > 500) {
            advance_segment();
        }
        break;
    }

    /* ── 弧线循迹: 灰度质心23 → 转向 ── */
    case CTRL_LINE_TRACK: {
        /* 入线检测 */
        if (!g_comp.line_detected && gray_raw != 0) {
            g_comp.line_detected = true;
        }

        /* 灰度 PID → 速度差 (mm/s) */
        float speed_diff = 0.0f;
        if (g_comp.line_detected) {
            speed_diff = pid_compute(&g_pid_steer, 0.0f, -line_position, 0.01f);
            if (speed_diff >  500.0f) speed_diff =  500.0f;
            if (speed_diff < -500.0f) speed_diff = -500.0f;
        }

        /* per-motor 速度控制 */
        motor_speed_apply(g_target_speed + speed_diff,
                          g_target_speed - speed_diff, 0.01f, NULL, NULL);

        /* 出线检测: 连续丢失 N 个周期 → 切换到 POSITION 到达目标 */
        if (g_comp.line_detected) {
            if (gray_raw == 0) {
                g_comp.line_lost_cnt++;
            } else {
                g_comp.line_lost_cnt = 0;
            }
        }

        if (g_comp.line_lost_cnt > LINE_LOST_THRESH) {
            /* 出线 → 按已知目标点导航 */
            g_comp.mode    = CTRL_POSITION;
            g_ctrl_mode    = CTRL_POSITION;
            g_comp.line_detected = false;
            g_comp.line_lost_cnt = 0;
            pid_reset(&g_pid_steer);
        }
        break;
    }

    /* ── 顶点停车 ── */
    case CTRL_VERTEX_PAUSE: {
        Motor_set(0, 0);
        pid_reset(&g_pid_heading);
        pid_reset(&g_pid_speed_l);
        pid_reset(&g_pid_speed_r);
        pid_reset(&g_pid_steer);
        pid_reset(&g_pid_pos);
        if (g_comp.vertex_pause_start == 0) {
            g_comp.vertex_pause_start = g_comp.elapsed_ms;
            extern void buzzer_beep(uint16_t ms);
            buzzer_beep(200);
        }

        /* Kalman 顶点修正 */
        if (seg->vertex_id >= 0) {
            extern volatile int g_kf_vertex_trigger;
            g_kf_vertex_trigger = seg->vertex_id;
        }

        /* debug 单步模式: 等 CENTER 长按; 正常模式: 800ms 自动前进 */
        if (g_debug_step_mode) {
            extern volatile bool g_debug_step_continue;
            if (g_debug_step_continue) {
                g_debug_step_continue = false;
                g_comp.vertex_pause_start = 0;
                pid_reset(&g_pid_heading);
                advance_segment();
            }
        } else {
            if ((g_comp.elapsed_ms - g_comp.vertex_pause_start) >= VERTEX_PAUSE_MS) {
                g_comp.vertex_pause_start = 0;
                pid_reset(&g_pid_heading);
                advance_segment();
            }
        }
        break;
    }

    /* ── 任务完成 ── */
    case CTRL_COMPLETE:
    case CTRL_ESTOP:
        Motor_set(0, 0);
        break;

    /* ── 空闲 ── */
    case CTRL_IDLE:
    default:
        break;
    }
}

/* ══════════ Getters (LCD 任务用) ══════════ */

ctrl_mode_t control_get_mode(void)    { return g_comp.mode; }
uint8_t     control_get_lap(void)     { return g_comp.lap; }
uint32_t    control_get_elapsed_ms(void) { return g_comp.elapsed_ms; }
float       control_get_cur_speed(void) { return g_target_speed; }
