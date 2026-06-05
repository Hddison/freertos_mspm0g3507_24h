/*
 * ============ app_motor.h ============
 * 电机控制环 — 速度环 / 航向环 / 位置环
 *
 * 三层级联: Position → Heading → Speed → PWM
 * 每个环都可独立测试, 也可组合使用。
 */

#ifndef APP_MOTOR_H
#define APP_MOTOR_H

#include <stdint.h>
#include <stdbool.h>

/* ══════════ 底层: 左右独立速度环 ══════════ */
/* 读编码器 → per-motor PID → Motor_set。
 * 所有上层控制环最终都调用此函数输出 PWM。 */

void motor_speed_apply(float left_target_mmps, float right_target_mmps,
                       float dt, int16_t *out_lp, int16_t *out_rp);

/* ══════════ 速度环 (Speed Loop) ══════════ */
/* 左右轮同速直行, 各自独立 PID 闭环 */

typedef struct {
    bool  running;
    float target_mmps;       /* 目标速度 (mm/s) */
} motor_speed_loop_t;

void motor_speed_loop_init(motor_speed_loop_t *sl);
void motor_speed_loop_start(motor_speed_loop_t *sl, float target_mmps);
void motor_speed_loop_stop(motor_speed_loop_t *sl);
/* 每周期调用, 返回左右 PWM 供显示 */
void motor_speed_loop_run(motor_speed_loop_t *sl, float dt,
                          int16_t *out_lp, int16_t *out_rp);

/* ══════════ 航向环 (Heading Hold) ══════════ */
/* 航向 PID → 速度差 → 左右独立速度 PID。
 * 用于原地旋转保持航向, 或叠加到前进速度上提供转向。 */

typedef struct {
    bool  running;
    float target_deg;        /* 目标航向 (°) */
    float speed_diff;        /* 航向PID输出的速度差 (mm/s, 供显示) */
} motor_heading_hold_t;

void motor_heading_hold_init(motor_heading_hold_t *hh);
void motor_heading_hold_start(motor_heading_hold_t *hh, float target_deg);
void motor_heading_hold_stop(motor_heading_hold_t *hh);
void motor_heading_hold_run(motor_heading_hold_t *hh, float dt,
                            int16_t *out_lp, int16_t *out_rp);

/* ══════════ 位置环 (Position Hold) ══════════ */
/* 记录 A 点 → 计算 B=A+dist*(cosθ,sinθ) → 航向PID追B + 速度PID前进。
 * 用 Kalman 坐标导航, 编码器距离判到达。 */

typedef struct {
    bool  running;
    float target_dist_mm;    /* 目标行驶距离 (mm) */
    float cruise_mmps;       /* 巡航速度 (mm/s) */
    /* 起点 (START 时记录) */
    float start_x, start_y;  /* Kalman 坐标 A */
    float start_heading;     /* 起始航向 (rad) */
    float start_enc;         /* 起始编码器距离 (mm) */
    /* 实时输出 (供显示) */
    float dist_to_b;         /* 到 B 的 Kalman 距离 (mm) */
    float hdg_err_deg;       /* 航向偏差 (°) */
    float speed_diff;        /* 航向 PID 速度差 (mm/s) */
    float left_target;       /* 左轮目标速度 (mm/s) */
    float right_target;      /* 右轮目标速度 (mm/s) */
} motor_position_hold_t;

void motor_position_hold_init(motor_position_hold_t *ph);
void motor_position_hold_start(motor_position_hold_t *ph,
                               float dist_mm, float cruise_mmps);
void motor_position_hold_stop(motor_position_hold_t *ph);
void motor_position_hold_run(motor_position_hold_t *ph, float dt,
                             int16_t *out_lp, int16_t *out_rp);

#endif /* APP_MOTOR_H */
