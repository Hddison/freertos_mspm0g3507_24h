/*
 *  ============ app_flash.h =============
 *  Flash 存储层 — 按键绑定配置 + 偏航角持久化
 *
 *  Sector 0 布局 (4KB):
 *    Offset 0-11:  按键配置 (app_flash_config_t)
 *    Offset 16-27: 偏航角   (app_flash_yaw_t)
 *
 *  写入用 RMW: Read 32B → modify → erase sector → write 32B
 */

#ifndef APP_FLASH_H
#define APP_FLASH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_FLASH_MAGIC         0x4D53504DUL   /* "MSPM" */

/* 默认按键 (首次上电后备选) */
#define BTN_SAVE_YAW_DEFAULT    4   /* CENTER */
#define BTN_RESTORE_YAW_DEFAULT 0   /* UP     */
#define BTN_ENC_RESET_DEFAULT   3   /* RIGHT  */

/* ── 按键配置 ── */
typedef struct {
    uint32_t magic;
    uint8_t  btn_save;
    uint8_t  btn_restore;
    uint8_t  btn_enc_reset;
    uint8_t  reserved;
    uint32_t checksum;
} app_flash_config_t;

/* ── 偏航角 ── */
typedef struct {
    uint32_t magic;
    float    saved_yaw;
    uint32_t checksum;
} app_flash_yaw_t;

/* ── API ── */

/* 初始化, 返回 true=已有配置, false=首次上电 */
bool app_flash_init(void);
bool app_flash_saveConfig(const app_flash_config_t *cfg);
bool app_flash_loadConfig(app_flash_config_t *cfg);
bool app_flash_saveYaw(float yaw);
bool app_flash_loadYaw(float *yaw);

#ifdef __cplusplus
}
#endif
#endif
