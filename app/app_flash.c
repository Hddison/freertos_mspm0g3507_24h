/*
 *  ============ app_flash.c =============
 *  Flash 存储层实现 — RMW 256 bytes in Sector 0
 *
 *  布局:
 *    0x00-0x1F: sys_config_t
 *    0x20-0x3F: pid_params_t
 *    0x40-0x5F: calib_params_t
 *    0x60-0x6F: lap_times[4]
 *    0xFC-0xFF: checksum (XOR of 0x00-0xFB)
 */

#include "app_flash.h"

#include <string.h>
#include "hw_w25q128.h"
#include "bsp_uart.h"

#define SECTOR0_CFG_SIZE    256
#define CHKSUM_OFFSET       (SECTOR0_CFG_SIZE - 4)

/* 偏移量 */
#define OFF_SYSCFG   0x00
#define OFF_PID      0x20
#define OFF_CALIB    0x40
#define OFF_LAPTIMES 0x60
#define OFF_YAW      0x70   /* 兼容旧 yaw 存储 */

static uint32_t _xorChecksum(const uint8_t *d, size_t n)
{
    uint32_t cs = 0;
    for (size_t i = 0; i < n; i++) cs ^= (uint32_t)d[i];
    return cs;
}

/* RMW: 读 256B → 改 offset 处 len 字节 → 更新 checksum → 擦 → 写 */
static bool _sector0_rmw(uint16_t offset, const uint8_t *data, size_t len)
{
    uint8_t buf[SECTOR0_CFG_SIZE];
    if (offset + len > CHKSUM_OFFSET) return false;

    HW_W25Q128_read(buf, 0, sizeof(buf));
    memcpy(&buf[offset], data, len);

    /* 更新整体 checksum */
    uint32_t cs = _xorChecksum(buf, CHKSUM_OFFSET);
    memcpy(&buf[CHKSUM_OFFSET], &cs, 4);

    if (!HW_W25Q128_eraseSector(0)) return false;
    return HW_W25Q128_write(buf, 0, sizeof(buf));
}

/* 读指定区域 */
static bool _sector0_read(uint16_t offset, uint8_t *data, size_t len)
{
    if (offset + len > SECTOR0_CFG_SIZE) return false;
    HW_W25Q128_read(data, offset, len);
    return true;
}

/* ══════ 初始化 ══════ */
bool app_flash_init(void)
{
    uint16_t id = HW_W25Q128_readID();
    if (id != 0xEF17) {
        BSP_UART_tx_str("[Flash] Not detected!\r\n");
        return false;
    }

    /* 读取 magic+version */
    uint8_t hdr[8];
    HW_W25Q128_read(hdr, 0, sizeof(hdr));
    uint32_t magic;
    memcpy(&magic, hdr, 4);
    if (magic != APP_FLASH_MAGIC) return false;

    /* 校验 */
    uint8_t buf[SECTOR0_CFG_SIZE];
    HW_W25Q128_read(buf, 0, sizeof(buf));
    uint32_t stored_cs, computed_cs;
    memcpy(&stored_cs, &buf[CHKSUM_OFFSET], 4);
    computed_cs = _xorChecksum(buf, CHKSUM_OFFSET);
    return (stored_cs == computed_cs);
}

/* ══════ 系统配置 ══════ */
bool app_flash_loadSysConfig(sys_config_t *cfg)
{
    if (!_sector0_read(OFF_SYSCFG, (uint8_t *)cfg, sizeof(*cfg))) return false;
    return (cfg->magic == APP_FLASH_MAGIC);
}

bool app_flash_saveSysConfig(const sys_config_t *cfg)
{
    sys_config_t buf;
    memcpy(&buf, cfg, sizeof(buf));
    buf.magic   = APP_FLASH_MAGIC;
    buf.version = APP_FLASH_VERSION;
    return _sector0_rmw(OFF_SYSCFG, (const uint8_t *)&buf, sizeof(buf));
}

/* ══════ PID 参数 ══════ */
bool app_flash_loadPid(pid_params_t *pid)
{
    return _sector0_read(OFF_PID, (uint8_t *)pid, sizeof(*pid));
}

bool app_flash_savePid(const pid_params_t *pid)
{
    return _sector0_rmw(OFF_PID, (const uint8_t *)pid, sizeof(*pid));
}

/* ══════ 校准参数 ══════ */
bool app_flash_loadCalib(calib_params_t *cal)
{
    return _sector0_read(OFF_CALIB, (uint8_t *)cal, sizeof(*cal));
}

bool app_flash_saveCalib(const calib_params_t *cal)
{
    return _sector0_rmw(OFF_CALIB, (const uint8_t *)cal, sizeof(*cal));
}

/* ══════ 圈速 ══════ */
bool app_flash_loadLapTimes(uint32_t times[4])
{
    return _sector0_read(OFF_LAPTIMES, (uint8_t *)times, 16);
}

bool app_flash_saveLapTimes(const uint32_t times[4])
{
    return _sector0_rmw(OFF_LAPTIMES, (const uint8_t *)times, 16);
}

/* ══════ 兼容: 旧按键配置 (setup wizard 使用) ══════ */
bool app_flash_saveConfig(const app_flash_config_t *cfg)
{
    app_flash_config_t buf;
    memcpy(&buf, cfg, sizeof(buf));
    buf.magic    = APP_FLASH_MAGIC;
    buf.checksum = _xorChecksum((const uint8_t *)&buf, 8);
    return _sector0_rmw(0x00, (const uint8_t *)&buf, sizeof(buf));
}

bool app_flash_loadConfig(app_flash_config_t *cfg)
{
    _sector0_read(0x00, (uint8_t *)cfg, sizeof(*cfg));
    if (cfg->magic != APP_FLASH_MAGIC) return false;
    uint32_t cs = _xorChecksum((const uint8_t *)cfg, 8);
    return (cs == cfg->checksum);
}

/* ══════ 兼容: 旧偏航角 ══════ */
bool app_flash_saveYaw(float yaw)
{
    app_flash_yaw_t buf;
    buf.magic     = APP_FLASH_MAGIC;
    buf.saved_yaw = yaw;
    buf.checksum  = _xorChecksum((const uint8_t *)&buf, 8);
    return _sector0_rmw(OFF_YAW, (const uint8_t *)&buf, sizeof(buf));
}

bool app_flash_loadYaw(float *yaw)
{
    if (!yaw) return false;
    app_flash_yaw_t buf;
    _sector0_read(OFF_YAW, (uint8_t *)&buf, sizeof(buf));
    if (buf.magic != APP_FLASH_MAGIC) return false;
    uint32_t cs = _xorChecksum((const uint8_t *)&buf, 8);
    if (cs != buf.checksum) return false;
    *yaw = buf.saved_yaw;
    return true;
}
