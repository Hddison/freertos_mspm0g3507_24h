/*
 *  ============ hw_motor.h =============
 *  TB6612 双路电机驱动 + GMR 编码器
 *
 *  PWM:  TIMA1, PA16(PWMA)/PA17(PWMB), 40kHz
 *  方向:  PA12(BIN1)/PA13(BIN2)/PA14(AIN1)/PA15(AIN2)
 *  编码:  TIMG8(PA26/PA27), TIMG7(PA28/PA31)
 */

#ifndef HW_MOTOR_H
#define HW_MOTOR_H

#include <stdint.h>
#include <stdbool.h>

#define MOTOR_PWM_MAX   999     /* PWM period-1, 0-999 */

/* ── 电机 & 编码器配置 (由 main.c 从 Flash 加载后设置) ── */
extern int8_t  g_motor_a_dir;     /* +1=正PWM前进, -1=反转 */
extern int8_t  g_motor_b_dir;
extern int8_t  g_enc1_pol;        /* 编码器极性: +1 / -1 */
extern int8_t  g_enc2_pol;
extern bool    g_motor_a_left;    /* true = Motor A 是左轮 */

/* API */
void   Motor_init(void);
void   Motor_set(int16_t pwma, int16_t pwmb);   /* ±999, 正=前进 */
int32_t Motor_enc1(void);                        /* 编码器 1 累计脉冲 (×极性) */
int32_t Motor_enc2(void);                        /* 编码器 2 累计脉冲 (×极性) */
int32_t Motor_enc1Raw(void);                     /* 编码器 1 原始脉冲 */
int32_t Motor_enc2Raw(void);                     /* 编码器 2 原始脉冲 */
float Motor_enc1Dist(void);                      /* 编码器 1 距离 (mm) */
float Motor_enc2Dist(void);                      /* 编码器 2 距离 (mm) */
void   Motor_encReset(void);                     /* 编码器清零 */

/* 转速 (counts/10ms, TIMA0 ISR 更新) */
extern volatile int32_t g_enc1_speed;
extern volatile int32_t g_enc2_speed;

/* 单编码器速度 (mm/s), 已乘极性 */
float Motor_enc1Speed(void);
float Motor_enc2Speed(void);

#endif
