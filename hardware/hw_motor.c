/*
 *  ============ hw_motor.c =============
 *  TB6612 双路电机驱动 + GMR 编码器 (GPIO 中断解码)
 */

#include "hw_motor.h"

#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_timera.h>
#include <ti/driverlib/dl_gpio.h>

/* ══════ 编码器累计脉冲 (ISR 中更新) ══════ */
static volatile int32_t g_enc1;
static volatile int32_t g_enc2;

/* ── 电机 & 编码器配置 (由 main.c 从 Flash 加载后设置) ── */
int8_t  g_motor_a_dir  = 1;    /* Motor A: +1=正PWM前进, -1=反转 */
int8_t  g_motor_b_dir  = -1;    /* Motor B: +1=正PWM前进, -1=反转 */
int8_t  g_enc1_pol     = 1;    /* Encoder 1 极性 */
int8_t  g_enc2_pol     = 1;    /* Encoder 2 极性 */
bool    g_motor_a_left = false; /* Motor A = 左轮 */

#define ENC1_MASK   (GPIO_ENC_PIN_E1A_PIN | GPIO_ENC_PIN_E1B_PIN)  /* E1A=PA27 + E1B=PA26 */
#define ENC2_MASK   (GPIO_ENC_PIN_E2A_PIN | GPIO_ENC_PIN_E2B_PIN)  /* E2A=PA28 + E2B=PA31 */

/* ══════ 初始化 ══════ */

void Motor_init(void)
{
    /* 方向引脚: 初始 LOW, 输出 */
    DL_GPIO_clearPins(GPIO_MOTOR_PORT,
        GPIO_MOTOR_BIN1_PIN | GPIO_MOTOR_BIN2_PIN |
        GPIO_MOTOR_AIN1_PIN | GPIO_MOTOR_AIN2_PIN);

    /* PWM 初始值 = 0 (电机停转) */
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, 0, DL_TIMER_CC_0_INDEX);
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, 0, DL_TIMER_CC_1_INDEX);
    DL_TimerA_startCounter(PWM_MOTOR_INST);

    /* 启动 10ms 定时器用于编码器转速采样 (TIMA0_IRQHandler 已在本文件定义) */
    NVIC_EnableIRQ(TIMER_10ms_INST_INT_IRQN);
    DL_TimerA_startCounter(TIMER_10ms_INST);
}

/* ══════ 设置 PWM (带符号: 正=前进, 负=后退) ══════ */

void Motor_set(int16_t pwma, int16_t pwmb)
{
    /* 应用方向修正 (正PWM=前进, 修正后符号决定 TB6612 IN1/IN2) */
    int16_t a = pwma * g_motor_a_dir;
    int16_t b = pwmb * g_motor_b_dir;

    if (a >= 0) {
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_AIN1_PIN);
        DL_GPIO_setPins(GPIO_MOTOR_PORT,   GPIO_MOTOR_AIN2_PIN);
    } else {
        DL_GPIO_setPins(GPIO_MOTOR_PORT,   GPIO_MOTOR_AIN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_AIN2_PIN);
        a = -a;
    }
    if (a > MOTOR_PWM_MAX) a = MOTOR_PWM_MAX;
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, (uint16_t)a, DL_TIMER_CC_1_INDEX);

    if (b >= 0) {
        DL_GPIO_setPins(GPIO_MOTOR_PORT,   GPIO_MOTOR_BIN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_BIN2_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_BIN1_PIN);
        DL_GPIO_setPins(GPIO_MOTOR_PORT,   GPIO_MOTOR_BIN2_PIN);
        b = -b;
    }
    if (b > MOTOR_PWM_MAX) b = MOTOR_PWM_MAX;
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, (uint16_t)b, DL_TIMER_CC_0_INDEX);
}

/* ══════ 读取编码器累计脉冲 ══════ */

