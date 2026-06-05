/*
 * ============ app_control.c =============
 * 3 层级联控制 + 竞赛状态机实现
 */

#include "app_control.h"
#include "app_config.h"
#include "app_kalman.h"
#include "hw_motor.h"

#include <math.h>    /* sqrtf, sinf, cosf, atan2f, fabsf */
#include <stdlib.h>   /* abs */
#include <string.h>

/* ══════════ PID 实例定义 ══════════ */

pid_t g_pid_speed;
pid_t g_pid_pos;
pid_t g_pid_heading;
pid_t g_pid_steer;
float g_target_speed = DEFAULT_TARGET_SPEED;

/* ══════════ 竞赛状态 ══════════ */

competition_t g_comp;
volatile int g_ctrl_mode = CTRL_IDLE;

/* ══════════ 辅助函数 ══════════ */

/* 角度归一化到 [-PI, +PI] */
static float wrap_angle(float a)
{
    while (a >  3.14159265359f) a -= 2.0f * 3.14159265359f;
    while (a < -3.14159265359f) a += 2.0f * 3.14159265359f;
    return a;
}

/* 整数钳位 */
static int16_t clamp_i16(float val, int16_t lo, int16_t hi)
{
    if (val > (float)hi) return hi;
    if (val < (float)lo) return lo;
    return (int16_t)val;
}

/* ══════════ 路径段表 ══════════ */

/* Task 1: A→B (8段, 但只需前2段) */
static const path_segment_t path_task1[] = {
    {CTRL_POSITION,     VERTEX_B_X, VERTEX_B_Y, 0.0f,   0, 1},  /* A→B */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 1},                          /* B 点 */
    {CTRL_COMPLETE,     0, 0, 0, 0, -1},
};

/* Task 2: A→B→右弧→C→D→左弧→A (顺时针1圈, 8段) */
static const path_segment_t path_task2[] = {
    {CTRL_POSITION,     VERTEX_B_X, VERTEX_B_Y, 0.0f,   0, 1},  /* A→B */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 1},                          /* B 点 */
    {CTRL_LINE_TRACK,   VERTEX_C_X, VERTEX_C_Y, 0.0f,  1, -1},  /* B→C 右弧 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 2},                          /* C 点 */
    {CTRL_POSITION,     VERTEX_D_X, VERTEX_D_Y, 3.14159f, 0, 3},/* C→D 西 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 3},                          /* D 点 */
    {CTRL_LINE_TRACK,   VERTEX_A_X, VERTEX_A_Y, 0.0f, -1, -1},  /* D→A 左弧 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0},                          /* A 点 */
    {CTRL_COMPLETE,     0, 0, 0, 0, -1},
};

/* Task 3: A→C(对角)→右弧上行→B→D(对角)→左弧上行→A (8段) */
static const path_segment_t path_task3[] = {
    {CTRL_POSITION,     VERTEX_C_X, VERTEX_C_Y, -0.6747f, 0, 2},/* A→C 东南 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 2},                          /* C 点 */
    {CTRL_LINE_TRACK,   VERTEX_B_X, VERTEX_B_Y, 0.0f,  1, -1},  /* C→B 右弧上行 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 1},                          /* B 点 */
    {CTRL_POSITION,     VERTEX_D_X, VERTEX_D_Y, -2.2469f, 0, 3},/* B→D 西南 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 3},                          /* D 点 */
    {CTRL_LINE_TRACK,   VERTEX_A_X, VERTEX_A_Y, 0.0f, -1, -1},  /* D→A 左弧上行 */
    {CTRL_VERTEX_PAUSE, 0, 0, 0, 0, 0},                          /* A 点 */
    {CTRL_COMPLETE,     0, 0, 0, 0, -1},
};

/* Task 4: 同 Task 3, 4 圈 */
#define TASK4_LAPS 4

/* ══════════ 段切换 ══════════ */

static void advance_segment(void)
{
    g_comp.active_segment++;
    if (g_comp.active_segment >= g_comp.total_segments) {
        /* Task 4: 检查是否需要多圈 */
        if (g_comp.task_id == 4 && g_comp.lap < g_comp.total_laps - 1) {
            g_comp.lap++;
            g_comp.active_segment = 0;  /* 重新开始 */
        } else {
            g_comp.mode = CTRL_COMPLETE;
            g_ctrl_mode = CTRL_COMPLETE;
            Motor_set(0, 0);
            return;
        }
    }

    const path_segment_t *seg = &g_comp.segments[g_comp.active_segment];
    g_comp.mode           = seg->mode;
    g_ctrl_mode            = seg->mode;
    g_comp.segment_start_tick = 0;   /* 将在 control_run 中设置 */
    g_comp.segment_start_dist = 0;
    g_comp.line_lost_cnt  = 0;
    g_comp.line_detected  = false;

    if (seg->mode == CTRL_COMPLETE) {
        Motor_set(0, 0);
    }
}

/* ══════════ 初始化 ══════════ */

