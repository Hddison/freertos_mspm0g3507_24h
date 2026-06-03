/*
 *  ============ interrupt_priorities.h =============
 *  MSPM0G3507 NVIC 中断优先级分配
 *
 *  Cortex-M0+ 仅实现 2 位优先级 (4 级: 0-3)
 *    - 数值越小优先级越高 (0 = 最高)
 *    - FreeRTOS configMAX_SYSCALL_INTERRUPT_PRIORITY = 1
 *      → 优先级 1/2/3 的 ISR 可安全调用 FreeRTOS FromISR API
 *      → 优先级 0 的 ISR 禁止调用 FreeRTOS API (会抢占内核关键区)
 */

#ifndef INTERRUPT_PRIORITIES_H
#define INTERRUPT_PRIORITIES_H

#include <ti/devices/msp/msp.h>   /* NVIC_Type, IRQn_Type */

/* ── 优先级层次 (CMSIS 原始值 0-3) ── */

#define PRIO_KERNEL      0    /* SysTick / PendSV / SVC — FreeRTOS 内核独占 */
#define PRIO_COMM_HIGH   1    /* 高优先级通信: DMA, SPI (可调用 FromISR API)   */
#define PRIO_SENSOR      2    /* 传感器 & 外设: I2C, 编码器, 定时器捕获        */
#define PRIO_DEBUG       3    /* 调试 & 空闲: UART, 未使用外设                  */

/* ── 各外设优先级分配 ── */

#define PRIO_DMA_CH       PRIO_COMM_HIGH   /* DMA 通道 0-3, LCD 传输关键        */
#define PRIO_SPI_LCD      PRIO_COMM_HIGH   /* SPI1, 配合 DMA 高速刷屏            */
#define PRIO_I2C_IMU      PRIO_SENSOR      /* I2C0, JY61P 角度读取 (1MHz)       */
#define PRIO_I2C_GRAY     PRIO_SENSOR      /* I2C1, NCHD12 灰度 (100kHz)        */
#define PRIO_ENCODER_GPIO PRIO_SENSOR      /* GPIOA GROUP0, 编码器双边沿中断     */
#define PRIO_TIMER_CAP    PRIO_SENSOR      /* TIMG7/8, 编码器捕获 (备用 ISR)     */
#define PRIO_UART_DEBUG   PRIO_DEBUG       /* UART0, 调试输出 (SysConfig 已设)   */
#define PRIO_UNUSED       PRIO_DEBUG       /* TIMA1 PWM 等未使用 ISR             */

#endif /* INTERRUPT_PRIORITIES_H */
