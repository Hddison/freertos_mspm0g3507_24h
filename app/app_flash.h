/*
 * ============ app_flash.h =============
 * Flash 配置持久化 — W25Q128 Sector 0
 *
 * 布局 (v2, ~104 字节):
 *   Offset  Size  Field
 *   0x000   4     magic (0x4D53504D "MSPM")
 *   0x004   2     version
 *   0x006   1     flags (bit0=calibrated, bit1=remapped, bit2=buzzer)
 *   0x008   6     btn_remap[6]
 *   0x010   4     speed_kp  (float)
 *   0x014   4     speed_ki  (float)
 *   0x018   4     speed_kd  (float)
 *   0x01C   4     speed_l_kp(float, v2)
 *   0x020   4     speed_l_ki(float, v2)
 *   0x024   4     speed_l_kd(float, v2)
 *   0x028   4     speed_r_kp(float, v2)
 *   0x02C   4     speed_r_ki(float, v2)
 *   0x030   4     speed_r_kd(float, v2)
 *   0x034   4     pos_kp    (float)
 *   0x038   4     steer_kp  (float)
 *   0x03C   4     steer_kd  (float)
 *   0x040   4     heading_kp(float)
 *   0x044   4     heading_ki(float)
 *   0x048   4     heading_kd(float)
 *   0x04C   4     target_speed(float)
 *   0x050   4     imu_yaw_offset(float)
 *   0x054   4     imu_roll_offset(float)
 *   0x058   2     gray_threshold(uint16_t)
 *   0x05A   1     buzzer_enabled(uint8_t)
 *   0x05C   16    lap_times[4](uint32_t ms)
 *   0x06C   4     checksum (XOR)
 */

#ifndef APP_FLASH_H
#define APP_FLASH_H

#include <stdint.h>
#include <stdbool.h>

#define FLASH_CONFIG_MAGIC      0x4D53504DUL   /* "MSPM" */
#define FLASH_CONFIG_VERSION    2
#define FLASH_CONFIG_ADDR       0x00000000UL   /* Sector 0 */
#define FLASH_CONFIG_SIZE       sizeof(flash_config_t)

/* ── 标志位 ── */
#define FLASH_FLAG_CALIBRATED   0x01
#define FLASH_FLAG_REMAPPED     0x02
#define FLASH_FLAG_BUZZER_EN    0x04

/* ── 配置结构体 ── */
typedef struct __attribute__((packed, aligned(4))) {
    uint32_t magic;
    uint16_t version;
    uint8_t  flags;
    uint8_t  btn_remap[6];
    float    speed_kp;
    float    speed_ki;
    float    speed_kd;
    float    speed_l_kp;         /* 左轮速度 PID (v2) */
    float    speed_l_ki;
    float    speed_l_kd;
    float    speed_r_kp;         /* 右轮速度 PID (v2) */
    float    speed_r_ki;
    float    speed_r_kd;
    float    pos_kp;
    float    steer_kp;
    float    steer_kd;
    float    heading_kp;
    float    heading_ki;
    float    heading_kd;
    float    target_speed;
    float    imu_yaw_offset;
    float    imu_roll_offset;
    uint16_t gray_threshold;
    uint8_t  buzzer_enabled;
    /* ── 电机配置 (v2) ── */
    int8_t   motor_a_direction;   /* +1=正PWM=前进, -1=反转 */
    int8_t   motor_b_direction;
    int8_t   enc1_polarity;       /* +1 / -1 编码器方向修正 */
    int8_t   enc2_polarity;
    uint8_t  motor_a_is_left;     /* 1 = Motor A 是左轮, 0 = 右轮 */
    uint32_t lap_times[4];
    uint8_t  _pad1[3];   /* filler to align checksum */
    uint32_t checksum;
} flash_config_t;

/* ── API ── */

/* 从 Flash 加载配置, 验证 magic + checksum。成功返回 true */
bool flash_config_load(flash_config_t *cfg);

/* 保存配置到 Flash (擦除 Sector 0, 写入含 checksum 的完整结构) */
bool flash_config_save(const flash_config_t *cfg);

/* 填充编译期默认值 */
void flash_config_defaults(flash_config_t *cfg);

#endif /* APP_FLASH_H */
