/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define GPIO_HFXT_PORT                                                     GPIOA
#define GPIO_HFXIN_PIN                                             DL_GPIO_PIN_5
#define GPIO_HFXIN_IOMUX                                         (IOMUX_PINCM10)
#define GPIO_HFXOUT_PIN                                            DL_GPIO_PIN_6
#define GPIO_HFXOUT_IOMUX                                        (IOMUX_PINCM11)
#define CPUCLK_FREQ                                                     80000000
/* Defines for SYSPLL_ERR_01 Workaround */
/* Represent 1.000 as 1000 */
#define FLOAT_TO_INT_SCALE                                               (1000U)
#define FCC_EXPECTED_RATIO                                                  2500
#define FCC_UPPER_BOUND                       (FCC_EXPECTED_RATIO * (1 + 0.003))
#define FCC_LOWER_BOUND                       (FCC_EXPECTED_RATIO * (1 - 0.003))

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);


/* Defines for PWM_MOTOR */
#define PWM_MOTOR_INST                                                     TIMA1
#define PWM_MOTOR_INST_IRQHandler                               TIMA1_IRQHandler
#define PWM_MOTOR_INST_INT_IRQN                                 (TIMA1_INT_IRQn)
#define PWM_MOTOR_INST_CLK_FREQ                                         80000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_MOTOR_C0_PORT                                             GPIOA
#define GPIO_PWM_MOTOR_C0_PIN                                     DL_GPIO_PIN_17
#define GPIO_PWM_MOTOR_C0_IOMUX                                  (IOMUX_PINCM39)
#define GPIO_PWM_MOTOR_C0_IOMUX_FUNC                 IOMUX_PINCM39_PF_TIMA1_CCP0
#define GPIO_PWM_MOTOR_C0_IDX                                DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_MOTOR_C1_PORT                                             GPIOA
#define GPIO_PWM_MOTOR_C1_PIN                                     DL_GPIO_PIN_16
#define GPIO_PWM_MOTOR_C1_IOMUX                                  (IOMUX_PINCM38)
#define GPIO_PWM_MOTOR_C1_IOMUX_FUNC                 IOMUX_PINCM38_PF_TIMA1_CCP1
#define GPIO_PWM_MOTOR_C1_IDX                                DL_TIMER_CC_1_INDEX



/* Defines for CAP_MOTOR1 */
#define CAP_MOTOR1_INST                                                  (TIMG8)
#define CAP_MOTOR1_INST_IRQHandler                              TIMG8_IRQHandler
#define CAP_MOTOR1_INST_INT_IRQN                                (TIMG8_INT_IRQn)
#define CAP_MOTOR1_INST_LOAD_VALUE                                       (4999U)
/* GPIO defines for channel 0 */
#define GPIO_CAP_MOTOR1_C0_PORT                                            GPIOA
#define GPIO_CAP_MOTOR1_C0_PIN                                    DL_GPIO_PIN_26
#define GPIO_CAP_MOTOR1_C0_IOMUX                                 (IOMUX_PINCM59)
#define GPIO_CAP_MOTOR1_C0_IOMUX_FUNC                IOMUX_PINCM59_PF_TIMG8_CCP0
/* GPIO defines for channel 1 */
#define GPIO_CAP_MOTOR1_C1_PORT                                            GPIOA
#define GPIO_CAP_MOTOR1_C1_PIN                                    DL_GPIO_PIN_27
#define GPIO_CAP_MOTOR1_C1_IOMUX                                 (IOMUX_PINCM60)
#define GPIO_CAP_MOTOR1_C1_IOMUX_FUNC                IOMUX_PINCM60_PF_TIMG8_CCP1