void control_init(void)
{
    /* PID 实例初始化 (默认值, flash_config 加载后会覆盖) */
    pid_init(&g_pid_speed,   DEFAULT_SPEED_KP,   DEFAULT_SPEED_KI,
             DEFAULT_SPEED_KD, DEFAULT_SPEED_I_LIM, PWM_MAX);
    pid_init(&g_pid_pos,     DEFAULT_POS_KP,     0.0f,
             0.0f,            DEFAULT_POS_I_LIM,  DEFAULT_TARGET_SPEED);
    pid_init(&g_pid_heading, DEFAULT_HEADING_KP,  0.0f,
             0.0f,            DEFAULT_HEADING_I_LIM, PWM_MAX);
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
    pid_set_gains(&g_pid_speed,   cfg->speed_kp,  cfg->speed_ki,  cfg->speed_kd);
    pid_set_gains(&g_pid_pos,     cfg->pos_kp,    0.0f,            0.0f);
    pid_set_gains(&g_pid_heading, cfg->heading_kp, 0.0f,            0.0f);
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
        g_comp.segments      = path_task3;  /* same path as task 3 */
        g_comp.total_segments = sizeof(path_task3) / sizeof(path_segment_t);
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
    g_comp.segment_start_tick = 0;

    /* 重置 PID 积分 */
    pid_reset(&g_pid_speed);
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

void control_run(const kalman5_t *kf, float line_position, uint16_t gray_raw)
{
    /* 更新计时 */
    g_comp.elapsed_ms += 10;

    const path_segment_t *seg = &g_comp.segments[g_comp.active_segment];

    switch (g_comp.mode) {

    /* ── 位置闭环: 目标点导航 ── */
    case CTRL_POSITION: {
        float dx = seg->target_x - kf->x;
        float dy = seg->target_y - kf->y;
        float dist = sqrtf(dx * dx + dy * dy);
        float target_heading = atan2f(dy, dx);
        float heading_err = wrap_angle(target_heading - kf->theta);

        /* 速度曲线: 远距满速, 近距减速 */
        float target_spd;
        if (dist > SPEED_RAMP_DIST) {
            target_spd = g_target_speed;
        } else if (dist < VERTEX_ARRIVAL_DIST) {
            target_spd = 0.0f;
        } else {
            target_spd = g_target_speed * (dist / SPEED_RAMP_DIST);
        }
        if (target_spd < MIN_CRUISE_SPEED && dist > VERTEX_ARRIVAL_DIST)
            target_spd = MIN_CRUISE_SPEED;

        /* 速度 PID (内环) */
        float dt = 0.01f;
        float base_pwm = pid_compute(&g_pid_speed, target_spd, kf->v, dt);

        /* Pure pursuit 曲率: κ = 2·sin(Δθ)/d */
        float curvature = 2.0f * sinf(heading_err) / fmaxf(dist, 1.0f);

        /* 曲率→差速 + 航向 PD 补偿 */
        float steer_pwm = curvature * kf->v * CURVATURE_GAIN;

        /* 航向 PID (辅助, 抑制振荡) */
        float heading_correction = pid_compute(&g_pid_heading, 0.0f,
                                               heading_err, dt);
        steer_pwm += heading_correction;

        /* 差分输出 */
        int16_t left  = clamp_i16(base_pwm - steer_pwm, -PWM_MAX, PWM_MAX);
        int16_t right = clamp_i16(base_pwm + steer_pwm, -PWM_MAX, PWM_MAX);

        /* 死区 */
        if (abs(left)  < PWM_DEAD_ZONE) left  = 0;
        if (abs(right) < PWM_DEAD_ZONE) right = 0;

        Motor_set(left, right);

        /* 到点判定 */
        if (dist < VERTEX_ARRIVAL_DIST) {
            advance_segment();  /* → VERTEX_PAUSE */
        }
        break;
    }

    /* ── 弧线循迹: 灰度质心 → 转向 ── */
    case CTRL_LINE_TRACK: {
        /* 入线检测 */
        if (!g_comp.line_detected && gray_raw != 0) {
            g_comp.line_detected = true;
        }

        float steer_pwm = 0.0f;
        if (g_comp.line_detected) {
            /* 灰度 PID (对质心偏差做闭环) */
            steer_pwm = pid_compute(&g_pid_steer, 0.0f, line_position, 0.01f);
        }

        /* 速度 PID */
        float base_pwm = pid_compute(&g_pid_speed, g_target_speed, kf->v, 0.01f);

        int16_t left  = clamp_i16(base_pwm - steer_pwm, -PWM_MAX, PWM_MAX);
        int16_t right = clamp_i16(base_pwm + steer_pwm, -PWM_MAX, PWM_MAX);

        if (abs(left)  < PWM_DEAD_ZONE) left  = 0;
        if (abs(right) < PWM_DEAD_ZONE) right = 0;

        Motor_set(left, right);

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

        if (g_comp.vertex_pause_start == 0) {
            g_comp.vertex_pause_start = g_comp.elapsed_ms;
        }

        /* Kalman 顶点位置修正 — 标记由 Sensor 任务在下一周期执行 */
        if (seg->vertex_id >= 0) {
            /* TODO: 通过全局标志通知 Sensor 任务调用 kalman5_update_vertex() */
            /* 顶点坐标从 VERTEX_A~D_X/Y 宏获取 */
        }

        /* 800ms 后前进到下一段 */
        if ((g_comp.elapsed_ms - g_comp.vertex_pause_start) >= VERTEX_PAUSE_MS) {
            g_comp.vertex_pause_start = 0;
            pid_reset(&g_pid_speed);
            pid_reset(&g_pid_heading);
            advance_segment();
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
