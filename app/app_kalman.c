/*
 * ============ app_kalman.c =============
 * 5 状态 EKF 实现 — 小车位置 & 姿态融合
 *
 * 所有矩阵运算手工展开 (Cortex-M0+ 无 FPU, 避免通用线性代数库)。
 * 测量更新使用标量形式, 除法在分母是标量, 避免矩阵求逆。
 */

#include "app_kalman.h"
#include "app_config.h"
#include <math.h>    /* cosf, sinf, fabsf */
#include <string.h>  /* memset */

/* ══════════ 协方差矩阵索引 (上三角存储) ══════════ */

/* P(i,j) 的线性索引, i<=j */
#define P_IDX(i, j)  ((i) * KF_STATE_DIM + (j) - (i) * ((i) + 1) / 2)

/* 读取 / 写入对称元素 (自动取 i<=j) */
static inline float  p_get(const float *P, int i, int j)
{
    if (i <= j) return P[P_IDX(i, j)];
    else        return P[P_IDX(j, i)];  /* 对称 */
}

static inline void p_set(float *P, int i, int j, float val)
{
    if (i <= j) P[P_IDX(i, j)] = val;
    else        P[P_IDX(j, i)] = val;
}

/* ══════════ 角度归一化 ══════════ */

float kalman5_wrap_angle(float a)
{
    while (a >  3.14159265359f) a -= 2.0f * 3.14159265359f;
    while (a < -3.14159265359f) a += 2.0f * 3.14159265359f;
    return a;
}

/* ══════════ 初始化 ══════════ */

void kalman5_init(kalman5_t *kf, float init_x, float init_y,
                  float init_theta)
{
    memset(kf, 0, sizeof(kalman5_t));

    /* 初始状态 */
    kf->x     = init_x;
    kf->y     = init_y;
    kf->theta = init_theta;
    kf->v     = 0.0f;
    kf->omega = 0.0f;

    /* 初始协方差 (对角, 表达初始不确定度) */
    p_set(kf->P, 0, 0, KF_INIT_P_POS);     /* P(x,x)   */
    p_set(kf->P, 1, 1, KF_INIT_P_POS);     /* P(y,y)   */
    p_set(kf->P, 2, 2, KF_INIT_P_HEAD);    /* P(θ,θ)   */
    p_set(kf->P, 3, 3, KF_INIT_P_VEL);     /* P(v,v)   */
    p_set(kf->P, 4, 4, KF_INIT_P_OMEGA);   /* P(ω,ω)   */

    /* 过程噪声 */
    kf->Q_pos        = KF_Q_POS;
    kf->Q_head       = KF_Q_HEAD;
    kf->Q_vel        = KF_Q_VEL;
    kf->Q_omega      = KF_Q_OMEGA;
    kf->Q_vel_scale  = 1.0f;

    /* 测量噪声 */
    kf->R_line   = KF_R_LINE;
    kf->R_vertex = KF_R_VERTEX;
}

/* ══════════ 标量测量更新 (核心) ══════════ */

/*
 * 对状态向量做标量 Kalman 更新:
 *   z: 测量值 (标量)
 *   h: 测量矩阵 (1×5 行向量, 元素为 h[0..4])
 *   hx_pred: h · x_pred (预测的测量值)
 *   R: 测量噪声方差
 */
static void kalman_scalar_update(kalman5_t *kf, const float h[5],
                                 float innovation, float R)
{
    /* 计算 temp = P * h^T  (5×1 向量) */
    float temp[5];
    for (int j = 0; j < KF_STATE_DIM; j++) {
        temp[j] = 0.0f;
        for (int i = 0; i < KF_STATE_DIM; i++) {
            temp[j] += p_get(kf->P, i, j) * h[i];
        }
    }

    /* 计算创新协方差 S = h * P * h^T + R = h * temp + R (标量) */
    float S = R;
    for (int i = 0; i < KF_STATE_DIM; i++) {
        S += h[i] * temp[i];
    }

    /* 防止除零 */
    if (S < 1e-10f) return;

    /* Kalman 增益 K = temp / S (5×1) */
    float K[5];
    for (int i = 0; i < KF_STATE_DIM; i++) {
        K[i] = temp[i] / S;
    }

    /* 状态更新: x = x + K * innovation */
    kf->x     += K[0] * innovation;
    kf->y     += K[1] * innovation;
    kf->theta += K[2] * innovation;
    kf->v     += K[3] * innovation;
    kf->omega += K[4] * innovation;

    /* 协方差更新: P = P - K * temp^T  (即 P -= K⊗temp, 外积) */
    for (int i = 0; i < KF_STATE_DIM; i++) {
        for (int j = i; j < KF_STATE_DIM; j++) {
            float delta = K[i] * temp[j];
            float old   = p_get(kf->P, i, j);
            p_set(kf->P, i, j, old - delta);
        }
    }
}

/* ══════════ 预测步骤 ══════════ */