/* Defines for CAP_MOTOR2 */
#define CAP_MOTOR2_INST                                                  (TIMG7)
#define CAP_MOTOR2_INST_IRQHandler                              TIMG7_IRQHandler
#define CAP_MOTOR2_INST_INT_IRQN                                (TIMG7_INT_IRQn)
#define CAP_MOTOR2_INST_LOAD_VALUE                                       (9999U)
/* GPIO defines for channel 0 */
#define GPIO_CAP_MOTOR2_C0_PORT                                            GPIOA
#define GPIO_CAP_MOTOR2_C0_PIN                                    DL_GPIO_PIN_28
#define GPIO_CAP_MOTOR2_C0_IOMUX                                  (IOMUX_PINCM3)
#define GPIO_CAP_MOTOR2_C0_IOMUX_FUNC                 IOMUX_PINCM3_PF_TIMG7_CCP0
/* GPIO defines for channel 1 */
#define GPIO_CAP_MOTOR2_C1_PORT                                            GPIOA
#define GPIO_CAP_MOTOR2_C1_PIN                                    DL_GPIO_PIN_31
#define GPIO_CAP_MOTOR2_C1_IOMUX                                  (IOMUX_PINCM6)
#define GPIO_CAP_MOTOR2_C1_IOMUX_FUNC                 IOMUX_PINCM6_PF_TIMG7_CCP1






/* Defines for I2C_0 */
#define I2C_0_INST                                                          I2C0
#define I2C_0_INST_IRQHandler                                    I2C0_IRQHandler
#define I2C_0_INST_INT_IRQN                                        I2C0_INT_IRQn
#define I2C_0_BUS_SPEED_HZ                                               1000000
#define GPIO_I2C_0_SDA_PORT                                                GPIOA
#define GPIO_I2C_0_SDA_PIN                                         DL_GPIO_PIN_0
#define GPIO_I2C_0_IOMUX_SDA                                      (IOMUX_PINCM1)
#define GPIO_I2C_0_IOMUX_SDA_FUNC                       IOMUX_PINCM1_PF_I2C0_SDA
#define GPIO_I2C_0_SCL_PORT                                                GPIOA
#define GPIO_I2C_0_SCL_PIN                                         DL_GPIO_PIN_1
#define GPIO_I2C_0_IOMUX_SCL                                      (IOMUX_PINCM2)
#define GPIO_I2C_0_IOMUX_SCL_FUNC                       IOMUX_PINCM2_PF_I2C0_SCL

/* Defines for I2C_NCHD12 */
#define I2C_NCHD12_INST                                                     I2C1
#define I2C_NCHD12_INST_IRQHandler                               I2C1_IRQHandler
#define I2C_NCHD12_INST_INT_IRQN                                   I2C1_INT_IRQn
#define I2C_NCHD12_BUS_SPEED_HZ                                           100000
#define GPIO_I2C_NCHD12_SDA_PORT                                           GPIOB
#define GPIO_I2C_NCHD12_SDA_PIN                                    DL_GPIO_PIN_3
#define GPIO_I2C_NCHD12_IOMUX_SDA                                (IOMUX_PINCM16)
#define GPIO_I2C_NCHD12_IOMUX_SDA_FUNC                 IOMUX_PINCM16_PF_I2C1_SDA
#define GPIO_I2C_NCHD12_SCL_PORT                                           GPIOA
#define GPIO_I2C_NCHD12_SCL_PIN                                   DL_GPIO_PIN_29
#define GPIO_I2C_NCHD12_IOMUX_SCL                                 (IOMUX_PINCM4)
#define GPIO_I2C_NCHD12_IOMUX_SCL_FUNC                  IOMUX_PINCM4_PF_I2C1_SCL


/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                           40000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_0_FBRD_40_MHZ_115200_BAUD                                      (45)




