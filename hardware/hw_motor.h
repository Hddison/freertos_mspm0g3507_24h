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

#define MOTOR_PWM_MAX   1000    /* PWM period-1, 0-1000 */

/* API */
void Motor_Init(void);
void Motor_Set(int16_t pwma, int16_t pwmb);   /* ±1000, 正=前进 */
int32_t Motor_Enc1(void);                      /* 编码器 1 累计脉冲 */
int32_t Motor_Enc2(void);                      /* 编码器 2 累计脉冲 */
float Motor_Enc1Dist(void);                    /* 编码器 1 距离 (mm) */
float Motor_Enc2Dist(void);                    /* 编码器 2 距离 (mm) */
void Motor_EncReset(void);                     /* 编码器清零 */

#endif
