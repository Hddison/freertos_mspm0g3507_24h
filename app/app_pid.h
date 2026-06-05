/*
 * ============ app_pid.h =============
 * 可复用 PID 控制器 (结构体实例化, 抗积分饱和)
 *
 * 特性:
 *   - 梯形积分 (Tustin 近似)
 *   - 条件积分抗饱和 (输出钳位且同向时冻结积分)
 *   - 双模式微分: 误差微分 / 测量值微分 (抗设定值冲击)
 *   - 积分钳位 + 输出钳位
 */

#ifndef APP_PID_H
#define APP_PID_H

#include <stdbool.h>

typedef struct {
    float kp, ki, kd;              /* 增益                           */
    float integral;                /* 积分累加器                     */
    float prev_error;              /* 上一周期误差                   */
    float prev_measurement;        /* 上一周期测量值 (D-on-meas用)   */
    float integral_limit;          /* 积分钳位 (±)                   */
    float output_limit;            /* 输出钳位 (±)                   */
    bool  use_d_on_measurement;    /* true=微分作用于测量值           */
} pid_t;

/* ── 初始化 PID 实例 ── */
void pid_init(pid_t *p, float kp, float ki, float kd,
              float integral_limit, float output_limit);

/* ── 执行一次 PID 计算 ──
 *   setpoint:   目标值
 *   measurement: 当前测量值
 *   dt:          距离上次调用时间 (秒)
 *   返回:        钳位后的控制输出
 */
float pid_compute(pid_t *p, float setpoint, float measurement, float dt);

/* ── 复位积分和误差历史 (切换模式/目标值时调用) ── */
void pid_reset(pid_t *p);

/* ── 运行时热更新增益 (LCD 菜单调用) ── */
void pid_set_gains(pid_t *p, float kp, float ki, float kd);

#endif /* APP_PID_H */
