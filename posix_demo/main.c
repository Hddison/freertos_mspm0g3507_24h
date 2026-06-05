/*
 * main.c — 小车服务层入口: 硬件初始化 + 任务启动
 * 2024 电赛 H 题 — 自动行驶小车
 */

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>

#include <string.h>

#include "bsp_uart.h"
#include "bsp_system.h"
#include "bsp_i2c.h"

#include "hal_spi.h"

#include "hw_buzzer.h"
#include "hw_st7789.h"
#include "hw_motor.h"

#include "gui_paint.h"      /* BLACK, GREEN, CYAN, WHITE color macros */

#include "app_config.h"
#include "app_flash.h"

#include "task_button.h"

/* ── 外部声明 (定义在 task_sensor.c) ── */
extern bool g_buzzer_enabled;
extern bool g_led_heartbeat_enabled;
extern uint8_t g_btn_remap[6];

/* ══════════ FreeRTOS Hooks ══════════ */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    BSP_UART_tx_str("STACK OVERFLOW: ");
    BSP_UART_tx_str(pcTaskName);
    for (;;) {}
}

void HardFault_Handler(void)
{
    BSP_UART_tx_str("\r\n!!! HardFault !!!\r\n");

    /* 读取 stacked PC (Cortex-M0+ 自动压栈: R0-R3,R12,LR,PC,xPSR)
     * PSP[6] = PC, PSP[5] = LR  */
    uint32_t *psp;
    __asm volatile ("MRS %0, PSP" : "=r"(psp));
    uint32_t stacked_pc = psp[6];
    uint32_t stacked_lr = psp[5];

    BSP_UART_tx_str("PC=");
    for (int s = 28; s >= 0; s -= 4) {
        uint8_t n = (stacked_pc >> s) & 0xF;
        BSP_UART_tx_byte(n < 10 ? '0' + n : 'A' + n - 10);
    }
    BSP_UART_tx_str(" LR=");
    for (int s = 28; s >= 0; s -= 4) {
        uint8_t n = (stacked_lr >> s) & 0xF;
        BSP_UART_tx_byte(n < 10 ? '0' + n : 'A' + n - 10);
    }
    BSP_UART_tx_str("\r\n");
    for (;;) {}
}

/* ══════════ Flash 配置全局 ══════════ */

flash_config_t g_flash_cfg;

/* ══════════ main ══════════ */

