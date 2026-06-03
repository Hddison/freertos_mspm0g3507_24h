/*
 * FreeRTOS MSPM0G3507 — 智能小车应用入口
 *
 * main() 职责: 系统初始化 + 创建任务 + 启动调度器
 * 各模块任务见 tasks/ 目录
 *
 * 任务架构:
 *   TaskLed    (prio idle+1,  2Hz): LED 闪烁
 *   TaskSensor (prio idle+2, 20Hz): 传感器读取 → 队列
 *   TaskLcd    (prio idle+3, 20Hz): 队列接收 → LCD 渲染
 *   TaskMotor  (prio idle+4, 100Hz): 电机 PID 控制 (占位)
 */

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>

#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_spi.h>

#include "interrupt_priorities.h"
#include "bsp_uart.h"
#include "bsp_system.h"
#include "bsp_spi.h"

#include "tasks/task_led.h"
#include "tasks/task_sensor.h"
#include "tasks/task_lcd.h"
#include "tasks/task_motor.h"
#include "tasks/task_flash.h"

/* ════════════ main ════════════ */
int main(void)
{
    /* ── 1. 硬件初始化 (SysConfig 生成) ── */
    SYSCFG_DL_init();

    /* 确保 W25Q128 CS (PB6) 拉高 — SysConfig 初始化为 LOW */
    DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);

    BSP_UART_tx_str("\r\n=== System Boot ===\r\n");

    /* ── 2. 中断优先级配置 ── */
    NVIC_SetPriority(DMA_INT_IRQn,      PRIO_DMA_CH);
    NVIC_SetPriority(SPI1_INT_IRQn,     PRIO_SPI_LCD);
    NVIC_SetPriority(I2C0_INT_IRQn,     PRIO_I2C_IMU);
    NVIC_SetPriority(I2C1_INT_IRQn,     PRIO_I2C_GRAY);
    NVIC_SetPriority(GPIOA_INT_IRQn,    PRIO_ENCODER_GPIO);
    NVIC_SetPriority(TIMG7_INT_IRQn,    PRIO_TIMER_CAP);
    NVIC_SetPriority(TIMG8_INT_IRQn,    PRIO_TIMER_CAP);
    NVIC_SetPriority(TIMA1_INT_IRQn,    PRIO_UNUSED);

    /* 禁用 UART 中断, 防止上电噪声触发 Default_Handler */
    DL_UART_Main_disableInterrupt(UART_0_INST, DL_UART_MAIN_INTERRUPT_RX);

    BSP_delay_ms(1000);  /* 硬件稳定 */

    /* ── 终极诊断: 调度器启动前读 Flash ID ── */
    {
        uint16_t id;
        uint8_t  mfr, dev;

        /* 清 RX FIFO */
        while (!DL_SPI_isRXFIFOEmpty(SPI_LCD_INST)) {
            (void)DL_SPI_receiveData8(SPI_LCD_INST);
        }

        /* CS LOW → 发 0x90 → 读 ID → CS HIGH */
        DL_GPIO_clearPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);
        BSP_SPI_txrx_byte(0x90);
        BSP_SPI_txrx_byte(0x00);
        BSP_SPI_txrx_byte(0x00);
        BSP_SPI_txrx_byte(0x00);
        mfr = BSP_SPI_txrx_byte(0xFF);
        dev = BSP_SPI_txrx_byte(0xFF);
        DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);

        id = ((uint16_t)mfr << 8) | dev;

        char boot_msg[48];
        snprintf(boot_msg, sizeof(boot_msg),
            "[Boot] Flash ID: 0x%04X (MFR=%02X DEV=%02X)\r\n", id, mfr, dev);
        BSP_UART_tx_str(boot_msg);
    }

    /* ── 3. 创建任务 (按优先级从低到高) ── */
    TaskLed_create();

    QueueHandle_t sensorQueue = TaskSensor_create();
    if (sensorQueue) {
        TaskLcd_create(sensorQueue);
    } else {
        BSP_UART_tx_str("ERROR: Sensor task init failed!\r\n");
    }

    TaskMotor_create();
    TaskFlash_create();

    /* ── 4. 启动 FreeRTOS 调度器 ── */
    BSP_UART_tx_str("=== Scheduler Start ===\r\n");
    vTaskStartScheduler();

    /* 不应到达此处 */
    for (;;) {}
}

/* ════════════ 中断桩: 防止上电瞬态触发 Default_Handler ════════════ */

void UART0_IRQHandler(void)
{
    DL_UART_Main_clearInterruptStatus(UART_0_INST,
        DL_UART_MAIN_INTERRUPT_RX);
}

void DMA_IRQHandler(void)
{
    for (uint8_t ch = 0; ch < 4; ch++) {
        DL_DMA_clearInterruptStatus(DMA, ch);
    }
}

void I2C0_IRQHandler(void) { }
void I2C1_IRQHandler(void) { }
void SPI1_IRQHandler(void) { }

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pxTask;
    (void)pcTaskName;
    for (;;) {}
}
#endif
