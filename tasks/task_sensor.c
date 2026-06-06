/*
 * ============ task_sensor.c =============
 * 传感器融合任务 (100 Hz, idle+6 — 最高优先级)
 *
 * 数据采集: IMU (JY61P I2C0 DMA) + 灰度 (NCHD12 I2C1 DMA) + 编码器
 * 融合:     Kalman 预测 + 灰度测量更新
 * 输出:     volatile g_kf_state (Control 任务读取) + sensor_queue (LCD 任务)
 * 维护:     LED 心跳, 看门狗喂狗, 打滑检测
 */

#include "task_button.h"
#include "app_config.h"
#include "app_kalman.h"
#include "app_slip.h"

#include "bsp_i2c.h"
#include "bsp_uart.h"
#include "util.h"          /* util_itoa */
#include "hw_jy61p.h"
#include "app_motor_test.h"

#include <string.h>        /* strncmp */
#include "hw_nchd12.h"
#include "hw_motor.h"

#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

/* ══════════ 全局 Kalman 状态 (volatile — Sensor 写, Control/LCD 读) ══════════ */

volatile float g_kf_x     = 0.0f;
volatile float g_kf_y     = 0.0f;
volatile float g_kf_theta = 0.0f;
volatile float g_kf_v     = 0.0f;
volatile float g_kf_omega = 0.0f;

/* 完整 Kalman 实例 (仅供 Sensor 任务修改) */
// static kalman5_t g_kf;
kalman5_t g_kf;

/* 完整传感器数据 (volatile — Sensor 写, Control/LCD 读) */
volatile sensor_data_t g_sensor_data;

/* 外部: 蜂鸣器使能 / LED 心跳使能 */
bool g_buzzer_enabled        = true;
bool g_led_heartbeat_enabled = true;

/* 外部: 控制模式 (Control 任务写, Sensor 读 — 判断是否在弧线上) */
extern volatile int g_ctrl_mode;
#define CTRL_MODE_LINE_TRACK  2   /* 与 app_control.h 中一致 */

/* ══════════ 灰度质心计算 ══════════ */

static float compute_line_centroid(uint16_t gray_12bit)
{
    int sum_pos = 0, sum_cnt = 0;
    for (int i = 0; i < 12; i++) {
        if (gray_12bit & (1U << (11 - i))) {   /* bit[11] = 传感器 0 (最左) */
            sum_pos += i;
            sum_cnt++;
        }
    }
    if (sum_cnt == 0) return 0.0f;
    return (float)sum_pos / (float)sum_cnt - 5.5f;  /* 居中归零 */
}

/* ══════════ Sensor 任务 ══════════ */

