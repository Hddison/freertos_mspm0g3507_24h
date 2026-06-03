/*
 *  ============ bsp_button.c =============
 *  轻量按键驱动 — 50ms 轮询, 短按 <500ms < 长按
 */

#include "bsp_button.h"
#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>

#define BTN_COUNT        6
#define DEBOUNCE_MS      40     /* 去抖时间 */
#define LONG_PRESS_MS    600    /* 长按阈值 */

/* 按键信息: GPIO 端口/引脚 (低电平按下) */
static const struct {
    GPIO_Regs *port;
    uint32_t   pin;
} btn_pins[BTN_COUNT] = {
    [BTN_ID_UP]     = { GPIO_KEY_PIN_UP_PORT,     GPIO_KEY_PIN_UP_PIN     },
    [BTN_ID_LEFT]   = { GPIO_KEY_PIN_LEFT_PORT,   GPIO_KEY_PIN_LEFT_PIN   },
    [BTN_ID_DOWN]   = { GPIO_KEY_PIN_DOWN_PORT,   GPIO_KEY_PIN_DOWN_PIN   },
    [BTN_ID_RIGHT]  = { GPIO_KEY_PIN_RIGHT_PORT,  GPIO_KEY_PIN_RIGHT_PIN  },
    [BTN_ID_CENTER] = { GPIO_KEY_PIN_CENTER_PORT, GPIO_KEY_PIN_CENTER_PIN },
    [BTN_ID_BUTTON] = { GPIO_KEY_PIN_BUTTON_PORT, GPIO_KEY_PIN_BUTTON_PIN },
};

static uint8_t  btn_state[BTN_COUNT];    /* 0=释放, 1=按下(已去抖) */
static uint16_t btn_ticks[BTN_COUNT];    /* 按下持续 ms */
static uint16_t btn_debounce[BTN_COUNT]; /* 去抖计数 ms */
static uint8_t  g_evt_id;               /* 上次事件按键 ID */
static uint8_t  g_evt;                  /* 上次事件类型 */

uint8_t BSP_Button_ID(void) { return g_evt_id; }

uint8_t BSP_Button_Scan(void)
{
    g_evt = BTN_EVT_NONE;

    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        uint8_t pressed = (DL_GPIO_readPins(btn_pins[i].port, btn_pins[i].pin) == 0);

        if (pressed) {
            if (btn_state[i] == 0) {
                /* 按下前沿: 去抖 */
                btn_debounce[i] += 50;
                if (btn_debounce[i] >= DEBOUNCE_MS) {
                    btn_state[i] = 1;
                    btn_ticks[i]  = 0;
                    btn_debounce[i] = 0;
                }
            } else {
                /* 持续按下: 累计时间 */
                btn_ticks[i] += 50;
            }
        } else {
            if (btn_state[i] == 1) {
                /* 释放: 判断短按/长按 */
                if (btn_ticks[i] >= LONG_PRESS_MS) {
                    g_evt = BTN_EVT_LONG;
                } else {
                    g_evt = BTN_EVT_SHORT;
                }
                g_evt_id   = i;
                btn_state[i] = 0;
                btn_ticks[i] = 0;
                return g_evt;
            }
            btn_debounce[i] = 0;
        }
    }

    return g_evt;
}