/* Defines for SPI_LCD */
#define SPI_LCD_INST                                                       SPI1
#define SPI_LCD_INST_IRQHandler                                 SPI1_IRQHandler
#define SPI_LCD_INST_INT_IRQN                                     SPI1_INT_IRQn
#define GPIO_SPI_LCD_PICO_PORT                                            GPIOB
#define GPIO_SPI_LCD_PICO_PIN                                     DL_GPIO_PIN_8
#define GPIO_SPI_LCD_IOMUX_PICO                                 (IOMUX_PINCM25)
#define GPIO_SPI_LCD_IOMUX_PICO_FUNC                 IOMUX_PINCM25_PF_SPI1_PICO
#define GPIO_SPI_LCD_POCI_PORT                                            GPIOB
#define GPIO_SPI_LCD_POCI_PIN                                     DL_GPIO_PIN_7
#define GPIO_SPI_LCD_IOMUX_POCI                                 (IOMUX_PINCM24)
#define GPIO_SPI_LCD_IOMUX_POCI_FUNC                 IOMUX_PINCM24_PF_SPI1_POCI
/* GPIO configuration for SPI_LCD */
#define GPIO_SPI_LCD_SCLK_PORT                                            GPIOB
#define GPIO_SPI_LCD_SCLK_PIN                                     DL_GPIO_PIN_9
#define GPIO_SPI_LCD_IOMUX_SCLK                                 (IOMUX_PINCM26)
#define GPIO_SPI_LCD_IOMUX_SCLK_FUNC                 IOMUX_PINCM26_PF_SPI1_SCLK



/* Defines for DMA_I2C_TX */
#define DMA_I2C_TX_CHAN_ID                                                   (3)
#define I2C_0_INST_DMA_TRIGGER_0                              (DMA_I2C0_TX_TRIG)
/* Defines for DMA_I2C_RX */
#define DMA_I2C_RX_CHAN_ID                                                   (2)
#define I2C_0_INST_DMA_TRIGGER_1                              (DMA_I2C0_RX_TRIG)
/* Defines for DMA_CH0 */
#define DMA_CH0_CHAN_ID                                                      (1)
#define I2C_NCHD12_INST_DMA_TRIGGER                           (DMA_I2C1_RX_TRIG)
/* Defines for DMA_SPI_LCD_TX */
#define DMA_SPI_LCD_TX_CHAN_ID                                               (0)
#define SPI_LCD_INST_DMA_TRIGGER                              (DMA_SPI1_TX_TRIG)


/* Port definition for Pin Group GPIO_LEDS */
#define GPIO_LEDS_PORT                                                   (GPIOB)

/* Defines for USER_LED_1: GPIOB.20 with pinCMx 48 on package pin 19 */
#define GPIO_LEDS_USER_LED_1_PIN                                (DL_GPIO_PIN_20)
#define GPIO_LEDS_USER_LED_1_IOMUX                               (IOMUX_PINCM48)
/* Port definition for Pin Group GPIO_W25Q */
#define GPIO_W25Q_PORT                                                   (GPIOB)

/* Defines for W_CS: GPIOB.6 with pinCMx 23 on package pin 58 */
#define GPIO_W25Q_W_CS_PIN                                       (DL_GPIO_PIN_6)
#define GPIO_W25Q_W_CS_IOMUX                                     (IOMUX_PINCM23)
/* Defines for PIN_UP: GPIOA.24 with pinCMx 54 on package pin 25 */
#define GPIO_KEY_PIN_UP_PORT                                             (GPIOA)
#define GPIO_KEY_PIN_UP_PIN                                     (DL_GPIO_PIN_24)
#define GPIO_KEY_PIN_UP_IOMUX                                    (IOMUX_PINCM54)
/* Defines for PIN_LEFT: GPIOA.25 with pinCMx 55 on package pin 26 */
#define GPIO_KEY_PIN_LEFT_PORT                                           (GPIOA)
#define GPIO_KEY_PIN_LEFT_PIN                                   (DL_GPIO_PIN_25)
#define GPIO_KEY_PIN_LEFT_IOMUX                                  (IOMUX_PINCM55)
/* Defines for PIN_DOWN: GPIOB.24 with pinCMx 52 on package pin 23 */
#define GPIO_KEY_PIN_DOWN_PORT                                           (GPIOB)
#define GPIO_KEY_PIN_DOWN_PIN                                   (DL_GPIO_PIN_24)
#define GPIO_KEY_PIN_DOWN_IOMUX                                  (IOMUX_PINCM52)
/* Defines for PIN_RIGHT: GPIOB.25 with pinCMx 56 on package pin 27 */
#define GPIO_KEY_PIN_RIGHT_PORT                                          (GPIOB)
#define GPIO_KEY_PIN_RIGHT_PIN                                  (DL_GPIO_PIN_25)
#define GPIO_KEY_PIN_RIGHT_IOMUX                                 (IOMUX_PINCM56)
/* Defines for PIN_CENTER: GPIOA.22 with pinCMx 47 on package pin 18 */
#define GPIO_KEY_PIN_CENTER_PORT                                         (GPIOA)
#define GPIO_KEY_PIN_CENTER_PIN                                 (DL_GPIO_PIN_22)
#define GPIO_KEY_PIN_CENTER_IOMUX                                (IOMUX_PINCM47)
/* Defines for PIN_BUTTON: GPIOB.21 with pinCMx 49 on package pin 20 */
#define GPIO_KEY_PIN_BUTTON_PORT                                         (GPIOB)
#define GPIO_KEY_PIN_BUTTON_PIN                                 (DL_GPIO_PIN_21)
#define GPIO_KEY_PIN_BUTTON_IOMUX                                (IOMUX_PINCM49)
/* Port definition for Pin Group GPIO_LCD */
#define GPIO_LCD_PORT                                                    (GPIOB)

