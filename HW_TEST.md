# HW 层逐模块测试计划

## 前提

- `main.c` 起点: `SYSCFG_DL_init(); for(;;){}`
- 每步只测一个模块，通过后进入下一步
- 全部输出走 UART0 (115200)，不依赖 LCD
- 每步给出完整可编译的 main.c 代码

---

## 1. UART + LED

**目的**: 确认串口输出、LED 亮灭

```c
#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>
#include "bsp_uart.h"
#include "bsp_system.h"

int main(void) {
    SYSCFG_DL_init();
    BSP_UART_tx_str("\r\n=== Test 1: UART+LED ===\r\n");

    BSP_UART_tx_str("LED ON\r\n");
    DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
    BSP_delay_ms(500);
    BSP_UART_tx_str("LED OFF\r\n");
    DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);

    for(;;) {}
}
```

**验证**: 串口看到 "LED ON" / "LED OFF", LED 亮 500ms 后灭

---

## 2. 按键

**目的**: 6 键扫描, 按下时打印

```c
int main(void) {
    SYSCFG_DL_init();
    BSP_UART_tx_str("\r\n=== Test 2: Buttons ===\r\n");

    uint8_t prev[6] = {0};
    for(;;) {
        for(int i=0; i<6; i++) {
            GPIO_Regs *ports[] = {GPIO_KEY_PIN_UP_PORT, GPIO_KEY_PIN_LEFT_PORT,
                GPIO_KEY_PIN_DOWN_PORT, GPIO_KEY_PIN_RIGHT_PORT,
                GPIO_KEY_PIN_CENTER_PORT, GPIO_KEY_PIN_BUTTON_PORT};
            uint32_t pins[] = {GPIO_KEY_PIN_UP_PIN, GPIO_KEY_PIN_LEFT_PIN,
                GPIO_KEY_PIN_DOWN_PIN, GPIO_KEY_PIN_RIGHT_PIN,
                GPIO_KEY_PIN_CENTER_PIN, GPIO_KEY_PIN_BUTTON_PIN};
            const char *n[] = {"UP","LEFT","DOWN","RIGHT","CENTER","BTN"};

            uint8_t now = (DL_GPIO_readPins(ports[i], pins[i]) == 0);
            if(now && !prev[i]) BSP_UART_tx_str(n[i]), BSP_UART_tx_str(" down\r\n");
            if(!now && prev[i]) BSP_UART_tx_str(n[i]), BSP_UART_tx_str(" up\r\n");
            prev[i] = now;
        }
        BSP_delay_ms(20);
    }
}
```

**验证**: 每按一个键, 串口打印 "XXX down" / "XXX up"

---

## 3. Buzzer

**目的**: PB22 高电平响

```c
#include "hw_buzzer.h"

int main(void) {
    SYSCFG_DL_init();
    BSP_UART_tx_str("\r\n=== Test 3: Buzzer ===\r\n");
    Buzzer_init();

    BSP_UART_tx_str("beep x3\r\n");
    for(int i=0; i<3; i++) {
        Buzzer_set(1); BSP_delay_ms(200);
        Buzzer_set(0); BSP_delay_ms(200);
    }
    for(;;) {}
}
```

**验证**: 蜂鸣器响 3 声

---

## 4. W25Q128 Flash

**目的**: 读 ID, 读/写/擦除测试

```c
#include "hw_w25q128.h"

int main(void) {
    SYSCFG_DL_init();
    DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);  // CS HIGH
    BSP_delay_ms(100);
    BSP_UART_tx_str("\r\n=== Test 4: Flash ===\r\n");

    // 1. ID
    uint16_t id = HW_W25Q128_readID();
    char b[32]; int n = snprintf(b, sizeof(b), "ID: 0x%04X %s\r\n", id,
        (id==0xEF17)?"OK":"FAIL"); BSP_UART_tx_str(b);

    // 2. 写 → 读验证 (Sector 0, 偏移 0x100, 16 字节)
    uint8_t wbuf[16], rbuf[16];
    for(int i=0; i<16; i++) wbuf[i] = (uint8_t)(0xA0 + i);
    if(HW_W25Q128_eraseSector(0)) {
        BSP_UART_tx_str("Erase OK\r\n");
        if(HW_W25Q128_write(wbuf, 0x100, 16)) {
            BSP_UART_tx_str("Write OK, reading...\r\n");
            HW_W25Q128_read(rbuf, 0x100, 16);
            bool ok = true;
            for(int i=0; i<16; i++) if(rbuf[i] != wbuf[i]) ok=false;
            for(int i=0; i<16; i++) {
                snprintf(b, sizeof(b), "  [%d] W:%02X R:%02X %s\r\n",
                    i, wbuf[i], rbuf[i], (wbuf[i]==rbuf[i])?"OK":"MISMATCH");
                BSP_UART_tx_str(b);
            }
            BSP_UART_tx_str(ok ? "Flash R/W PASS\r\n" : "Flash R/W FAIL\r\n");
        } else BSP_UART_tx_str("Write FAIL\r\n");
    } else BSP_UART_tx_str("Erase FAIL\r\n");

    for(;;) {}
}
```

