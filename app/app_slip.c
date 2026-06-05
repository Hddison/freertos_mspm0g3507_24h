/*
 * ============ app_slip.c =============
 * 打滑检测实现
 */

#include "app_slip.h"
#include "app_config.h"
#include <math.h>    /* fabsf */

/* ── 状态 ── */
static bool    g_slip_active;
static float   g_prev_enc_speed;    /* 上一周期编码器速度 (mm/s) */
static bool    g_first_sample;
static uint8_t g_slip_count;        /* 连续打滑计数 */
static uint8_t g_normal_count;      /* 连续正常计数 (滞后退出) */

#define SLIP_ENTER_COUNT  3          /* 连续检测到打滑 ≥3 次 → 确认打滑 */
#define SLIP_EXIT_COUNT   5          /* 连续正常 ≥5 次 → 退出打滑状态 */

/* ══════════ 初始化 ══════════ */

void slip_init(void)
{
    g_slip_active    = false;
    g_prev_enc_speed = 0.0f;
    g_first_sample   = true;
    g_slip_count     = 0;
    g_normal_count   = 0;
}

/* ══════════ 检测 ══════════ */

bool slip_detect(float ax_forward, float dv_enc)
{
    /* 首个采样: 初始化 prev, 不检测 */
    if (g_first_sample) {
        g_prev_enc_speed = 0.0f;
        g_first_sample   = false;
        return false;
    }

    /* 加速度差异 */
    float diff = fabsf(ax_forward - dv_enc);

    if (diff > SLIP_ACCEL_THRESH) {
        g_slip_count++;
        g_normal_count = 0;
        if (g_slip_count >= SLIP_ENTER_COUNT) {
            g_slip_active = true;
        }
    } else {
        g_normal_count++;
        if (g_normal_count >= SLIP_EXIT_COUNT) {
            g_slip_count   = 0;
            g_slip_active  = false;
        }
    }

    return g_slip_active;
}

/* ══════════ 查询 ══════════ */

bool slip_is_active(void)
{
    return g_slip_active;
}

void slip_reset(void)
{
    g_slip_active  = false;
    g_slip_count   = 0;
    g_normal_count = 0;
}
