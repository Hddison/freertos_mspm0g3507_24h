/*
 * ============ app_motor_test.c =============
 * 电机辨识测试实现
 */

#include "app_motor_test.h"
#include "app_config.h"
#include "hw_motor.h"
#include <string.h>

/* ── 测试参数 ── */
#define MT_STEP_DURATION_MS  2500      /* 每步持续 2.5s     */
#define MT_TEST_PWM           250      /* 测试 PWM (±250)   */
#define MT_NEUTRAL_MS         300      /* 步间暂停 300ms     */

/* ── 状态 ── */
static motor_test_phase_t g_mt_phase  = MT_IDLE;
static uint32_t           g_mt_t0     = 0;    /* 当前步起始时间 */
static uint32_t           g_mt_pause  = 0;    /* 暂停起始时间   */
static bool               g_mt_in_pause = false;
static int16_t            g_mt_pwm_a  = 0;
static int16_t            g_mt_pwm_b  = 0;

/* 编码器初始值 (用于显示增量) */
static int32_t            g_mt_enc1_start = 0;
static int32_t            g_mt_enc2_start = 0;

/* ══════════ 阶段名称 ══════════ */

static const char* phase_labels[] = {
    "IDLE",
    "Motor A +PWM (FWD?)",   /* 正转测试 — 观察方向和轮子 */
    "Motor A -PWM (REV?)",   /* 反转测试                     */
    "Motor B +PWM (FWD?)",
    "Motor B -PWM (REV?)",
    "Both +PWM (Straight?)", /* 双轮前进 — 确认方向          */
    "TEST DONE - Check ENC", /* 完成, 显示编码器              */
};

const char* motor_test_get_label(void)
{
    int idx = (int)g_mt_phase;
    if (idx < 0 || idx > (int)MT_DONE) return "???";
    return phase_labels[idx];
}

motor_test_phase_t motor_test_get_phase(void) { return g_mt_phase; }
int16_t motor_test_get_pwm_a(void)            { return g_mt_pwm_a; }
int16_t motor_test_get_pwm_b(void)            { return g_mt_pwm_b; }
uint32_t motor_test_get_elapsed(void)         { return g_mt_t0; }

/* ══════════ 启动 ─═════════ */

void motor_test_start(void)
{
    g_mt_phase     = MT_A_FWD;
    g_mt_t0        = 0;   /* 由首次 run 设置 */
    g_mt_in_pause  = true; /* 先暂停, 让用户看清状态 */
    g_mt_pause     = 0;
    g_mt_pwm_a     = 0;
    g_mt_pwm_b     = 0;
    Motor_set(0, 0);

    /* 记录编码器起始值 */
    g_mt_enc1_start = Motor_enc1();
    g_mt_enc2_start = Motor_enc2();
}

/* ══════════ 停止 ─═════════ */

void motor_test_stop(void)
{
    g_mt_phase = MT_IDLE;
    g_mt_pwm_a = 0;
    g_mt_pwm_b = 0;
    Motor_set(0, 0);
}

/* ══════════ 状态机 (由 task_lcd 每 50ms 调用) ══════════ */

void motor_test_run(void)
{
    if (g_mt_phase == MT_IDLE || g_mt_phase == MT_DONE) return;

    uint32_t phase_elapsed;

    /* ── 暂停阶段处理 ── */
    if (g_mt_in_pause) {
        if (g_mt_pause == 0) {
            g_mt_pause = g_mt_t0;  /* t0 由外部设置, 此处用 phase 起始 */
            Motor_set(0, 0);
            g_mt_pwm_a = 0;
            g_mt_pwm_b = 0;
        }
        /* 用简单计数器代替 tick (LCD 任务 50ms 周期提供) */
        static uint16_t pause_cnt = 0;
        pause_cnt++;
        if (pause_cnt >= (MT_NEUTRAL_MS / 50)) {
            pause_cnt = 0;
            g_mt_in_pause = false;
            /* 进入下一步 */
        }
        return;
    }

    /* ── 根据阶段设置 PWM ── */
    switch (g_mt_phase) {
    case MT_A_FWD:    g_mt_pwm_a =  MT_TEST_PWM; g_mt_pwm_b = 0;           break;
    case MT_A_REV:    g_mt_pwm_a = -MT_TEST_PWM; g_mt_pwm_b = 0;           break;
    case MT_B_FWD:    g_mt_pwm_a = 0;             g_mt_pwm_b =  MT_TEST_PWM; break;
    case MT_B_REV:    g_mt_pwm_a = 0;             g_mt_pwm_b = -MT_TEST_PWM; break;
    case MT_BOTH_FWD: g_mt_pwm_a =  MT_TEST_PWM; g_mt_pwm_b =  MT_TEST_PWM; break;
    default: break;
    }

    Motor_set(g_mt_pwm_a, g_mt_pwm_b);

    /* ── 计时 & 阶段推进 ── */
    /* 使用静态 tick 计数器 (由 50ms LCD 周期驱动) */
    static uint16_t tick_50ms = 0;
    tick_50ms++;
    phase_elapsed = (uint32_t)tick_50ms * 50;

    if (phase_elapsed >= MT_STEP_DURATION_MS) {
        tick_50ms = 0;
        /* 停止电机, 进入暂停 */
        Motor_set(0, 0);
        g_mt_pwm_a = 0;
        g_mt_pwm_b = 0;

        /* 推进到下一阶段 */
        switch (g_mt_phase) {
        case MT_A_FWD:    g_mt_phase = MT_A_REV;    break;
        case MT_A_REV:    g_mt_phase = MT_B_FWD;    break;
        case MT_B_FWD:    g_mt_phase = MT_B_REV;    break;
        case MT_B_REV:    g_mt_phase = MT_BOTH_FWD; break;
        case MT_BOTH_FWD: g_mt_phase = MT_DONE;     break;
        default: break;
        }

        if (g_mt_phase != MT_DONE) {
            g_mt_in_pause = true;
            g_mt_pause = 0;
        } else {
            /* 测试完成 — 显示编码器累计 */
            g_mt_pwm_a = 0;
            g_mt_pwm_b = 0;
        }
    }
}

/* ══════════ 手动控制 ══════════ */

void motor_test_manual(uint8_t motor, int16_t pwm)
{
    if (motor == 0) {
        /* Motor A only */
        g_mt_pwm_a = pwm;
        g_mt_pwm_b = 0;
        g_mt_phase = (pwm >= 0) ? MT_A_FWD : MT_A_REV;
    } else if (motor == 1) {
        /* Motor B only */
        g_mt_pwm_a = 0;
        g_mt_pwm_b = pwm;
        g_mt_phase = (pwm >= 0) ? MT_B_FWD : MT_B_REV;
    } else {
        /* Both */
        g_mt_pwm_a = pwm;
        g_mt_pwm_b = pwm;
        g_mt_phase = MT_BOTH_FWD;
    }

    Motor_set(g_mt_pwm_a, g_mt_pwm_b);
}
