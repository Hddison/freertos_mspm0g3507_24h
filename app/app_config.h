/*
 * ============ app_config.h =============
 * 小车服务层全局常量 — 竞赛任务 / PID 默认值 / 赛道几何
 *
 * 所有可调参数在此集中定义, 部分可通过 LCD 菜单 + Flash 持久化修改。
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

/* ═══════════════════════════ 系统参数 ═══════════════════════════ */

/* FreeRTOS 任务优先级 (0=idle, configMAX_PRIORITIES=10) */
#define TASK_PRIO_BUTTON    3
#define TASK_PRIO_LCD       4
#define TASK_PRIO_CONTROL   5
#define TASK_PRIO_SENSOR    6

/* 任务栈大小 (words, 4-byte each) */
#define STACK_BUTTON        128
#define STACK_LCD           1024
#define STACK_CONTROL       768
#define STACK_SENSOR        640

/* 任务周期 (ms) */
#define PERIOD_SENSOR_MS    10      /* 100 Hz */
#define PERIOD_CONTROL_MS   10      /* 100 Hz */
#define PERIOD_LCD_MS       50      /*  20 Hz */
#define PERIOD_BUTTON_MS    20      /*  50 Hz */

/* 队列深度 */
#define BUTTON_QUEUE_LEN    8
#define SENSOR_QUEUE_LEN    1       /* mailbox (xQueueOverwrite) */
#define CMD_QUEUE_LEN       4

/* ═══════════════════════ 物理常量 ═══════════════════════ */

/* 编码器 & 轮子 */
#define ENCODER_PPR         500U    /* 编码器脉冲/转                */
#define GEARBOX_RATIO       20.0f   /* 减速比 1:20                  */
#define WHEEL_DIAMETER_MM   48.0f   /* 轮径 (mm)                   */
#define WHEEL_BASE_MM       132.0f  /* 轮距 (左右轮中心间距, mm)    */

/* 距离系数: 每编码器计数值对应的行进距离 (mm/count) */
/* DIST = π·D / (PPR × 减速比 × 2x解码) */
#define DIST_PER_COUNT  (3.14159265359f * WHEEL_DIAMETER_MM / \
                        (ENCODER_PPR * GEARBOX_RATIO * 2.0f))

/* PWM 范围 */
#define PWM_MAX            999     /* TIMA1 周期-1 (0-999)          */
#define PWM_DEAD_ZONE       15     /* 死区: |pwm| < 此值设为 0      */

/* ═══════════════════════ 赛道几何 (单位: mm, rad) ═══════════════════════ */

/*
 *  赛道坐标系: 原点中心, X轴向东(右), Y轴向北(上)
 *
 *        A(-500,400) ─── 1000mm 直线 ─── B(500,400)
 *           |                               |
 *       LEFT ARC                        RIGHT ARC
 *    (x+500)²+y²=400²              (x-500)²+y²=400²
 *    圆心(-500,0) R=400            圆心(500,0) R=400
 *           |                               |
 *        D(-500,-400) ─── 1000mm 直线 ─── C(500,-400)
 */

#define TRACK_LEFT_CX       -500.0f  /* 左弧圆心 X (mm)            */
#define TRACK_LEFT_CY         0.0f   /* 左弧圆心 Y (mm)            */
#define TRACK_RIGHT_CX       500.0f  /* 右弧圆心 X (mm)            */
#define TRACK_RIGHT_CY        0.0f   /* 右弧圆心 Y (mm)            */
#define TRACK_ARC_RADIUS     400.0f  /* 弧半径 (mm)                */

/* 顶点坐标 (mm) */
#define VERTEX_A_X  -500.0f
#define VERTEX_A_Y   400.0f
#define VERTEX_B_X   500.0f
#define VERTEX_B_Y   400.0f
#define VERTEX_C_X   500.0f
#define VERTEX_C_Y  -400.0f
#define VERTEX_D_X  -500.0f
#define VERTEX_D_Y  -400.0f

/* 顶点到达判定距离 (mm) */
#define VERTEX_ARRIVAL_DIST  30.0f

/* 顶点停车时间 (ms) */
#define VERTEX_PAUSE_MS      800U