**验证**: ID=0xEF17, 擦/写/读一致

---

## 5. ST7789 LCD

**目的**: 初始化, 纯色填充, 文字

> 依赖 LCD 有显示能力; 若背光不亮(PB26硬件问题), 仅验证 SPI 通信无误

```c
#include "hw_st7789.h"
#include "gui_paint.h"

int main(void) {
    SYSCFG_DL_init();
    BSP_UART_tx_str("\r\n=== Test 5: LCD ===\r\n");

    // 1. Init
    ST7789_init(ST7789_HORIZONTAL);
    ST7789_backLight(1);
    BSP_UART_tx_str("Init done\r\n");

    // 2. 全屏 DMA 红
    ST7789_setWindows(0,0,ST7789_WIDTH-1,ST7789_HEIGHT-1);
    ST7789_clearRawDMA(RED, ST7789_WIDTH, ST7789_HEIGHT);
    BSP_UART_tx_str("Red fill done\r\n");
    BSP_delay_ms(1000);

    // 3. 全屏 DMA 绿
    ST7789_setWindows(0,0,ST7789_WIDTH-1,ST7789_HEIGHT-1);
    ST7789_clearRawDMA(GREEN, ST7789_WIDTH, ST7789_HEIGHT);
    BSP_UART_tx_str("Green fill done\r\n");
    BSP_delay_ms(1000);

    // 4. 文字
    Paint_NewImage(ST7789_WIDTH, ST7789_HEIGHT, ROTATE_0, BLACK);
    Paint_SetClearFuntion(ST7789_clear);
    Paint_SetDisplayFuntion(ST7789_drawPoint);
    Paint_Clear(BLACK);
    Paint_DrawString_EN(10, 140, "LCD OK", &Font20, BLACK, GREEN);
    BSP_UART_tx_str("Text draw done\r\n");

    for(;;) {}
}
```

**验证**: 屏幕依次红→绿→"LCD OK"文字

---

## 6. JY61P IMU

**目的**: 初始化, 读角度, 读加速度/陀螺仪

> 已知风险: IMU 寄存器地址可能错位(AY/AZ/GX/GY/GZ 偏移 1 字节)

```c
#include "hw_jy61p.h"
#include "bsp_i2c.h"

int main(void) {
    SYSCFG_DL_init();
    BSP_delay_ms(100);
    BSP_I2C_init();
    BSP_delay_ms(200);
    BSP_UART_tx_str("\r\n=== Test 6: IMU ===\r\n");

    // 1. Init
    bool ok = false;
    BSP_delay_ms(500);
    for(int i=0; i<10; i++) {
        if(JY61P_init()) { ok=true; break; }
        BSP_UART_tx_str("retry...\r\n"); BSP_delay_ms(1000);
    }
    if(!ok) { BSP_UART_tx_str("IMU init FAIL\r\n"); for(;;){} }

    // 2. 读版本
    uint16_t ver;
    if(JY61P_readVersion(&ver)) {
        char b[32]; snprintf(b,sizeof(b),"Ver: 0x%04X\r\n",ver); BSP_UART_tx_str(b);
    }

    // 3. 连续读角度 + IMU (10 次)
    for(int i=0; i<10; i++) {
        JY61P_RawAngle ra; JY61P_Angle a;
        JY61P_RawIMU ri;
        if(JY61P_readAngle(&ra)) {
            JY61P_convAngle(&ra, &a);
            char b[64];
            snprintf(b,sizeof(b),"[%d] R:%.1f P:%.1f Y:%.1f", i,
                (double)a.roll, (double)a.pitch, (double)a.yaw);
            BSP_UART_tx_str(b);
        }
        if(JY61P_readIMU(&ri)) {
            char b[64];
            snprintf(b,sizeof(b)," Gz:%.1f\r\n", (double)(ri.gz * JY61P_GYRO_SCALE));
            BSP_UART_tx_str(b);
        }
        BSP_delay_ms(200);
    }

    BSP_UART_tx_str("IMU test done\r\n");
    for(;;) {}
}
```

