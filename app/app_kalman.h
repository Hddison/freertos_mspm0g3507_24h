/*
 * ============ app_kalman.h =============
 * 5 状态扩展卡尔曼滤波器 — 小车位置 & 姿态融合
 *
 * 状态向量: [x(mm), y(mm), θ(rad), v(mm/s), ω(rad/s)]
 *   0: x      — 世界坐标系 X 位置
 *   1: y      — 世界坐标系 Y 位置
 *   2: theta  — 航向角 (rad, 0=+X东, CCW为正)
 *   3: v      — 线速度
 *   4: omega  — 角速度
 *
 * 所有测量更新使用标量形式, 避免 M0+ 上的 5×5 矩阵求逆。
 */

#ifndef APP_KALMAN_H
#define APP_KALMAN_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── 状态索引 ── */
#define KF_IDX_X     0
#define KF_IDX_Y     1
#define KF_IDX_THETA 2
#define KF_IDX_V     3
#define KF_IDX_OMEGA 4
#define KF_STATE_DIM 5

/* ── 协方差矩阵存储: 5×5 对称矩阵的上三角, 15 个 float ── */
/* P 布局: [P00, P01, P02, P03, P04,
 *              P11, P12, P13, P14,
 *                   P22, P23, P24,
 *                        P33, P34,
 *                             P44]
 * P_ij 索引: i<=j, offset = i*5 + j - i*(i+1)/2
 */
#define KF_P_SIZE 15

typedef struct {
    /* 状态向量 */
    float x, y;          /* 位置 (mm)                        */
    float theta;         /* 航向角 (rad)                     */
    float v;             /* 线速度 (mm/s)                    */
    float omega;         /* 角速度 (rad/s)                   */

    /* 协方差矩阵 (上三角, 15 float) */
    float P[KF_P_SIZE];

    /* 过程噪声 Q (对角) */
    float Q_pos;         /* 位置过程噪声                     */
    float Q_head;        /* 航向过程噪声                     */
    float Q_vel;         /* 速度过程噪声                     */
    float Q_omega;       /* 角速度过程噪声                   */
    float Q_vel_scale;   /* 速度噪声缩放 (打滑时 >1)         */

    /* 测量噪声 R */
    float R_line;        /* 灰度横向测量噪声                 */
    float R_vertex;      /* 顶点位置测量噪声                 */
} kalman5_t;

/* ── 初始化 ──
 *   init_x, init_y: 初始位置 (mm), 通常 (0, 0) 或赛道起点
 *   init_theta:     初始航向 (rad), 0 = 朝 +X (东)
 */
void kalman5_init(kalman5_t *kf, float init_x, float init_y,
                  float init_theta);

/* ── 预测步骤 (每 10ms 调用) ──
 *   delta_dist:  编码器平均前进距离 (mm/10ms), = (Δenc1+Δenc2)/2 * DIST_PER_COUNT
 *   delta_theta: 陀螺仪角增量 (rad/10ms), = gyro_z * dt
 *   dt:          时间步长 (s), 通常 0.01
 *   slip_detected: true=编码器可能打滑, 增大速度过程噪声
 */
void kalman5_predict(kalman5_t *kf, float delta_dist, float delta_theta,
                     float dt, bool slip_detected);

/* ── 测量更新: 灰度横向位置 (弧线段, 每 10ms 有黑线时调用) ──
 *   cross_track: 横向偏差 (mm), 正值=偏右, 从灰度质心 line_position 换算
 *   arc_tangent: 当前位置弧线切向角 (rad), 从弧线几何方程计算
 *   返回: 实际应用的 innovation (可作调试用)
 */
float kalman5_update_cross_track(kalman5_t *kf, float cross_track,
                                 float arc_tangent);

/* ── 测量更新: 顶点绝对位置 (到达 A/B/C/D 点时调用) ──
 *   known_x, known_y: 顶点已知坐标 (mm)
 */
void kalman5_update_vertex(kalman5_t *kf, float known_x, float known_y);

/* ── 角度归一化到 [-π, +π] ── */
float kalman5_wrap_angle(float angle_rad);

/* ── 设置过程噪声缩放 (打滑检测用) ── */
static inline void kalman5_set_vel_noise_scale(kalman5_t *kf, float scale)
{
    kf->Q_vel_scale = scale;
}

#ifdef __cplusplus
}
#endif

#endif /* APP_KALMAN_H */