int32_t Motor_enc1(void) { return g_enc1 * g_enc1_pol; }
int32_t Motor_enc2(void) { return g_enc2 * g_enc2_pol; }
int32_t Motor_enc1Raw(void) { return g_enc1; }
int32_t Motor_enc2Raw(void) { return g_enc2; }

/* 距离换算: 500PPR × 减速比 1:20 × 轮径 48mm × 2x 上升沿解码 */
#define DIST_PER_COUNT  (3.14159265f * 48.0f / (500.0f * 20.0f * 2.0f))

float Motor_enc1Dist(void) { return (float)Motor_enc1() * DIST_PER_COUNT; }
float Motor_enc2Dist(void) { return (float)Motor_enc2() * DIST_PER_COUNT; }
void  Motor_encReset(void) { g_enc1 = 0; g_enc2 = 0; }

/* 单编码器速度 (mm/s), 已乘极性 */
float Motor_enc1Speed(void) {
    return (float)(g_enc1_speed * g_enc1_pol) * DIST_PER_COUNT * 100.0f;
}
float Motor_enc2Speed(void) {
    return (float)(g_enc2_speed * g_enc2_pol) * DIST_PER_COUNT * 100.0f;
}

/* ══════ 10ms 定时器 ISR: 编码器转速采样 ══════ */

volatile int32_t g_enc1_speed;
volatile int32_t g_enc2_speed;

void TIMA0_IRQHandler(void)
{
    static int32_t prev1, prev2;
    static bool first = true;

    DL_TimerA_clearInterruptStatus(TIMER_10ms_INST,
        DL_TIMER_INTERRUPT_ZERO_EVENT);

    int32_t e1 = g_enc1;
    int32_t e2 = g_enc2;

    if (!first) {
        g_enc1_speed = e1 - prev1;
        g_enc2_speed = e2 - prev2;
    }
    first = false;
    prev1 = e1;
    prev2 = e2;
}

/* ══════ 编码器 ISR (GROUP1: PA26-31 双边沿) ══════ */

void GROUP1_IRQHandler(void)
{
    uint32_t ris = DL_GPIO_getEnabledInterruptStatus(GPIOA, ENC1_MASK | ENC2_MASK);

    if (ris & GPIO_ENC_PIN_E1A_PIN) {  /* PA27 = Enc1 A↑ */
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_ENC_PIN_E1A_PIN);
        if (DL_GPIO_readPins(GPIOA, GPIO_ENC_PIN_E1B_PIN))
            g_enc1--;  /* A↑, B=1 → 反转 */
        else
            g_enc1++;  /* A↑, B=0 → 正转 */
    }
    if (ris & GPIO_ENC_PIN_E1B_PIN) {  /* PA26 = Enc1 B↑ */
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_ENC_PIN_E1B_PIN);
        if (DL_GPIO_readPins(GPIOA, GPIO_ENC_PIN_E1A_PIN))
            g_enc1++;  /* B↑, A=1 → 正转 */
        else
            g_enc1--;  /* B↑, A=0 → 反转 */
    }
    if (ris & GPIO_ENC_PIN_E2A_PIN) {  /* PA28 = Enc2 A↑ (左轮, 反向) */
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_ENC_PIN_E2A_PIN);
        if (DL_GPIO_readPins(GPIOA, GPIO_ENC_PIN_E2B_PIN))
            g_enc2++;  /* A↑, B=1 → 正转 */
        else
            g_enc2--;  /* A↑, B=0 → 反转 */
    }
    if (ris & GPIO_ENC_PIN_E2B_PIN) {  /* PA31 = Enc2 B↑ (左轮, 反向) */
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_ENC_PIN_E2B_PIN);
        if (DL_GPIO_readPins(GPIOA, GPIO_ENC_PIN_E2A_PIN))
            g_enc2--;  /* B↑, A=1 → 反转 */
        else
            g_enc2++;  /* B↑, A=0 → 正转 */
    }
}