**验证**:
- Roll/Pitch/Yaw 有合理的值 (转动小车, Yaw 应变化)
- Gz 在手动静止时应接近 0
- **若数值异常大或有规律跳变 → 寄存器地址 bug 确认**

---

## 7. NCHD12 灰度

**目的**: 读 12 位灰度数据

```c
#include "hw_nchd12.h"
#include "bsp_i2c.h"

int main(void) {
    SYSCFG_DL_init();
    BSP_delay_ms(100);
    BSP_I2C_init();
    BSP_delay_ms(200);

    BSP_UART_tx_str("\r\n=== Test 7: Grayscale ===\r\n");

    uint16_t gs;
    if(NCHD12_read(&gs)) {
        for(int i=0; i<50; i++) {
            NCHD12_read(&gs);
            char b[32];
            snprintf(b,sizeof(b),"[%2d] 0x%03X [", i, gs&0xFFF);
            BSP_UART_tx_str(b);
            for(int j=0; j<12; j++)
                BSP_UART_tx_byte((gs & (1<<(11-j))) ? '1' : '0');
            BSP_UART_tx_str("]\r\n");
            BSP_delay_ms(200);
        }
    } else {
        BSP_UART_tx_str("NCHD12 read FAIL\r\n");
    }
    for(;;) {}
}
```

**验证**: 白纸上 12 通道全 0, 黑线上对应通道变 1

---

## 8. Motor + Encoder

**目的**: PWM 输出使电机转动, 编码器 ISR 计数

> 风险: 编码器 ISR 依赖 `NVIC_EnableIRQ(GPIOA_INT_IRQn)`, 需手动加

```c
#include "hw_motor.h"
#include "bsp_system.h"

extern void GROUP1_IRQHandler(void);  // 编码器 ISR (hw_motor.c)

int main(void) {
    SYSCFG_DL_init();
    BSP_UART_tx_str("\r\n=== Test 8: Motor+Encoder ===\r\n");

    // 启用编码器中断 (SysConfig 只配了 GPIO, 没开 NVIC)
    NVIC_EnableIRQ(GPIOA_INT_IRQn);

    Motor_init();
    BSP_UART_tx_str("Motor init done\r\n");

    // 读初始编码器
    BSP_UART_tx_str("Enc before:\r\n");
    char b[32]; snprintf(b,sizeof(b),"E1:%ld E2:%ld\r\n",
        (long)Motor_enc1(), (long)Motor_enc2()); BSP_UART_tx_str(b);

    // 电机正转 500ms, PWM=500
    BSP_UART_tx_str("Motor forward 500ms...\r\n");
    Motor_set(500, 500);
    BSP_delay_ms(500);
    Motor_set(0, 0);

    // 读编码器
    snprintf(b,sizeof(b),"E1:%ld E2:%ld\r\n",
        (long)Motor_enc1(), (long)Motor_enc2()); BSP_UART_tx_str(b);

    // 电机反转 500ms
    BSP_UART_tx_str("Motor reverse 500ms...\r\n");
    Motor_set(-500, -500);
    BSP_delay_ms(500);
    Motor_set(0, 0);

    // 读编码器
    snprintf(b,sizeof(b),"E1:%ld E2:%ld (expect ~0)\r\n",
        (long)Motor_enc1(), (long)Motor_enc2()); BSP_UART_tx_str(b);

    BSP_UART_tx_str("Motor test done\r\n");
    for(;;) {}
}
```

**验证**:
- 正转后编码器 +正数, 反转后归零
- 电机实际转动
- 若无编码器变化 → 检查 `NVIC_EnableIRQ(GPIOA)` + 边沿极性
- 若电机不转 → 检查方向引脚 + PWM

---

## 验证顺序

| 序号 | 模块 | 依赖 | 关键检查 |
|------|------|------|----------|
| 1 | UART+LED | 无 | 串口输出, LED 亮 |
| 2 | Button | 无 | 6 键读 GPIO |
| 3 | Buzzer | 无 | PB22 响 |
| 4 | Flash | SPI1 | ID=0xEF17, R/W |
| 5 | LCD | SPI1 | 色块+文字 |
| 6 | IMU | I2C0 | 角度/陀螺合理 |
| 7 | Grayscale | I2C1 | 12 位读黑线 |
| 8 | Motor+Encoder | PWM+GPIOA | 转+编码计数 |

每步通过后再下一步。发现问题时对照 BSP/HW 源码排查。