void kalman5_predict(kalman5_t *kf, float delta_dist, float delta_theta,
                     float dt, bool slip_detected, float ax_body)
{
    /* 防止 dt=0 */
    if (dt < 1e-6f) dt = 0.01f;

    /* ── 状态预测 (中点航向近似) ── */
    float mid_theta = kf->theta + delta_theta * 0.5f;
    float sin_th = sinf(mid_theta);
    float cos_th = cosf(mid_theta);

    /* 位置: 编码器里程计 (硬核, 不受滑差影响) */
    kf->x     += delta_dist * cos_th;
    kf->y     += delta_dist * sin_th;
    kf->theta += delta_theta;
    (void)ax_body;  /* 加速度计仅用于滑差检测和显示, 不参与 EKF 预测 */
    kf->v      = delta_dist / dt;
    kf->omega  = delta_theta / dt;
    kf->theta  = kalman5_wrap_angle(kf->theta);

    /* ── Jacobian F (5×5) ── */
    float F02 = -delta_dist * sin_th;
    float F12 =  delta_dist * cos_th;

    /* ── 计算 P_pred = F * P * F^T + Q ── */
    float FP[5][5];

    /* FP 第 0 行: F[0] = [1,0,F02,0,0] */
    for (int j = 0; j < KF_STATE_DIM; j++) {
        FP[0][j] = p_get(kf->P, 0, j) + F02 * p_get(kf->P, 2, j);
    }
    /* FP 第 1 行: F[1] = [0,1,F12,0,0] */
    for (int j = 0; j < KF_STATE_DIM; j++) {
        FP[1][j] = p_get(kf->P, 1, j) + F12 * p_get(kf->P, 2, j);
    }
    /* FP 第 2 行: F[2] = [0,0,1,0,0] → 直接复制 P 第 2 行 */
    for (int j = 0; j < KF_STATE_DIM; j++) {
        FP[2][j] = p_get(kf->P, 2, j);
    }
    /* FP 第 3,4 行: 全零 */
    for (int j = 0; j < KF_STATE_DIM; j++) {
        FP[3][j] = 0.0f;
        FP[4][j] = 0.0f;
    }

    /* 计算 P_new = FP * F^T (上三角部分) */
    float P_new[KF_P_SIZE];
    for (int i = 0; i < KF_STATE_DIM; i++) {
        for (int j = i; j < KF_STATE_DIM; j++) {
            float sum = 0.0f;
            /* k=0: F[j,0] = (j==0 ? 1 : 0) */
            if (j == 0) sum += FP[i][0] * 1.0f;
            /* k=1: F[j,1] = (j==1 ? 1 : 0) */
            if (j == 1) sum += FP[i][1] * 1.0f;
            /* k=2: F[j,2] = (j==0?F02 : j==1?F12 : j==2?1 : 0) */
            if (j == 0)      sum += FP[i][2] * F02;
            else if (j == 1) sum += FP[i][2] * F12;
            else if (j == 2) sum += FP[i][2] * 1.0f;
            /* k=3,4: F[j,k] = 0 → 无贡献 */

            P_new[P_IDX(i, j)] = sum;
        }
    }

    /* ── 加过程噪声 Q (对角) ── */
    /* 打滑时放大 Q_vel, 降低对速度预测的置信度 */
    float q_vel_scale = slip_detected ? SLIP_Q_VEL_SCALE : 1.0f;
    float q_vel = kf->Q_vel * q_vel_scale;
    P_new[P_IDX(0, 0)] += kf->Q_pos  * dt;
    P_new[P_IDX(1, 1)] += kf->Q_pos  * dt;
    P_new[P_IDX(2, 2)] += kf->Q_head * dt;
    P_new[P_IDX(3, 3)] += q_vel       * dt;
    P_new[P_IDX(4, 4)] += kf->Q_omega * dt;

    /* 写回 */
    memcpy(kf->P, P_new, sizeof(P_new));
}

/* ══════════ 灰度横向测量更新 ══════════ */

float kalman5_update_cross_track(kalman5_t *kf, float cross_track,
                                 float arc_tangent)
{
    float theta = kf->theta;

    /* 更新 1: 航向修正 (基于弧线切向角) */
    float innov_theta = kalman5_wrap_angle(arc_tangent - theta);
    float h_theta[5] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    kalman_scalar_update(kf, h_theta, innov_theta, kf->R_line);

    /* 更新 2: 横向位置修正 (基于灰度质心偏差) */
    /* cross_track 沿着垂直于航向的方向: n = [-sin(θ), cos(θ)] */
    /* 测量: z = -sin(θ)*x + cos(θ)*y = cross_track */
    float sin_th = sinf(theta);
    float cos_th = cosf(theta);
    float h_cross[5] = {-sin_th, cos_th, 0.0f, 0.0f, 0.0f};

    /* 预测的横向位置 */
    float pred_cross = h_cross[0] * kf->x + h_cross[1] * kf->y;
    float innov_cross = cross_track - pred_cross;

    kalman_scalar_update(kf, h_cross, innov_cross, kf->R_line);

    /* 保证航向在 [-π, π] */
    kf->theta = kalman5_wrap_angle(kf->theta);

    return innov_theta;  /* 调试用 */
}

/* ══════════ 顶点绝对位置更新 ══════════ */

void kalman5_update_vertex(kalman5_t *kf, float known_x, float known_y)
{
    float h_x[5] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float h_y[5] = {0.0f, 1.0f, 0.0f, 0.0f, 0.0f};

    float innov_x = known_x - kf->x;
    float innov_y = known_y - kf->y;

    kalman_scalar_update(kf, h_x, innov_x, kf->R_vertex);
    kalman_scalar_update(kf, h_y, innov_y, kf->R_vertex);
}
