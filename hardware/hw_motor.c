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

#define ENC1_MASK   (GPIO_CAP_MOTOR1_C1_PIN | GPIO_CAP_MOTOR1_C0_PIN)  /* PA26+PA27 */
#define ENC2_MASK   (GPIO_CAP_MOTOR2_C0_PIN | GPIO_CAP_MOTOR2_C1_PIN)  /* PA28+PA31 */

/* ══════ 初始化 ══════ */

void Motor_Init(void)
{
    /* 方向引脚: 初始 LOW, 输出 */
    DL_GPIO_clearPins(GPIO_MOTOR_PORT,
        GPIO_MOTOR_BIN1_PIN | GPIO_MOTOR_BIN2_PIN |
        GPIO_MOTOR_AIN1_PIN | GPIO_MOTOR_AIN2_PIN);

    /* PWM 初始值 = 0 (电机停转) */
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, 0, DL_TIMER_CC_0_INDEX);
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, 0, DL_TIMER_CC_1_INDEX);
    DL_TimerA_startCounter(PWM_MOTOR_INST);

    /* 编码器引脚: GPIO 输入 + 上拉 + 双边沿中断 */
    DL_GPIO_initDigitalInputFeatures(
        GPIO_CAP_MOTOR1_C0_IOMUX, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(
        GPIO_CAP_MOTOR1_C1_IOMUX, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(
        GPIO_CAP_MOTOR2_C0_IOMUX, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(
        GPIO_CAP_MOTOR2_C1_IOMUX, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);

    /* 配置双边沿触发 (PA26-31 在 UPPER polarity 寄存器) */
    /* 仅上升沿 (2x 解码): 方向判断简单可靠 */
    DL_GPIO_setUpperPinsPolarity(GPIOA,
        DL_GPIO_PIN_26_EDGE_RISE | DL_GPIO_PIN_27_EDGE_RISE |
        DL_GPIO_PIN_28_EDGE_RISE | DL_GPIO_PIN_31_EDGE_RISE);

    DL_GPIO_enableInterrupt(GPIOA, ENC1_MASK | ENC2_MASK);
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
}

/* ══════ 设置 PWM (带符号: 正=前进, 负=后退) ══════ */

void Motor_Set(int16_t pwma, int16_t pwmb)
{
    if (pwma >= 0) {
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_AIN1_PIN);
        DL_GPIO_setPins(GPIO_MOTOR_PORT,   GPIO_MOTOR_AIN2_PIN);
    } else {
        DL_GPIO_setPins(GPIO_MOTOR_PORT,   GPIO_MOTOR_AIN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_AIN2_PIN);
        pwma = -pwma;
    }
    if (pwma > MOTOR_PWM_MAX) pwma = MOTOR_PWM_MAX;
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, pwma, DL_TIMER_CC_1_INDEX);

    if (pwmb >= 0) {
        DL_GPIO_setPins(GPIO_MOTOR_PORT,   GPIO_MOTOR_BIN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_BIN2_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, GPIO_MOTOR_BIN1_PIN);
        DL_GPIO_setPins(GPIO_MOTOR_PORT,   GPIO_MOTOR_BIN2_PIN);
        pwmb = -pwmb;
    }
    if (pwmb > MOTOR_PWM_MAX) pwmb = MOTOR_PWM_MAX;
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, pwmb, DL_TIMER_CC_0_INDEX);
}

/* ══════ 读取编码器累计脉冲 ══════ */

int32_t Motor_Enc1(void) { return g_enc1; }
int32_t Motor_Enc2(void) { return g_enc2; }

/* 距离换算: 500PPR × 减速比 1:20 × 轮径 48mm × 2x 上升沿解码 */
#define DIST_PER_COUNT  (3.14159265f * 48.0f / (500.0f * 20.0f * 2.0f))

float Motor_Enc1Dist(void) { return (float)g_enc1 * DIST_PER_COUNT; }
float Motor_Enc2Dist(void) { return (float)g_enc2 * DIST_PER_COUNT; }
void  Motor_EncReset(void) { g_enc1 = 0; g_enc2 = 0; }

/* ══════ GPIO 编码器 ISR (GROUP1: PA26-31 双边沿) ══════ */

void GROUP1_IRQHandler(void)
{
    uint32_t ris = DL_GPIO_getEnabledInterruptStatus(GPIOA, ENC1_MASK | ENC2_MASK);

    if (ris & GPIO_CAP_MOTOR1_C1_PIN) {  /* PA27 = Enc1 A↑ */
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_CAP_MOTOR1_C1_PIN);
        if (DL_GPIO_readPins(GPIOA, GPIO_CAP_MOTOR1_C0_PIN))
            g_enc1--;  /* A↑, B=1 → 反转 */
        else
            g_enc1++;  /* A↑, B=0 → 正转 */
    }
    if (ris & GPIO_CAP_MOTOR1_C0_PIN) {  /* PA26 = Enc1 B↑ */
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_CAP_MOTOR1_C0_PIN);
        if (DL_GPIO_readPins(GPIOA, GPIO_CAP_MOTOR1_C1_PIN))
            g_enc1++;  /* B↑, A=1 → 正转 */
        else
            g_enc1--;  /* B↑, A=0 → 反转 */
    }
    if (ris & GPIO_CAP_MOTOR2_C0_PIN) {  /* PA28 = Enc2 A↑ (左轮, 反向) */
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_CAP_MOTOR2_C0_PIN);
        if (DL_GPIO_readPins(GPIOA, GPIO_CAP_MOTOR2_C1_PIN))
            g_enc2++;  /* A↑, B=1 → 正转 */
        else
            g_enc2--;  /* A↑, B=0 → 反转 */
    }
    if (ris & GPIO_CAP_MOTOR2_C1_PIN) {  /* PA31 = Enc2 B↑ (左轮, 反向) */
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_CAP_MOTOR2_C1_PIN);
        if (DL_GPIO_readPins(GPIOA, GPIO_CAP_MOTOR2_C0_PIN))
            g_enc2--;  /* B↑, A=1 → 反转 */
        else
            g_enc2++;  /* B↑, A=0 → 正转 */
    }
}
