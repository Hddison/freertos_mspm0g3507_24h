/*
 *  ============ hw_jy61p.h =============
 *  JY61P 姿态传感器驱动 (6 轴: 加速度 + 陀螺仪)
 */

#ifndef HW_JY61P_H
#define HW_JY61P_H

#include <stdbool.h>
#include <stdint.h>

#define JY61P_ADDR          0x50

/* 寄存器 */
#define JY61P_REG_SAVE      0x00
#define JY61P_REG_CALSW     0x01
#define JY61P_REG_RSW       0x02
#define JY61P_REG_RRATE     0x03
#define JY61P_REG_AX        0x34
#define JY61P_REG_AY        0x35
#define JY61P_REG_AZ        0x36
#define JY61P_REG_GX        0x37
#define JY61P_REG_GY        0x38
#define JY61P_REG_GZ        0x39
#define JY61P_REG_ROLL      0x3D
#define JY61P_REG_PITCH     0x3E
#define JY61P_REG_YAW       0x3F
#define JY61P_REG_TEMP      0x40
#define JY61P_REG_KEY       0x69
#define JY61P_REG_VER       0x2E
#define JY61P_REG_IIC       0x1A

/* 命令 */
#define JY61P_KEY_UNLOCK    0xB588
#define JY61P_RRATE_200HZ   0x000B
#define JY61P_RSW_ACC_ANG   0x000E
#define JY61P_CAL_REF       0x0008
#define JY61P_SAVE          0x0000

/* 转换 */
#define JY61P_ANG_SCALE     (180.0f / 32768.0f)
#define JY61P_ACC_SCALE     (16.0f * 9.8f / 32768.0f)
#define JY61P_GYRO_SCALE    (2000.0f / 32768.0f)

/* 数据结构 */
typedef struct {
    int16_t roll, pitch, yaw;
} JY61P_RawAngle;

typedef struct {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
} JY61P_RawIMU;

typedef struct {
    float roll, pitch, yaw;
} JY61P_Angle;

typedef struct {
    float ax, ay, az;
    float gx, gy, gz;
} JY61P_IMU;

/* API */
bool JY61P_init(void);
bool JY61P_readVersion(uint16_t *ver);
bool JY61P_readIICAddr(uint8_t *addr);
bool JY61P_readAngle(JY61P_RawAngle *raw);
bool JY61P_readIMU(JY61P_RawIMU *raw);

/* 一次 DMA 读 12 字节: AX(0x34)~GZ(0x39) 连续, 全 6 轴 */
bool JY61P_readIMU_dma(JY61P_RawIMU *raw);

/* 累积偏航: 自动解卷绕 (±180° → 连续角度) */
float JY61P_getTotalYaw(void);           /* 返回累积角度 (°) */
void  JY61P_updateTotalYaw(int16_t raw); /* 用原始值更新累积 */

static inline void JY61P_convAngle(const JY61P_RawAngle *r, JY61P_Angle *a)
{
    a->roll  = (float)r->roll  * JY61P_ANG_SCALE;
    a->pitch = (float)r->pitch * JY61P_ANG_SCALE;
    a->yaw   = (float)r->yaw   * JY61P_ANG_SCALE;
}

#endif