/* ═══════════════════════ 控制参数默认值 ═══════════════════════ */

/* 速度 PID (内环) */
#define DEFAULT_SPEED_KP     1.56f
#define DEFAULT_SPEED_KI     0.57f
#define DEFAULT_SPEED_KD     0.2f
#define DEFAULT_SPEED_I_LIM  500.0f  /* 积分限幅 (±PWM) */

/* 左轮速度 PID 出厂默认值 */
#define DEFAULT_SPEED_L_KP   1.00f
#define DEFAULT_SPEED_L_KI   1.00f
#define DEFAULT_SPEED_L_KD   0.00f

/* 右轮速度 PID 出厂默认值 */
#define DEFAULT_SPEED_R_KP   1.00f
#define DEFAULT_SPEED_R_KI   1.00f
#define DEFAULT_SPEED_R_KD   0.00f

/* 位置 PID (外环) — 输出 target_speed */
#define DEFAULT_POS_KP       0.05f
#define DEFAULT_POS_I_LIM    300.0f  /* target_speed 限幅 (mm/s)   */

/* 转向 PID (弧线循迹, 灰度质心→curvature) */
#define DEFAULT_STEER_KP     15.0f
#define DEFAULT_STEER_KD     3.0f
#define DEFAULT_STEER_I_LIM  400.0f

/* 航向 PID (heading hold 辅助) */
#define DEFAULT_HEADING_KP   5.20f
#define DEFAULT_HEADING_KI   25.55f
#define DEFAULT_HEADING_KD   2.22f
#define DEFAULT_HEADING_I_LIM 400.0f

/* 曲率→差速增益 (steer_pwm = curvature * v * GAIN) */
#define CURVATURE_GAIN       0.05f

/* 默认目标速度 (mm/s) */
#define DEFAULT_TARGET_SPEED 300.0f

/* 速度曲线: 距目标此距离开始减速 (mm) */
#define SPEED_RAMP_DIST      200.0f

/* 最低巡线速度 (mm/s) — 减速下限 */
#define MIN_CRUISE_SPEED     50.0f

/* 出线判定: 连续丢失线周期数 */
#define LINE_LOST_THRESH     5

/* 灰度通道宽度 (mm/通道) — 12 通道覆盖约 132mm */
#define CHANNEL_WIDTH_MM     11.0f

/* ═══════════════════════ 卡尔曼滤波器参数 ═══════════════════════ */

/* 初始协方差 */
#define KF_INIT_P_POS        100.0f   /* 位置不确定度 (mm²)        */
#define KF_INIT_P_HEAD        0.1f    /* 航向不确定度 (rad²)       */
#define KF_INIT_P_VEL        100.0f   /* 速度不确定度 (mm²/s²)    */
#define KF_INIT_P_OMEGA       0.01f   /* 角速度不确定度            */

/* 过程噪声 (每 10ms) */
#define KF_Q_POS             0.1f     /* 位置过程噪声               */
#define KF_Q_HEAD            0.001f   /* 航向过程噪声               */
#define KF_Q_VEL             50.0f    /* 速度过程噪声               */
#define KF_Q_OMEGA           0.005f   /* 角速度过程噪声             */

/* 测量噪声 */
#define KF_R_LINE            0.01f    /* 灰度横向测量噪声 (小=更信任) */
#define KF_R_VERTEX          10.0f    /* 顶点位置测量噪声 (mm²)     */

/* ═══════════════════════ 打滑检测 ═══════════════════════ */

/* 加速度计 vs 编码器 dv/dt 差异阈值 (mm/s²) */
#define SLIP_ACCEL_THRESH    500.0f

/* 打滑时 Q_vel 临时放大倍数 */
#define SLIP_Q_VEL_SCALE     10.0f

/* ═══════════════════════ 其他 ═══════════════════════ */

/* LCD 自动返回 STATUS 时间 (ms) */
#define MENU_TIMEOUT_MS      5000U

/* LED 心跳周期 (ms) */
#define HEARTBEAT_MS         500U

/* UART 调试输出开关 */
#define DEBUG_UART_ENABLE    1

#endif /* APP_CONFIG_H */
