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

#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_wwdt.h>

#include "interrupt_priorities.h"
#include "bsp_uart.h"
#include "bsp_system.h"

#include "tasks/task_led.h"
#include "tasks/task_sensor.h"
#include "tasks/task_lcd.h"
#include "tasks/task_motor.h"

#include "app/app_flash.h"
#include "tasks/task_setup.h"

/* ════════════ main ════════════ */
int main(void)
{
    /* ── 1. 硬件初始化 ── */
    SYSCFG_DL_init();
    DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);

    /* ── 看门狗: ~8s 超时 (LFCLK=32768Hz) ── */
    DL_WWDT_enablePower(WWDT0);
    DL_WWDT_initWatchdogMode(WWDT0,
        DL_WWDT_CLOCK_DIVIDE_1, DL_WWDT_TIMER_PERIOD_18_BITS,
        DL_WWDT_RUN_IN_SLEEP,
        DL_WWDT_WINDOW_PERIOD_0, DL_WWDT_WINDOW_PERIOD_0);
    DL_WWDT_restart(WWDT0);

    BSP_UART_tx_str("\r\n=== System Boot ===\r\n");

    /* ── 2. 中断优先级 ── */
    NVIC_SetPriority(DMA_INT_IRQn,      PRIO_DMA_CH);
    NVIC_SetPriority(SPI1_INT_IRQn,     PRIO_SPI_LCD);
    NVIC_SetPriority(I2C0_INT_IRQn,     PRIO_I2C_IMU);
    NVIC_SetPriority(I2C1_INT_IRQn,     PRIO_I2C_GRAY);
    NVIC_SetPriority(GPIOA_INT_IRQn,    PRIO_ENCODER_GPIO);
    NVIC_SetPriority(TIMG7_INT_IRQn,    PRIO_TIMER_CAP);
    NVIC_SetPriority(TIMG8_INT_IRQn,    PRIO_TIMER_CAP);
    NVIC_SetPriority(TIMA1_INT_IRQn,    PRIO_UNUSED);
    DL_UART_Main_disableInterrupt(UART_0_INST, DL_UART_MAIN_INTERRUPT_RX);
    BSP_delay_ms(1000);

    /* ── 3. Flash + 按键配置 ── */
    app_flash_config_t cfg;

    if (app_flash_init()) {
        app_flash_loadConfig(&cfg);
        BSP_UART_tx_str("[Boot] Config loaded\r\n");
    } else {
        BSP_UART_tx_str("[Boot] First boot — setup wizard\r\n");
        bool ok = false;
        for (int i = 0; i < 3 && !ok; i++) {
            ok = TaskSetup_runWizard(&cfg);
        }
        if (!ok) {
            cfg.btn_save      = BTN_SAVE_YAW_DEFAULT;
            cfg.btn_restore   = BTN_RESTORE_YAW_DEFAULT;
            cfg.btn_enc_reset = BTN_ENC_RESET_DEFAULT;
        }
    }

    /* ── 4. 创建任务 ── */
    TaskLed_create();
    QueueHandle_t sq = TaskSensor_create(&cfg);
    if (sq) TaskLcd_create(sq);
    TaskMotor_create();

    /* ── 5. 启动调度器 ── */
    BSP_UART_tx_str("=== Scheduler Start ===\r\n");
    vTaskStartScheduler();
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