int main(void)
{
    /* Step 1: SysConfig 全外设初始化 */
    SYSCFG_DL_init();

    /* Step 2: 禁用未处理的硬件中断 (防止触发未定义 ISR → HardFault)
     * UART0 RX 中断由 SysConfig 可能使能, 我们用 CPU 轮询 BSP_UART */
    NVIC_DisableIRQ(UART_0_INST_INT_IRQN);

    /* Step 3: W25Q128 CS = HIGH (防止误操作) */
    DL_GPIO_setPins(GPIO_W25Q_PORT, GPIO_W25Q_W_CS_PIN);

    /* Step 3: NVIC 优先级 (SysConfig 已设置基础值, 此处按 interrupt_priorities.h 微调) */
    /* DMA/SPI → PRIO_COMM_HIGH(1), I2C/GPIO → PRIO_SENSOR(2) */

    /* Step 4: 外设稳定延时 */
    BSP_delay_ms(1000);

    /* Step 5: UART 启动信息 */
    BSP_UART_tx_str("\r\n==================================\r\n");
    BSP_UART_tx_str("MSPM0G3507 Car Service Layer v2.0\r\n");
    BSP_UART_tx_str("2024 E-Contest H — Auto Car\r\n");
    BSP_UART_tx_str("==================================\r\n");

    /* Step 6: 加载 / 初始化 Flash 配置 */
    if (!flash_config_load(&g_flash_cfg)) {
        BSP_UART_tx_str("[FLASH] No valid config, writing defaults\r\n");
        flash_config_defaults(&g_flash_cfg);
        flash_config_save(&g_flash_cfg);
    } else {
        BSP_UART_tx_str("[FLASH] Config loaded OK\r\n");
    }

    /* Step 7: 应用 Flash 配置 */
    g_buzzer_enabled        = (g_flash_cfg.flags & FLASH_FLAG_BUZZER_EN) != 0;
    g_led_heartbeat_enabled = true;
    memcpy(g_btn_remap, g_flash_cfg.btn_remap, 6);

    /* Step 8: 电机初始化 (GPIO 方向 + PWM + 编码器 ISR) */
    Motor_init();
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
    BSP_UART_tx_str("[MOTOR] Initialized\r\n");

    /* Step 9: LCD 初始化 + Splash */
    ST7789_init(ST7789_HORIZONTAL);
    ST7789_backLight(1);
    ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);

    extern const uint8_t Font8_Table[];
    ST7789_drawStringFast(10, 100, "MSPM0G3507 Car", Font8_Table, 6, 10, GREEN, BLACK);
    ST7789_drawStringFast(10, 120, "2024 E-Contest H", Font8_Table, 6, 10, CYAN, BLACK);
    ST7789_drawStringFast(10, 140, "Auto Driving Car", Font8_Table, 6, 10, WHITE, BLACK);
    BSP_delay_ms(1500);
    BSP_UART_tx_str("[LCD] Initialized\r\n");

    /* Step 10: 蜂鸣器初始化 */
    Buzzer_init();
    BSP_UART_tx_str("[BUZZ] Initialized\r\n");

    /* Step 11: HAL SPI 互斥锁 */
    HAL_SPI_init();

    /* Step 12: I2C 初始化 (IMU + 灰度) */
    BSP_I2C_init();

    /* Step 13: 创建队列 */
    g_button_queue = xQueueCreate(BUTTON_QUEUE_LEN, sizeof(button_event_t));
    g_cmd_queue    = xQueueCreate(CMD_QUEUE_LEN,     sizeof(ctrl_cmd_t));
    g_sensor_queue = xQueueCreate(SENSOR_QUEUE_LEN,  sizeof(sensor_data_t));

    if (!g_button_queue || !g_cmd_queue || !g_sensor_queue) {
        BSP_UART_tx_str("FATAL: Queue creation failed\r\n");
        for (;;) {}
    }

    /* Step 14: 创建任务 (优先级从低到高) */

    /* Button 任务: 静态分配栈 (节省 heap) */
    static StackType_t button_stack[STACK_BUTTON];
    static StaticTask_t button_tcb;
    TaskHandle_t h = xTaskCreateStatic(vTaskButton, "BUTTON", STACK_BUTTON, NULL,
                                       TASK_PRIO_BUTTON, button_stack, &button_tcb);
    if (h == NULL) { BSP_UART_tx_str("FATAL: Button task\r\n"); for(;;){} }

    /* LCD 任务: 静态分配栈 */
    static StackType_t lcd_stack[STACK_LCD];
    static StaticTask_t lcd_tcb;
    h = xTaskCreateStatic(vTaskLcd, "LCD", STACK_LCD, NULL,
                          TASK_PRIO_LCD, lcd_stack, &lcd_tcb);
    if (h == NULL) { BSP_UART_tx_str("FATAL: LCD task\r\n"); for(;;){} }

    /* Control 任务: 静态分配栈 */
    static StackType_t control_stack[STACK_CONTROL];
    static StaticTask_t control_tcb;
    h = xTaskCreateStatic(vTaskControl, "CONTROL", STACK_CONTROL, NULL,
                          TASK_PRIO_CONTROL, control_stack, &control_tcb);
    if (h == NULL) { BSP_UART_tx_str("FATAL: Control task\r\n"); for(;;){} }

    /* Sensor 任务: heap 分配 (最高优先级) */
    BaseType_t ret = xTaskCreate(vTaskSensor, "SENSOR", STACK_SENSOR, NULL,
                                 TASK_PRIO_SENSOR, NULL);
    if (ret != pdPASS) { BSP_UART_tx_str("FATAL: Sensor task\r\n"); for(;;){} }

    BSP_UART_tx_str("[RTOS] All 4 tasks created\r\n");
    BSP_UART_tx_str("[RTOS] Starting scheduler...\r\n");

    /* Step 15: 启动调度器 */
    vTaskStartScheduler();

    /* 不应到达这里 */
    for (;;) {}
}