void vTaskSensor(void *pvParameters)
{
    (void)pvParameters;

    /* ── 初始化传感器 ── */
    BSP_I2C_init();
    JY61P_init();        /* 配置 IMU: 200Hz, 加速度+角度模式 */
    /* 陀螺自动校准: 静止 500ms, 角速度变化 < 10 (≈0.6°/s) → 自动归零 */
    JY61P_set_gyro_autocal(10, 500);

    /* ── 初始化 Kalman (起点 A 为原点) ── */
    kalman5_init(&g_kf, VERTEX_A_X, VERTEX_A_Y, 0.0f);  /* 朝东, heading=0 */

    /* ── 初始化打滑检测 ── */
    slip_init();

    /* ── 清零编码器 ── */
    Motor_encReset();

    TickType_t xLastWakeTime = xTaskGetTickCount();

    /* 加速度计上电零偏校准: 前 100 样本取均值 */
    static float  g_accel_bias_ax = 0.0f;
    static int    g_accel_cal_cnt = 100;
    static float  g_accel_cal_sum = 0.0f;

    /* 心跳计数 */
    uint8_t hb_cnt = 0;

    for (;;) {
        /* ── 1. 读取 IMU 角度 + 原始 6 轴 (DMA, 非阻塞) ── */
        JY61P_RawAngle ra;
        JY61P_RawIMU   ri;
        bool imu_ok = JY61P_readAngle(&ra) && JY61P_readIMU_dma(&ri);

        if (imu_ok) {
            float ang_yaw = (float)ra.yaw * JY61P_ANG_SCALE;
            JY61P_updateTotalYaw(ra.yaw);

            g_sensor_data.jy61p_ok = true;

            /* ── JY61P 坐标系 → 车体坐标系 ──
             * JY61P 安装: Yj-=前进, Xj+=左, Zj+=上
             * 车体坐标系: Xc+=前进, Yc+=左, Zc+=上
             *   Xc = -Yj,  Yc = Xj,  Zc = Zj
             *
             *   前向加速度 ax = -JY61P_ay  (Yj- → 前)
             *   左向加速度 ay =  JY61P_ax  (Xj+ → 左)
             *
             *   车体横滚(绕Xc前轴) = -JY61P_pitch (绕Yj-)
             *   车体俯仰(绕Yc横轴) =  JY61P_roll  (绕Xj+)
             *   车体偏航            =  JY61P_yaw   (绕Zj, 不变)
             */
            g_sensor_data.roll     = -(float)ra.pitch * JY61P_ANG_SCALE;
            g_sensor_data.pitch    =  (float)ra.roll  * JY61P_ANG_SCALE;
            g_sensor_data.yaw      = ang_yaw;
            g_sensor_data.total_yaw = JY61P_getTotalYaw();
            /* 加速度: 前=-JY61P_Y, 减零偏 → 1 状态 Kalman 平滑 */
            float ax_raw = -(float)ri.ay * JY61P_ACC_SCALE * 1000.0f;
            if (g_accel_cal_cnt > 0) {
                g_accel_cal_sum += ax_raw;
                if (--g_accel_cal_cnt == 0)
                    g_accel_bias_ax = g_accel_cal_sum / 100.0f;
            }
            float ax_unbiased = ax_raw - g_accel_bias_ax;

            /* 1 状态 Kalman: 滤除电机振动噪声 (~40Hz), 保留运动信号 (<5Hz) */
            {
                static float ax_state = 0.0f;   /* 估计值 */
                static float ax_P     = 100.0f; /* 协方差 */
                const float Q_acc = 50.0f;      /* 过程噪声: 加速度变化率 */
                const float R_acc = 2000.0f;    /* 测量噪声: 传感器抖动 */

                /* 预测 */
                ax_P += Q_acc;

                /* 更新 */
                float K = ax_P / (ax_P + R_acc);
                ax_state += K * (ax_unbiased - ax_state);
                ax_P = (1.0f - K) * ax_P;
                g_sensor_data.ax = ax_state;
            }
            g_sensor_data.ay =  (float)ri.ax * JY61P_ACC_SCALE * 1000.0f;
            g_sensor_data.az =  (float)ri.az * JY61P_ACC_SCALE * 1000.0f;
            /* 角速度: 横滚速率=-JY61P_gy, 俯仰速率=JY61P_gx, 偏航=gz 不变 */
            g_sensor_data.gx = -(float)ri.gy * JY61P_GYRO_SCALE;
            g_sensor_data.gy =  (float)ri.gx * JY61P_GYRO_SCALE;
            g_sensor_data.gz =  (float)ri.gz * JY61P_GYRO_SCALE;
        } else {
            g_sensor_data.jy61p_ok = false;
        }

        /* ── 2. 读取灰度传感器 ──
         * 连续失败 ≥ 5 次则跳过 (避免 I2C 超时阻塞 LCD 任务) */
        uint16_t gray_bits = g_sensor_data.grayscale;
        bool gray_ok = false;
        {
            static uint8_t gray_fail_cnt = 0;
            if (gray_fail_cnt < 5) {
                gray_ok = NCHD12_read(&gray_bits);
                if (gray_ok) gray_fail_cnt = 0;
                else         gray_fail_cnt++;
            }
        }
        g_sensor_data.nchd12_ok    = gray_ok;
        g_sensor_data.grayscale    = gray_bits;
        g_sensor_data.line_position = compute_line_centroid(gray_bits);

        /* ── 3. 读取编码器 ── */
        g_sensor_data.enc1_dist = Motor_enc1Dist();
        g_sensor_data.enc2_dist = Motor_enc2Dist();
        float cur_speed = (float)(g_enc1_speed + g_enc2_speed) * 0.5f
                         * DIST_PER_COUNT * 100.0f;  /* counts/10ms → mm/s */
        g_sensor_data.enc_speed = cur_speed;

        /* ── 4. 里程计增量 ── */
        float delta_dist  = (float)(g_enc1_speed + g_enc2_speed) * 0.5f
                           * DIST_PER_COUNT;          /* mm/10ms */
        float gyro_rate   = g_sensor_data.gz;          /* °/s */
        float delta_theta = gyro_rate * 3.14159265359f / 180.0f * 0.01f; /* °/s→rad/10ms */

        /* ── 5. Kalman 预测 ── */
        kalman5_predict(&g_kf, delta_dist, delta_theta, 0.01f, false,
                        g_sensor_data.ax);  /* ax 仅显示用, 不参与预测 */

        /* ── 7. Kalman 测量更新 — 弧线段灰度 ── */
        if (gray_ok && gray_bits != 0 && g_ctrl_mode == CTRL_MODE_LINE_TRACK) {
            /* 横向偏差: line_position (-5.5~+5.5) × 通道宽度 */
            float cross_track = g_sensor_data.line_position * CHANNEL_WIDTH_MM;
            /* 弧线切向角: 从弧线几何计算 */
            float arc_tan = 0.0f;
            if (g_kf.x >= 0.0f) {
                /* 右弧: 圆心 (500, 0), 切向 = dy/dx 切线 */
                float dx = g_kf.x - TRACK_RIGHT_CX;
                float dy = g_kf.y - TRACK_RIGHT_CY;
                arc_tan = atan2f(-dx, dy);  /* 切线方向 */
            } else {
                /* 左弧: 圆心 (-500, 0) */
                float dx = g_kf.x - TRACK_LEFT_CX;
                float dy = g_kf.y - TRACK_LEFT_CY;
                arc_tan = atan2f(dx, -dy);
            }
            kalman5_update_cross_track(&g_kf, cross_track, arc_tan);
        }

        /* ── 7b. 顶点位置修正 (Control 任务触发) ── */
        {
            extern volatile int g_kf_vertex_trigger;
            int vid = g_kf_vertex_trigger;
            if (vid >= 0 && vid <= 3) {
                const float vx[] = {VERTEX_A_X, VERTEX_B_X, VERTEX_C_X, VERTEX_D_X};
                const float vy[] = {VERTEX_A_Y, VERTEX_B_Y, VERTEX_C_Y, VERTEX_D_Y};
                kalman5_update_vertex(&g_kf, vx[vid], vy[vid]);
                g_kf_vertex_trigger = -1;  /* 清除触发 */
            }
        }

        /* ── 8. 写 volatile 全局 (Control 任务读取) ── */
        g_kf_x     = g_kf.x;
        g_kf_y     = g_kf.y;
        g_kf_theta = g_kf.theta;
        g_kf_v     = g_kf.v;
        g_kf_omega = g_kf.omega;

        /* ── 9. LED 心跳 (每 500ms) ── */
        if (g_led_heartbeat_enabled) {
            hb_cnt++;
            if (hb_cnt >= (HEARTBEAT_MS / PERIOD_SENSOR_MS)) {
                hb_cnt = 0;
                DL_GPIO_togglePins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
            }
        }

        /* ── 10. Feed watchdog ── */
        /* DL_WWDT_restart(WWDT0);  — 如果 SysConfig 配置了 WWDT */

        /* ── 11. 发送 sensor_queue (mailbox) ── */
        xQueueOverwrite(g_sensor_queue, (void*)&g_sensor_data);

        /* ── 12. UART 电机测试命令 ──
         * 指令 (以 \r 结束):
         *   ma+PWM   — Motor A 正转 (如 ma+200)
         *   ma-PWM   — Motor A 反转 (如 ma-150)
         *   mb+PWM   — Motor B 正转
         *   mb-PWM   — Motor B 反转
         *   mboth+PWM— 双轮同转
         *   mstop    — 全停
         *   menc     — 打印编码器值
         *   mtest    — 自动测试序列
         *   1/2/3/4/5/6 — 快捷: Motor A/B 分别 ±200/±300 测试
         */
        {
            static char  uart_buf[16];
            static uint8_t uart_idx = 0;

            if (BSP_UART_rx_ready()) {
                char c = (char)BSP_UART_rx_byte();
                if (c == '\r' || c == '\n') {
                    uart_buf[uart_idx] = 0;
                    uart_idx = 0;

                    /* 解析命令 */
                    int16_t pwm_val = 0;
                    if (uart_buf[0] == 'm') {
                        if (strncmp(uart_buf, "mstop", 5) == 0) {
                            Motor_set(0, 0);
                            BSP_UART_tx_str("[MTR] STOP\r\n");
                        }
                        else if (strncmp(uart_buf, "menc", 4) == 0) {
                            char tmp[64]; int p=0;
                            p+=util_itoa(Motor_enc1(), tmp+p); tmp[p++]=' ';
                            p+=util_itoa(Motor_enc2(), tmp+p); tmp[p++]=' ';
                            p+=util_itoa(g_enc1_speed,tmp+p); tmp[p++]=' ';
                            p+=util_itoa(g_enc2_speed,tmp+p);
                            tmp[p]=0;
                            BSP_UART_tx_str("[MTR] ENC1 ENC2 SPD1 SPD2: ");
                            BSP_UART_tx_str(tmp); BSP_UART_tx_str("\r\n");
                        }
                        else if (strncmp(uart_buf, "mtest", 5) == 0) {
                            motor_test_start();
                            BSP_UART_tx_str("[MTR] Auto test started\r\n");
                        }
                        else if (uart_buf[1] == 'a' || uart_buf[1] == 'b' || uart_buf[1] == 'o') {
                            /* parse: ma+200, mb-150, mboth+300 */
                            char motor = uart_buf[1];
                            bool negative = (uart_buf[2] == '-' || (uart_buf[1]=='b' && uart_buf[2]=='o' && uart_buf[4]=='-'));
                            const char *num_start = uart_buf + 2;
                            if (motor == 'b' && uart_buf[2] == 'o') num_start = uart_buf + 4;
                            if (*num_start == '+' || *num_start == '-') num_start++;
                            /* 手动 atoi */
                            {
                                int val = 0;
                                while (*num_start >= '0' && *num_start <= '9') {
                                    val = val * 10 + (*num_start - '0');
                                    num_start++;
                                }
                                pwm_val = (int16_t)val;
                            }
                            if (negative) pwm_val = -pwm_val;

                            if (motor == 'a') {
                                Motor_set(pwm_val, 0);
                                BSP_UART_tx_str("[MTR] Motor A = "); BSP_UART_tx_str(uart_buf+2); BSP_UART_tx_str("\r\n");
                            } else if (motor == 'b') {
                                Motor_set(0, pwm_val);
                                BSP_UART_tx_str("[MTR] Motor B = "); BSP_UART_tx_str(uart_buf+2); BSP_UART_tx_str("\r\n");
                            } else {
                                Motor_set(pwm_val, pwm_val);
                                BSP_UART_tx_str("[MTR] Both = "); BSP_UART_tx_str(uart_buf+4); BSP_UART_tx_str("\r\n");
                            }
                        }
                    }
                    /* 单字节命令 (避免多字节 UART 丢字符) */
                    else if (uart_buf[1] == 0) {
                        switch (uart_buf[0]) {
                        case '1': Motor_set( 200,   0); BSP_UART_tx_str("[MTR] A +200\r\n"); break;
                        case '2': Motor_set(-200,   0); BSP_UART_tx_str("[MTR] A -200\r\n"); break;
                        case '3': Motor_set(   0, 200); BSP_UART_tx_str("[MTR] B +200\r\n"); break;
                        case '4': Motor_set(   0,-200); BSP_UART_tx_str("[MTR] B -200\r\n"); break;
                        case '5': Motor_set( 200, 200); BSP_UART_tx_str("[MTR] BOTH +200\r\n"); break;
                        case '6': Motor_set(-200,-200); BSP_UART_tx_str("[MTR] BOTH -200\r\n"); break;
                        case '0': Motor_set(   0,   0); BSP_UART_tx_str("[MTR] STOP\r\n"); break;
                        case 'e': {
                            char tmp[64]; int p=0;
                            p+=util_itoa(Motor_enc1(),tmp+p); tmp[p++]=' ';
                            p+=util_itoa(Motor_enc2(),tmp+p); tmp[p++]=' ';
                            p+=util_itoa(g_enc1_speed,tmp+p); tmp[p++]=' ';
                            p+=util_itoa(g_enc2_speed,tmp+p);
                            tmp[p]=0;
                            BSP_UART_tx_str("[MTR] ENC1 ENC2 SPD1 SPD2: ");
                            BSP_UART_tx_str(tmp); BSP_UART_tx_str("\r\n");
                            break;
                        }
                        }
                    }
                } else if (uart_idx < 15) {
                    uart_buf[uart_idx++] = c;
                }
            }

            /* 自动测试状态机 (每 50ms 推进一次) */
            {
                static uint8_t mt_div = 0;
                mt_div++;
                if (mt_div >= 5) {  /* 50ms */
                    mt_div = 0;
                    motor_test_run();
                }
            }
        }

        /* ── 严格 10ms 周期 ── */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(PERIOD_SENSOR_MS));
    }
}
