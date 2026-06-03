/*
 *  ============ task_setup.c =============
 *  串口交互按键绑定向导, 阻塞式运行
 */

#include "task_setup.h"

#include "bsp_uart.h"
#include "bsp_system.h"
#include "bsp_button.h"

static const char *_name(uint8_t id)
{
    static const char *n[] = {"UP","LEFT","DOWN","RIGHT","CENTER","BUTTON"};
    return (id < 6) ? n[id] : "?";
}

static uint8_t _waitBtn(const char *prompt)
{
    BSP_UART_tx_str(prompt);
    while (BSP_Button_Scan() != BTN_EVT_NONE) BSP_delay_ms(50);
    uint8_t id;
    for (;;) {
        uint8_t evt = BSP_Button_Scan();
        if (evt == BTN_EVT_SHORT || evt == BTN_EVT_LONG) {
            id = BSP_Button_ID();
            break;
        }
        BSP_delay_ms(50);
    }
    BSP_UART_tx_str(" [");
    BSP_UART_tx_str(_name(id));
    BSP_UART_tx_str("]\r\n");
    BSP_delay_ms(500);
    while (BSP_Button_Scan() != BTN_EVT_NONE) BSP_delay_ms(50);
    return id;
}

bool TaskSetup_runWizard(app_flash_config_t *cfg)
{
    BSP_UART_tx_str("\r\n=== Button Binding Setup ===\r\n");

    cfg->btn_save    = _waitBtn("1/3 Press [SAVE YAW]...");
    cfg->btn_restore = _waitBtn("2/3 Press [RESTORE YAW]...");
    cfg->btn_enc_reset = _waitBtn("3/3 Press [ENC RESET]...");
    cfg->reserved = 0;

    BSP_UART_tx_str("Binding: SAVE=");
    BSP_UART_tx_str(_name(cfg->btn_save));
    BSP_UART_tx_str(" RST=");
    BSP_UART_tx_str(_name(cfg->btn_restore));
    BSP_UART_tx_str(" ENC=");
    BSP_UART_tx_str(_name(cfg->btn_enc_reset));
    BSP_UART_tx_str("\r\nSaving... ");

    if (app_flash_saveConfig(cfg)) {
        BSP_UART_tx_str("OK\r\n");
        return true;
    }
    BSP_UART_tx_str("FAIL\r\n");
    return false;
}
