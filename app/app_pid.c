/*
 * ============ app_pid.c =============
 * 可复用 PID 控制器实现
 */

#include "app_pid.h"
#include <math.h>   /* fabsf */
#include <string.h> /* memset */

/* ── 初始化 ── */
void pid_init(pid_t *p, float kp, float ki, float kd,
              float integral_limit, float output_limit)
{
    memset(p, 0, sizeof(pid_t));
    p->kp = kp;
    p->ki = ki;
    p->kd = kd;
    p->integral_limit = integral_limit;
    p->output_limit   = output_limit;
    p->use_d_on_measurement = true;   /* 默认 D-on-meas, 抗冲击 */
}

/* ── PID 计算 ── */
float pid_compute(pid_t *p, float setpoint, float measurement, float dt)
{
    /* 防止除零 (首次调用 dt 可能异常) */
    if (dt <= 0.0f) dt = 0.01f;

    /* ── 比例项 ── */
    float error = setpoint - measurement;
    float p_term = p->kp * error;

    /* ── 积分项 (梯形积分 + 条件抗饱和) ── */
    float new_integral = p->integral + p->ki * (error + p->prev_error) * 0.5f * dt;

    /* 钳位积分 */
    if (new_integral >  p->integral_limit) new_integral =  p->integral_limit;
    if (new_integral < -p->integral_limit) new_integral = -p->integral_limit;

    float i_term = new_integral;

    /* ── 微分项 ── */
    float d_term;
    if (p->use_d_on_measurement) {
        /* D-on-measurement: 微分测量值变化 (抗设定值冲击) */
        float d_meas = (measurement - p->prev_measurement) / dt;
        d_term = -p->kd * d_meas;
    } else {
        /* D-on-error: 经典微分 */
        float d_err = (error - p->prev_error) / dt;
        d_term = p->kd * d_err;
    }

    /* ── 合成输出 ── */
    float output = p_term + i_term + d_term;

    /* ── 输出钳位 + 条件积分冻结 ── */
    bool saturated = false;
    if (output > p->output_limit) {
        output = p->output_limit;
        saturated = true;
    } else if (output < -p->output_limit) {
        output = -p->output_limit;
        saturated = true;
    }

    /* 条件积分: 输出饱和且误差同向时冻结积分 (不更新积分器) */
    if (!saturated || (error * output <= 0.0f)) {
        p->integral = new_integral;
    }
    /* else: 积分器保持上一周期值 (冻结) */

    /* ── 保存历史 ── */
    p->prev_error       = error;
    p->prev_measurement = measurement;

    return output;
}

/* ── 复位 ── */
void pid_reset(pid_t *p)
{
    p->integral         = 0.0f;
    p->prev_error       = 0.0f;
    p->prev_measurement = 0.0f;
}

/* ── 热更新增益 ── */
void pid_set_gains(pid_t *p, float kp, float ki, float kd)
{
    p->kp = kp;
    p->ki = ki;
    p->kd = kd;
}
