/*
 *  ============ board_pins.h =============
 *  LP-MSPM0G3507 LaunchPad 引脚映射表
 *
 *  用途: 记录 MCU 引脚与板上/外部硬件的对应关系，
 *        同时提供方便引用的别名宏。
 *
 *  MCU 引脚规划由 SysConfig (posix_demo.syscfg) 图形化管理:
 *    修改引脚后，运行 "SysConfig 生成代码" 任务刷新 syscfg/ 目录，
 *    然后同步更新本文件中的映射注释和别名宏。
 */

#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "ti_msp_dl_config.h"   /* SysConfig 生成的 GPIO_LEDS_* 宏 */

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 *  已使用引脚 (SysConfig 当前配置)
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *  PB22  →  LED1 (板载用户 LED)
 *           端口: GPIOB  |  引脚位: DL_GPIO_PIN_22 (0x400000)
 *           IOMUX: PINCM50  |  封装: LQFP-64 pin 21
 *           模式: 数字输出, 初始高电平 (LED 灭)
 *           跳线: J4 连接 PB22, J19 选择上拉
 *
 *  PA19  →  SWDIO (板载 XDS110 调试器)
 *           模式: DEBUGSS 外设功能, 不可用于 GPIO
 *
 *  PA20  →  SWCLK (板载 XDS110 调试器)
 *           模式: DEBUGSS 外设功能, 不可用于 GPIO
 *
 *  PA10  →  UART0 TX (UART_0, BUSCLK=40MHz)
 *           波特率: 9600, 8N1, 无流控
 *
 *  PA11  →  UART0 RX (UART_0, BUSCLK=40MHz)
 *           波特率: 9600, 8N1, 无流控
 *
 *  时钟树:
 *    HFXT (外部晶振) → SYSPLL → MCLK 80 MHz (CPUCLK)
 *    BUSCLK = MCLK / 2 = 40 MHz (UART/I2C/SPI 等外设时钟)
 */

/* ── LED 别名宏 (指向 SysConfig 生成的原始宏) ── */
#define LED_PORT    GPIO_LEDS_PORT
#define LED_PIN     GPIO_LEDS_USER_LED_1_PIN

/* ── UART0 别名宏 ── */
#define UART0_INST  UART_0_INST

/* ═══════════════════════════════════════════════════════════════════════════
 *  预留引脚 (电赛常用外设规划)
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *  ┌──────────┬──────────┬───────────────────────────┐
 *  │ 功能     │ 建议引脚  │ 备注                      │
 *  ├──────────┼──────────┼───────────────────────────┤
 *  │ UART TX  │ PA10     │ UART0 (已占用)             │
 *  │ UART RX  │ PA11     │ UART0 (已占用)             │
 *  │ UART TX  │ PA8      │ UART1 (备用)               │
 *  │ UART RX  │ PA9      │ UART1 (备用)               │
 *  │ I2C SDA  │ PA0      │ I2C0, JY61P 姿态传感器     │
 *  │ I2C SCL  │ PA1      │ I2C0, JY61P 姿态传感器     │
 *  │ I2C SDA  │ PB2      │ I2C0, OLED / 传感器 (预留) │
 *  │ I2C SCL  │ PB3      │ I2C0, OLED / 传感器 (预留) │
 *  │ ADC0     │ PA25     │ ADC12, 电池电压采样        │
 *  │ PWM      │ PA12     │ TIMA0, 舵机 / 电机控制     │
 *  │ GPIO 按键│ PA0      │ 外部中断, 矩阵键盘         │
 *  │ SPI MOSI │ PB6      │ SPI0, LCD / NRF24L01       │
 *  │ SPI MISO │ PB7      │ SPI0, LCD / NRF24L01       │
 *  │ SPI SCK  │ PB4      │ SPI0, LCD / NRF24L01       │
 *  └──────────┴──────────┴───────────────────────────┘
 *
 *  全部可用引脚详见 MSPM0G3507 数据手册或 SysConfig GUI。
 *  修改引脚分配后，请同步更新 syscfg 工程和此表。
 */

#ifdef __cplusplus
}
#endif

#endif /* BOARD_PINS_H */
