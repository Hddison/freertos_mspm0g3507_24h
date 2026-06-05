/*
 * ============ app_slip.h =============
 * 编码器打滑检测 — 加速度计辅助
 *
 * 原理: IMU 加速度计测量车身实际加速度 (ax_forward),
 *       编码器差分得到轮速推算加速度 (dv_enc),
 *       两者差异过大 → 车轮打滑。
 *
 * 打滑时: 临时降低 Kalman 对编码器速度的信任 (增大 Q_vel),
 *         并触发降速保护。
 */

#ifndef APP_SLIP_H
#define APP_SLIP_H

#include <stdbool.h>

/* ── 初始化 ── */
void slip_init(void);

/* ── 检测打滑 ──
 *   ax_forward:  IMU 前向加速度 (mm/s²), 已转换到车体坐标系
 *   dv_enc:      编码器速度差分 (mm/s²), = (v_now - v_prev) / dt
 *   返回:        true = 检测到打滑
 */
bool slip_detect(float ax_forward, float dv_enc);

/* ── 查询打滑状态 ── */
bool slip_is_active(void);

/* ── 重置打滑状态 ── */
void slip_reset(void);

#endif /* APP_SLIP_H */
