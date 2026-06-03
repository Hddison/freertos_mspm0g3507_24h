/*
 *  ============ app_flash.c =============
 *  Flash 存储层实现 — CPU 轮询 RMW
 */

#include "app_flash.h"

#include <string.h>
#include <stdio.h>

#include "hw_w25q128.h"
#include "bsp_uart.h"

#define SECTOR0_OFFSET_CONFIG  0
#define SECTOR0_OFFSET_YAW     16
#define SECTOR0_BUF_SIZE       32

static uint32_t _xorChecksum(const uint8_t *d, size_t n)
{
    uint32_t cs = 0;
    for (size_t i = 0; i < n; i++) cs ^= (uint32_t)d[i];
    return cs;
}

/* RMW: 读 32B → 改 offset 处 len 字节 → 擦 → 写 32B */
static bool _sector0_rmw(uint16_t offset, const uint8_t *data, size_t len)
{
    uint8_t buf[SECTOR0_BUF_SIZE];
    if (offset + len > sizeof(buf)) return false;

    HW_W25Q128_read(buf, 0, sizeof(buf));
    memcpy(&buf[offset], data, len);
    if (!HW_W25Q128_eraseSector(0)) return false;
    return HW_W25Q128_write(buf, 0, sizeof(buf));
}

/* ══════ 初始化 ══════ */
bool app_flash_init(void)
{
    uint16_t id = HW_W25Q128_readID();
    if (id != 0xEF17) {
        BSP_UART_tx_str("[Flash] Not detected!\r\n");
        return false;
    }

    app_flash_config_t cfg;
    HW_W25Q128_read((uint8_t *)&cfg, SECTOR0_OFFSET_CONFIG, sizeof(cfg));
    if (cfg.magic != APP_FLASH_MAGIC) return false;
    uint32_t cs = _xorChecksum((const uint8_t *)&cfg, 8);
    return (cs == cfg.checksum);
}

/* ══════ 按键配置 ══════ */
bool app_flash_saveConfig(const app_flash_config_t *cfg)
{
    app_flash_config_t buf;
    memcpy(&buf, cfg, sizeof(buf));
    buf.magic    = APP_FLASH_MAGIC;
    buf.checksum = _xorChecksum((const uint8_t *)&buf, 8);

    if (!_sector0_rmw(SECTOR0_OFFSET_CONFIG, (const uint8_t *)&buf, sizeof(buf)))
        return false;

    /* 验证 */
    app_flash_config_t v;
    HW_W25Q128_read((uint8_t *)&v, SECTOR0_OFFSET_CONFIG, sizeof(v));
    return (v.magic == APP_FLASH_MAGIC && memcmp(&v.btn_save, &buf.btn_save, 4) == 0);
}

bool app_flash_loadConfig(app_flash_config_t *cfg)
{
    HW_W25Q128_read((uint8_t *)cfg, SECTOR0_OFFSET_CONFIG, sizeof(*cfg));
    if (cfg->magic != APP_FLASH_MAGIC) return false;
    uint32_t cs = _xorChecksum((const uint8_t *)cfg, 8);
    return (cs == cfg->checksum);
}

/* ══════ 偏航角 ══════ */
bool app_flash_saveYaw(float yaw)
{
    app_flash_yaw_t buf;
    buf.magic     = APP_FLASH_MAGIC;
    buf.saved_yaw = yaw;
    buf.checksum  = _xorChecksum((const uint8_t *)&buf, 8);

    if (!_sector0_rmw(SECTOR0_OFFSET_YAW, (const uint8_t *)&buf, sizeof(buf)))
        return false;

    app_flash_yaw_t v;
    HW_W25Q128_read((uint8_t *)&v, SECTOR0_OFFSET_YAW, sizeof(v));
    return (v.magic == APP_FLASH_MAGIC && v.saved_yaw == yaw);
}

bool app_flash_loadYaw(float *yaw)
{
    if (!yaw) return false;
    app_flash_yaw_t buf;
    HW_W25Q128_read((uint8_t *)&buf, SECTOR0_OFFSET_YAW, sizeof(buf));
    if (buf.magic != APP_FLASH_MAGIC) return false;
    uint32_t cs = _xorChecksum((const uint8_t *)&buf, 8);
    if (cs != buf.checksum) return false;
    *yaw = buf.saved_yaw;
    return true;
}
