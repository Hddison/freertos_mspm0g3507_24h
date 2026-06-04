/*
 *  ============ app_flash.h =============
 *  Flash 存储层 — 所有持久化配置
 *
 *  Sector 0 (4KB) 布局, RMW buffer = 256 bytes:
 *    0x000: sys_config_t   (magic+version+flags+btn_remap+buzzer)
 *    0x020: pid_params_t   (speed+steer+heading PID)
 *    0x040: calib_params_t (IMU offset + gray threshold)
 *    0x060: lap_times[4]   (uint32_t ms)
 *    0xFC:  checksum       (XOR of bytes 0x00-0xFB)
 */

#ifndef APP_FLASH_H
#define APP_FLASH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_FLASH_MAGIC         0x4D53504DUL   /* "MSPM" */
#define APP_FLASH_VERSION       2              /* v2: 扩展配置 */

/* ── 系统配置 ── */
typedef struct {
    uint32_t magic;           /* 0x00: 魔数                            */
    uint16_t version;         /* 0x04: 配置版本                        */
    uint8_t  flags;           /* 0x06: bit0=calib, bit1=remap, bit2=buz */
    uint8_t  reserved;        /* 0x07                                 */
    uint8_t  btn_remap[6];    /* 0x08: 物理→逻辑按键映射               */
    /* 0x0E-0x1F: reserved */
} sys_config_t;

/* flag bits */
#define FLAG_CALIBRATED     (1<<0)
#define FLAG_REMAPPED       (1<<1)
#define FLAG_BUZZER_ENABLED (1<<2)

/* ── PID 参数 ── */
typedef struct {
    float    speed_kp;        /* 0x20                                 */
    float    speed_ki;        /* 0x24                                 */
    float    speed_kd;        /* 0x28                                 */
    float    steer_kp;        /* 0x2C                                 */
    float    steer_kd;        /* 0x30                                 */
    float    heading_kp;      /* 0x34                                 */
    float    target_speed;    /* 0x38: mm/s                           */
} pid_params_t;

/* ── 校准参数 ── */
typedef struct {
    float    imu_yaw_offset;  /* 0x40                                 */
    float    imu_roll_offset; /* 0x44                                 */
    float    imu_pitch_offset;/* 0x48                                 */
    uint16_t gray_threshold;  /* 0x4C                                 */
    uint8_t  reserved[2];    /* 0x4E                                 */
} calib_params_t;

/* ── 兼容: 旧按键绑定配置 (setup wizard 使用) ── */
typedef struct {
    uint32_t magic;
    uint8_t  btn_save;
    uint8_t  btn_restore;
    uint8_t  btn_enc_reset;
    uint8_t  reserved;
    uint32_t checksum;
} app_flash_config_t;

#define BTN_SAVE_YAW_DEFAULT    4   /* CENTER */
#define BTN_RESTORE_YAW_DEFAULT 0   /* UP     */
#define BTN_ENC_RESET_DEFAULT   3   /* RIGHT  */

/* ── 兼容: 旧偏航角存储 ── */
typedef struct {
    uint32_t magic;
    float    saved_yaw;
    uint32_t checksum;
} app_flash_yaw_t;

/* ══════ API ══════ */

/* 初始化, 返回 true=已有配置, false=首次上电 */
bool app_flash_init(void);

/* ── 系统配置 ── */
bool app_flash_loadSysConfig(sys_config_t *cfg);
bool app_flash_saveSysConfig(const sys_config_t *cfg);

/* ── PID 参数 ── */
bool app_flash_loadPid(pid_params_t *pid);
bool app_flash_savePid(const pid_params_t *pid);

/* ── 校准参数 ── */
bool app_flash_loadCalib(calib_params_t *cal);
bool app_flash_saveCalib(const calib_params_t *cal);

/* ── 圈速 ── */
bool app_flash_loadLapTimes(uint32_t times[4]);
bool app_flash_saveLapTimes(const uint32_t times[4]);

/* ── 兼容: 旧接口 ── */
bool app_flash_saveConfig(const app_flash_config_t *cfg);
bool app_flash_loadConfig(app_flash_config_t *cfg);
bool app_flash_saveYaw(float yaw);
bool app_flash_loadYaw(float *yaw);

#ifdef __cplusplus
}
#endif
#endif
