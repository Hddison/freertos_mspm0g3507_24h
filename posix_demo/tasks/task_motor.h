/*
 *  ============ task_motor.h =============
 *  电机控制任务 — PID 速度/位置闭环 (占位)
 *
 *  模式: 控制循环任务 (control loop task)
 *  - 高频率运行 (100Hz)
 *  - 可接收来自其他任务的控制命令 (队列)
 *  - 当前为占位实现, 仅维持电机停转状态
 */

#ifndef TASK_MOTOR_H
#define TASK_MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

void TaskMotor_create(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_MOTOR_H */
