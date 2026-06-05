/*
 * ============ app_flash.c =============
 * Flash 配置持久化实现 — W25Q128 Sector 0
 */

#include "app_flash.h"
#include "app_config.h"

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

    if (!ok) return false;

    memcpy(cfg, buf, FLASH_CONFIG_SIZE);

    /* 验证 magic */
    if (cfg->magic != FLASH_CONFIG_MAGIC) return false;

    /* 验证 checksum */
    uint32_t expected = cfg->checksum;
    uint32_t computed = compute_checksum(cfg);
    if (expected != computed) return false;

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

    bool ok = HW_W25Q128_eraseSector(0);  /* Sector 0 = addr 0x000000 */
    if (!ok) {
        /* Flash 擦除忙等待期间会释放 SPI 锁 */
        HAL_SPI_unlock();
        return false;
    }

    /* 写入配置 (最大 256 字节, 一页内) */
    ok = HW_W25Q128_write((const uint8_t*)&local, FLASH_CONFIG_ADDR,
                          FLASH_CONFIG_SIZE);

    HAL_SPI_unlock();
    return ok;
}

/* ══════════ 默认值 ══════════ */

void flash_config_defaults(flash_config_t *cfg)
{
    memset(cfg, 0, sizeof(flash_config_t));

    cfg->magic   = FLASH_CONFIG_MAGIC;
    cfg->version = FLASH_CONFIG_VERSION;

    /* 按键重映射: 默认 1:1 */
    cfg->btn_remap[0] = 0;  /* UP     → UP    */
    cfg->btn_remap[1] = 1;  /* LEFT   → LEFT  */
    cfg->btn_remap[2] = 2;  /* DOWN   → DOWN  */
    cfg->btn_remap[3] = 3;  /* RIGHT  → RIGHT */
    cfg->btn_remap[4] = 4;  /* CENTER → ENTER */
    cfg->btn_remap[5] = 5;  /* BUTTON → BACK  */

    /* PID 默认值 */
    cfg->speed_kp    = DEFAULT_SPEED_KP;
    cfg->speed_ki    = DEFAULT_SPEED_KI;
    cfg->speed_kd    = DEFAULT_SPEED_KD;
    cfg->pos_kp      = DEFAULT_POS_KP;
    cfg->steer_kp    = DEFAULT_STEER_KP;
    cfg->steer_kd    = DEFAULT_STEER_KD;
    cfg->heading_kp  = DEFAULT_HEADING_KP;
    cfg->target_speed = DEFAULT_TARGET_SPEED;

    /* 校准默认值 */
    cfg->imu_yaw_offset  = 0.0f;
    cfg->imu_roll_offset = 0.0f;

    /* 灰度阈值 (0=禁用阈值过滤) */
    cfg->gray_threshold = 0;

    /* 蜂鸣器默认开启 */
    cfg->buzzer_enabled = 1;
    cfg->flags = FLASH_FLAG_BUZZER_EN;

    /* 圈速清零 */
    memset(cfg->lap_times, 0, sizeof(cfg->lap_times));

    cfg->checksum = compute_checksum(cfg);
}
