/*
 * ============ app_motor.c ============
 * 电机控制环实现 — 速度环 / 航向环 / 位置环
 */

#include "app_motor.h"
#include "app_config.h"
#include "app_pid.h"
#include "hw_motor.h"
#include "task_button.h"  /* sensor_data_t volatile g_sensor_data */

#include <math.h>   /* sqrtf, sinf, cosf, atan2f, fabsf */
#include <string.h> /* memset */

/* ══════════ 外部 PID 实例 (app_control.c) ══════════ */
extern pid_t g_pid_speed_l;
extern pid_t g_pid_speed_r;
extern pid_t g_pid_heading;

/* ══════════ 外部 Kalman 状态 (task_sensor.c) ══════════ */
extern volatile float g_kf_x, g_kf_y, g_kf_theta;

/* ══════════ 角度归一化 ══════════ */
static float wrap_angle(float a)
{
    while (a >  3.14159265359f) a -= 2.0f * 3.14159265359f;
    while (a < -3.14159265359f) a += 2.0f * 3.14159265359f;
    return a;
}

/* ══════════════════════════════════════════════════════════════
 *  底层: 左右独立速度环
 * ══════════════════════════════════════════════════════════════ */

void motor_speed_apply(float left_target, float right_target, float dt,
                       int16_t *out_lp, int16_t *out_rp)
{
    /* Enc2=左轮, Enc1=右轮 (Motor A=右, g_motor_a_left=false) */
    float enc_l = Motor_enc2Speed();
    float enc_r = Motor_enc1Speed();

    float lpwm = pid_compute(&g_pid_speed_l, left_target,  enc_l, dt);
    float rpwm = pid_compute(&g_pid_speed_r, right_target, enc_r, dt);
    if (lpwm >  (float)PWM_MAX) lpwm =  (float)PWM_MAX;
    if (lpwm < -(float)PWM_MAX) lpwm = -(float)PWM_MAX;
    if (rpwm >  (float)PWM_MAX) rpwm =  (float)PWM_MAX;
    if (rpwm < -(float)PWM_MAX) rpwm = -(float)PWM_MAX;
    int16_t lp = (int16_t)lpwm;
    int16_t rp = (int16_t)rpwm;

    Motor_set(rp, lp);  /* A=右, B=左(dir=-1) */

    if (out_lp) *out_lp = lp;
    if (out_rp) *out_rp = rp;
}

/* ══════════════════════════════════════════════════════════════
 *  速度环 (Speed Loop)
 * ══════════════════════════════════════════════════════════════ */

void motor_speed_loop_init(motor_speed_loop_t *sl)
{
    memset(sl, 0, sizeof(*sl));
}

void motor_speed_loop_start(motor_speed_loop_t *sl, float target_mmps)
{
    sl->target_mmps = target_mmps;
    pid_reset(&g_pid_speed_l);
    pid_reset(&g_pid_speed_r);
    sl->running = true;
}

void motor_speed_loop_stop(motor_speed_loop_t *sl)
{
    sl->running = false;
    Motor_set(0, 0);
}

void motor_speed_loop_run(motor_speed_loop_t *sl, float dt,
                          int16_t *out_lp, int16_t *out_rp)
{
    if (!sl->running) return;
    motor_speed_apply(sl->target_mmps, sl->target_mmps, dt, out_lp, out_rp);
}

/* ══════════════════════════════════════════════════════════════
 *  航向环 (Heading Hold)
 * ══════════════════════════════════════════════════════════════ */

void motor_heading_hold_init(motor_heading_hold_t *hh)
{
    memset(hh, 0, sizeof(*hh));
}

void motor_heading_hold_start(motor_heading_hold_t *hh, float target_deg)
{
    hh->target_deg = target_deg;
    hh->speed_diff = 0.0f;
    pid_reset(&g_pid_heading);
    pid_reset(&g_pid_speed_l);
    pid_reset(&g_pid_speed_r);
    hh->running = true;
}

void motor_heading_hold_stop(motor_heading_hold_t *hh)
{
    hh->running = false;
    Motor_set(0, 0);
}

