/*
 * ============ app_motor_test.h =============
 * 电机辨识测试 — 确定左/右轮 + 正转方向
 *
 * 使用方法:
 *   1. 进入菜单 → Motor Test
 *   2. 观察 LCD 显示的当前测试状态
 *   3. 看哪个轮子在转、往哪个方向转
 *   4. 记录: Motor A 对应 (左/右)轮, 正 PWM = (前进/后退)
 *
 * 测试序列 (每步 2s, 自动推进):
 *   Step 1: Motor A +200 (正转) → 观察哪个轮子, 方向?
 *   Step 2: Motor A -200 (反转) → 确认方向
 *   Step 3: Motor B +200 (正转) → 观察哪个轮子, 方向?
 *   Step 4: Motor B -200 (反转) → 确认方向
 *   Step 5: 双轮 +200 (直行) → 车身前进方向?
 *   Step 6: 停止, 显示编码器累计
 */

#ifndef APP_MOTOR_TEST_H
#define APP_MOTOR_TEST_H

#include <stdint.h>
#include <stdbool.h>

/* 测试状态 */
typedef enum {
    MT_IDLE = 0,
    MT_A_FWD,        /* 电机 A 正转   */
    MT_A_REV,        /* 电机 A 反转   */
    MT_B_FWD,        /* 电机 B 正转   */
    MT_B_REV,        /* 电机 B 反转   */
    MT_BOTH_FWD,     /* 双轮正转       */
    MT_DONE          /* 测试完成       */
} motor_test_phase_t;

/* ── API ── */

/* 启动电机测试 (重置编码器, 从 Step 1 开始) */
void motor_test_start(void);

/* 停止测试 */
void motor_test_stop(void);

/* 运行测试状态机 (每 10ms 由 Control/LCD 任务调用) */
void motor_test_run(void);

/* 手动控制 (按键驱动, 用于交互式测试) */
void motor_test_manual(uint8_t motor, int16_t pwm);

/* 获取当前测试信息 (LCD 显示用) */
motor_test_phase_t motor_test_get_phase(void);
const char* motor_test_get_label(void);
int16_t motor_test_get_pwm_a(void);
int16_t motor_test_get_pwm_b(void);
uint32_t motor_test_get_elapsed(void);

#endif /* APP_MOTOR_TEST_H */
