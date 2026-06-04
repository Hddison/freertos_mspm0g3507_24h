/*
 *  ============ hw_jy61p.c =============
 *  JY61P 姿态传感器驱动
 */

#include "hw_jy61p.h"
#include "bsp_i2c.h"

/* 写 16-bit 寄存器: unlock → write reg + value(LE) */
static bool _write(uint8_t reg, uint16_t val)
{
    uint8_t key[2] = { 0x88, 0xB5 };
    uint8_t buf[2] = { (uint8_t)val, (uint8_t)(val >> 8) };

    return BSP_I2C_write(JY61P_ADDR, JY61P_REG_KEY, key, 2)
        && BSP_I2C_write(JY61P_ADDR, reg, buf, 2);
}

/* 读 16-bit 寄存器 */
static bool _read(uint8_t reg, int16_t *val)
{
    uint8_t buf[2];
    if (!BSP_I2C_read(JY61P_ADDR, reg, buf, 2)) return false;
    *val = (int16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
    return true;
}

/* ── API ── */

bool JY61P_init(void)
{
    /* 解锁 → 设 200Hz → 设输出 → 解锁 → 归零 → 解锁 → 保存 → 解锁 */
    return _write(JY61P_REG_KEY,  JY61P_KEY_UNLOCK)
        && _write(JY61P_REG_RRATE, JY61P_RRATE_200HZ)
        && _write(JY61P_REG_RSW,   JY61P_RSW_ACC_ANG)
        && _write(JY61P_REG_KEY,   JY61P_KEY_UNLOCK)
        && _write(JY61P_REG_CALSW, JY61P_CAL_REF)
        && _write(JY61P_REG_KEY,   JY61P_KEY_UNLOCK)
        && _write(JY61P_REG_SAVE,  JY61P_SAVE)
        && _write(JY61P_REG_KEY,   JY61P_KEY_UNLOCK);
}

bool JY61P_readVersion(uint16_t *ver)
{
    uint8_t buf[2];
    if (!ver) return false;
    if (!BSP_I2C_read(JY61P_ADDR, JY61P_REG_VER, buf, 2)) return false;
    *ver = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    return true;
}

bool JY61P_readIICAddr(uint8_t *addr)
{
    uint8_t buf[2];
    if (!addr) return false;
    if (!BSP_I2C_read(JY61P_ADDR, JY61P_REG_IIC, buf, 2)) return false;
    *addr = buf[0];
    return true;
}

bool JY61P_readAngle(JY61P_RawAngle *raw)
{
    /* ROLL(0x3D) / PITCH(0x3E) / YAW(0x3F) 连续, DMA 一次读 6 字节 */
    if (!raw) return false;
    uint8_t buf[6];
    if (!BSP_I2C_read_dma(JY61P_ADDR, JY61P_REG_ROLL, buf, 6))
        return false;
    raw->roll  = (int16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
    raw->pitch = (int16_t)((uint16_t)buf[2] | ((uint16_t)buf[3] << 8));
    raw->yaw   = (int16_t)((uint16_t)buf[4] | ((uint16_t)buf[5] << 8));
    return true;
}

bool JY61P_readIMU(JY61P_RawIMU *raw)
{
    return raw
        && _read(JY61P_REG_AX, &raw->ax)
        && _read(JY61P_REG_AY, &raw->ay)
        && _read(JY61P_REG_AZ, &raw->az)
        && _read(JY61P_REG_GX, &raw->gx)
        && _read(JY61P_REG_GY, &raw->gy)
        && _read(JY61P_REG_GZ, &raw->gz);
}

/* 一次 DMA 读 12 字节: AX(0x34)~GZ(0x39) 连续 */
bool JY61P_readIMU_dma(JY61P_RawIMU *raw)
{
    if (!raw) return false;
    uint8_t buf[12];
    if (!BSP_I2C_read_dma(JY61P_ADDR, JY61P_REG_AX, buf, 12))
        return false;
    raw->ax = (int16_t)((uint16_t)buf[0]  | ((uint16_t)buf[1]  << 8));
    raw->ay = (int16_t)((uint16_t)buf[2]  | ((uint16_t)buf[3]  << 8));
    raw->az = (int16_t)((uint16_t)buf[4]  | ((uint16_t)buf[5]  << 8));
    raw->gx = (int16_t)((uint16_t)buf[6]  | ((uint16_t)buf[7]  << 8));
    raw->gy = (int16_t)((uint16_t)buf[8]  | ((uint16_t)buf[9]  << 8));
    raw->gz = (int16_t)((uint16_t)buf[10] | ((uint16_t)buf[11] << 8));
    return true;
}

/* ── 累积偏航 (解卷绕) ── */

static int32_t total_yaw_raw;   /* 累积原始值, 范围无限 */
static int16_t prev_yaw_raw;    /* 上一次原始值 */
static bool    first_yaw = true;

float JY61P_getTotalYaw(void)
{
    return (float)total_yaw_raw * JY61P_ANG_SCALE;
}

void JY61P_updateTotalYaw(int16_t raw)
{
    if (first_yaw) {
        prev_yaw_raw  = raw;
        total_yaw_raw = (int32_t)raw;
        first_yaw     = false;
        return;
    }

    int32_t delta = (int32_t)raw - (int32_t)prev_yaw_raw;

    /* 越过 ±180° 边界时解卷绕 */
    if      (delta >  32767) delta -= 65536;  /* +179° → -179° */
    else if (delta < -32768) delta += 65536;  /* -179° → +179° */

    total_yaw_raw += delta;
    prev_yaw_raw   = raw;
}