void motor_heading_hold_run(motor_heading_hold_t *hh, float dt,
                            int16_t *out_lp, int16_t *out_rp)
{
    if (!hh->running) return;

    /* 当前航向 (°) — 和 Heading Hold 显示同源 */
    extern volatile sensor_data_t g_sensor_data;
    float cur_yaw = g_sensor_data.total_yaw;

    /* 航向误差 (°) */
    float error = hh->target_deg - cur_yaw;
    while (error >  180.0f) error -= 360.0f;
    while (error < -180.0f) error += 360.0f;

    /* 航向 PID → 速度差 (mm/s) */
    hh->speed_diff = pid_compute(&g_pid_heading, 0.0f, error, dt);
    if (-2.0f < error && error < 2.0f) { hh->speed_diff = 0.0f; pid_reset(&g_pid_heading); }
    if (hh->speed_diff >  500.0f) hh->speed_diff =  500.0f;
    if (hh->speed_diff < -500.0f) hh->speed_diff = -500.0f;

    /* 左轮=+diff, 右轮=-diff */
    motor_speed_apply(+hh->speed_diff, -hh->speed_diff, dt, out_lp, out_rp);
}

/* ══════════════════════════════════════════════════════════════
 *  位置环 (Position Hold)
 * ══════════════════════════════════════════════════════════════ */

void motor_position_hold_init(motor_position_hold_t *ph)
{
    memset(ph, 0, sizeof(*ph));
    ph->target_dist_mm = 1000.0f;
    ph->cruise_mmps    = 300.0f;
}

void motor_position_hold_start(motor_position_hold_t *ph,
                               float dist_mm, float cruise_mmps)
{
    ph->target_dist_mm = dist_mm;
    ph->cruise_mmps    = cruise_mmps;
    /* 记录 A 点 */
    ph->start_x       = g_kf_x;
    ph->start_y       = g_kf_y;
    ph->start_heading = g_kf_theta;
    ph->start_enc     = (Motor_enc1Dist() + Motor_enc2Dist()) * 0.5f;
    /* 重置 PID */
    pid_reset(&g_pid_heading);
    pid_reset(&g_pid_speed_l);
    pid_reset(&g_pid_speed_r);
    ph->running = true;
}

void motor_position_hold_stop(motor_position_hold_t *ph)
{
    ph->running = false;
    Motor_set(0, 0);
}

void motor_position_hold_run(motor_position_hold_t *ph, float dt,
                             int16_t *out_lp, int16_t *out_rp)
{
    if (!ph->running) return;

    /* B = A + dist * (cos θ₀, sin θ₀) */
    float bx = ph->start_x + ph->target_dist_mm * cosf(ph->start_heading);
    float by = ph->start_y + ph->target_dist_mm * sinf(ph->start_heading);

    /* Kalman 距离到 B */
    float dx = bx - g_kf_x;
    float dy = by - g_kf_y;
    ph->dist_to_b = sqrtf(dx*dx + dy*dy);

    /* 目标航向 & 误差 */
    float target_hdg = atan2f(dy, dx);
    float hdg_err = wrap_angle(target_hdg - g_kf_theta);
    ph->hdg_err_deg = hdg_err * 57.2958f;
    if (hdg_err <  0.035f && hdg_err > -0.035f) hdg_err = 0.0f;

    /* 编码器行驶距离 → 速度曲线 */
    float enc_avg = (Motor_enc1Dist() + Motor_enc2Dist()) * 0.5f;
    float traveled = enc_avg - ph->start_enc;
    float remain   = ph->target_dist_mm - traveled;

    float target_spd;
    if (remain > 200.0f)       target_spd = ph->cruise_mmps;
    else if (remain < 20.0f)   target_spd = 0.0f;
    else                       target_spd = ph->cruise_mmps * remain / 200.0f;

    /* 航向 PID → 速度差 */
    ph->speed_diff = pid_compute(&g_pid_heading, 0.0f, hdg_err, dt);
    if (ph->speed_diff >  500.0f) ph->speed_diff =  500.0f;
    if (ph->speed_diff < -500.0f) ph->speed_diff = -500.0f;

    /* 左右目标速度 */
    ph->left_target  = target_spd + ph->speed_diff;
    ph->right_target = target_spd - ph->speed_diff;

    motor_speed_apply(ph->left_target, ph->right_target, dt, out_lp, out_rp);

    /* 到达判定 */
    if (remain <= 0.0f || ph->dist_to_b < 30.0f) {
        motor_position_hold_stop(ph);
    }
}
