/*
 * ============ app_flash.c =============
 * Flash 配置持久化实现 — W25Q128 Sector 0
 */

#include "app_flash.h"
#include "app_config.h"

#include "bsp_button.h"   /* BTN_DIR_* */
#include "hal_spi.h"
#include "hw_w25q128.h"
#include <stddef.h>   /* offsetof */
#include <string.h>

/* ══════════ 计算 checksum (XOR of all bytes) ══════════ */

static uint32_t compute_checksum(const flash_config_t *cfg)
{
    const uint8_t *p = (const uint8_t*)cfg;
    uint32_t cksum = 0;
    /* XOR all bytes before checksum field */
    for (size_t i = 0; i < offsetof(flash_config_t, checksum); i++) {
        cksum = (cksum << 1) | (cksum >> 31);  /* rotate left 1 */
        cksum ^= p[i];
    }
    return cksum;
}

/* ══════════ 加载配置 ══════════ */

bool flash_config_load(flash_config_t *cfg)
{
    uint8_t buf[FLASH_CONFIG_SIZE];

    HAL_SPI_lock();
    bool ok = HW_W25Q128_read(buf, FLASH_CONFIG_ADDR, FLASH_CONFIG_SIZE);
    HAL_SPI_unlock();

    if (!ok) {
        extern void BSP_UART_tx_str(const char*);
        BSP_UART_tx_str("[FLASH] Read FAILED\r\n");
        return false;
    }

    memcpy(cfg, buf, FLASH_CONFIG_SIZE);

    /* 验证 magic */
    if (cfg->magic != FLASH_CONFIG_MAGIC) {
        extern void BSP_UART_tx_str(const char*);
        BSP_UART_tx_str("[FLASH] Bad magic\r\n");
        return false;
    }

    /* 验证 checksum */
    uint32_t expected = cfg->checksum;
    uint32_t computed = compute_checksum(cfg);
    if (expected != computed) {
        extern void BSP_UART_tx_str(const char*);
        char tmp[32];
        tmp[0] = 'e'; tmp[1] = 'x'; tmp[2] = 'p'; tmp[3] = '='; tmp[4] = 0;
        BSP_UART_tx_str("[FLASH] Bad cksum exp=... comp=...\r\n");
        return false;
    }

    return true;
}

/* ══════════ 保存配置 ══════════ */

bool flash_config_save(const flash_config_t *cfg)
{
    /* 计算 checksum */
    flash_config_t local = *cfg;
    local.magic    = FLASH_CONFIG_MAGIC;
    local.version  = FLASH_CONFIG_VERSION;
    local.checksum = compute_checksum(&local);

    /* 获取 SPI 锁, 擦除 Sector 0 */
    HAL_SPI_lock();

    {
        extern void BSP_UART_tx_str(const char*);
        BSP_UART_tx_str("[FLASH] Erasing sector 0...\r\n");
    }
    bool ok = HW_W25Q128_eraseSector(0);
    if (!ok) {
        extern void BSP_UART_tx_str(const char*);
        BSP_UART_tx_str("[FLASH] Erase FAILED\r\n");
        HAL_SPI_unlock();
        return false;
    }

    /* 写入配置 */
    {
        extern void BSP_UART_tx_str(const char*);
        BSP_UART_tx_str("[FLASH] Writing...\r\n");
    }
    ok = HW_W25Q128_write((const uint8_t*)&local, FLASH_CONFIG_ADDR,
                          FLASH_CONFIG_SIZE);

    HAL_SPI_unlock();

    if (ok) {
        extern void BSP_UART_tx_str(const char*);
        BSP_UART_tx_str("[FLASH] Save OK\r\n");
    } else {
        extern void BSP_UART_tx_str(const char*);
        BSP_UART_tx_str("[FLASH] Write FAILED\r\n");
    }
    return ok;
}

/* ══════════ 默认值 ══════════ */

void flash_config_defaults(flash_config_t *cfg)
{
    memset(cfg, 0, sizeof(flash_config_t));

    cfg->magic   = FLASH_CONFIG_MAGIC;
    cfg->version = FLASH_CONFIG_VERSION;

    /* 按键重映射: 物理布局匹配逻辑方向 */
    cfg->btn_remap[0] = BTN_DIR_RIGHT; /* UP     → RIGHT */
    cfg->btn_remap[1] = BTN_DIR_UP;    /* LEFT   → UP    */
    cfg->btn_remap[2] = BTN_DIR_LEFT;  /* DOWN   → LEFT  */
    cfg->btn_remap[3] = BTN_DIR_DOWN;  /* RIGHT  → DOWN  */
    cfg->btn_remap[4] = BTN_DIR_ENTER; /* CENTER → ENTER */
    cfg->btn_remap[5] = BTN_DIR_BACK;  /* BUTTON → BACK  */

    /* PID 默认值 */
    cfg->speed_kp    = DEFAULT_SPEED_KP;
    cfg->speed_ki    = DEFAULT_SPEED_KI;
    cfg->speed_kd    = DEFAULT_SPEED_KD;
    cfg->pos_kp      = DEFAULT_POS_KP;
    cfg->steer_kp    = DEFAULT_STEER_KP;
    cfg->steer_kd    = DEFAULT_STEER_KD;
    cfg->heading_kp  = DEFAULT_HEADING_KP;
    cfg->heading_ki  = DEFAULT_HEADING_KI;
    cfg->heading_kd  = DEFAULT_HEADING_KD;
    cfg->target_speed = DEFAULT_TARGET_SPEED;

    /* 校准默认值 */
    cfg->imu_yaw_offset  = 0.0f;
    cfg->imu_roll_offset = 0.0f;

    /* 灰度阈值 (0=禁用阈值过滤) */
    cfg->gray_threshold = 0;

    /* 蜂鸣器默认开启 */
    cfg->buzzer_enabled = 1;
    cfg->flags = FLASH_FLAG_BUZZER_EN;

    /* 电机方向 & 编码器极性 (默认 1: 正向不反转, 电机A=左轮) */
    cfg->motor_a_direction = 1;
    cfg->motor_b_direction = 1;
    cfg->enc1_polarity      = 1;
    cfg->enc2_polarity      = 1;
    cfg->motor_a_is_left    = 1;

    /* 圈速清零 */
    memset(cfg->lap_times, 0, sizeof(cfg->lap_times));

    cfg->checksum = compute_checksum(cfg);
}