/* Defines for LCD_RES: GPIOB.10 with pinCMx 27 on package pin 62 */
#define GPIO_LCD_LCD_RES_PIN                                    (DL_GPIO_PIN_10)
#define GPIO_LCD_LCD_RES_IOMUX                                   (IOMUX_PINCM27)
/* Defines for LCD_DC: GPIOB.11 with pinCMx 28 on package pin 63 */
#define GPIO_LCD_LCD_DC_PIN                                     (DL_GPIO_PIN_11)
#define GPIO_LCD_LCD_DC_IOMUX                                    (IOMUX_PINCM28)
/* Defines for LCD_CS: GPIOB.14 with pinCMx 31 on package pin 2 */
#define GPIO_LCD_LCD_CS_PIN                                     (DL_GPIO_PIN_14)
#define GPIO_LCD_LCD_CS_IOMUX                                    (IOMUX_PINCM31)
/* Defines for LCD_BLK: GPIOB.26 with pinCMx 57 on package pin 28 */
#define GPIO_LCD_LCD_BLK_PIN                                    (DL_GPIO_PIN_26)
#define GPIO_LCD_LCD_BLK_IOMUX                                   (IOMUX_PINCM57)
/* Port definition for Pin Group GPIO_MOTOR */
#define GPIO_MOTOR_PORT                                                  (GPIOA)

/* Defines for BIN1: GPIOA.12 with pinCMx 34 on package pin 5 */
#define GPIO_MOTOR_BIN1_PIN                                     (DL_GPIO_PIN_12)
#define GPIO_MOTOR_BIN1_IOMUX                                    (IOMUX_PINCM34)
/* Defines for BIN2: GPIOA.13 with pinCMx 35 on package pin 6 */
#define GPIO_MOTOR_BIN2_PIN                                     (DL_GPIO_PIN_13)
#define GPIO_MOTOR_BIN2_IOMUX                                    (IOMUX_PINCM35)
/* Defines for AIN1: GPIOA.14 with pinCMx 36 on package pin 7 */
#define GPIO_MOTOR_AIN1_PIN                                     (DL_GPIO_PIN_14)
#define GPIO_MOTOR_AIN1_IOMUX                                    (IOMUX_PINCM36)
/* Defines for AIN2: GPIOA.15 with pinCMx 37 on package pin 8 */
#define GPIO_MOTOR_AIN2_PIN                                     (DL_GPIO_PIN_15)
#define GPIO_MOTOR_AIN2_IOMUX                                    (IOMUX_PINCM37)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);
void SYSCFG_DL_PWM_MOTOR_init(void);
void SYSCFG_DL_CAP_MOTOR1_init(void);
void SYSCFG_DL_CAP_MOTOR2_init(void);
void SYSCFG_DL_I2C_0_init(void);
void SYSCFG_DL_I2C_NCHD12_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_SPI_LCD_init(void);
void SYSCFG_DL_DMA_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
