/*
 * ============ bsp_button.c =============
 *
 * 状态机基于调用周期累加计时:
 *   建议周期 = 20ms → 每次累加 20ms
 *   去抖 40ms = 2 次调用
 *   短按 < 500ms, 长按 >= 500ms
 *   HOLD: 500ms 首次 + 每 200ms 重复
 *
 * 注意: 计时精度依赖调用周期。若实际周期偏离 20ms, 阈值会偏移。
 */

#include "bsp_button.h"
#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>

/* ── 时序参数 (ms, 基于 20ms 调用周期) ── */
#define SCAN_PERIOD_MS   20      /* 期望调用周期                      */
#define DEBOUNCE_MS      40      /* 去抖时间                          */
#define LONG_PRESS_MS    500     /* 长按阈值                          */
#define HOLD_REPEAT_MS   200     /* HOLD 重复间隔                      */

/* ── 按键 GPIO 表 ── */
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

/* ── 状态变量 ── */
static uint8_t  btn_state[BTN_COUNT];        /* 0=释放, 1=按下(已去抖)  */
static uint16_t btn_ticks[BTN_COUNT];        /* 按下累计 ms              */
static uint16_t btn_debounce[BTN_COUNT];     /* 去抖累计 ms              */
static uint16_t btn_last_hold[BTN_COUNT];    /* 上次 HOLD 时的 ticks     */
static bool     btn_hold_fired[BTN_COUNT];   /* 本次按下是否已发 HOLD    */
static uint8_t  g_evt_id;                   /* 最新事件按键 ID           */
static uint8_t  g_evt;                      /* 最新事件类型              */

/* ── BSP_Button_ID ── */
uint8_t BSP_Button_ID(void)
{
    return g_evt_id;
}

/* ── BSP_Button_Scan ── */
uint8_t BSP_Button_Scan(void)
{
    g_evt = BTN_EVT_NONE;

    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        uint8_t pressed = (DL_GPIO_readPins(btn_pins[i].port,
                                             btn_pins[i].pin) == 0);

        if (pressed) {
            /* ── 按下 ── */
            if (btn_state[i] == 0) {
                /* 前沿去抖 */
                btn_debounce[i] += SCAN_PERIOD_MS;
                if (btn_debounce[i] >= DEBOUNCE_MS) {
                    btn_state[i]       = 1;
                    btn_ticks[i]       = 0;
                    btn_debounce[i]    = 0;
                    btn_hold_fired[i]  = false;
                    btn_last_hold[i]   = 0;
                    g_evt    = BTN_EVT_PRESS_DOWN;
                    g_evt_id = i;
                    return g_evt;
                }
            } else {
                /* 持续按下 */
                btn_ticks[i] += SCAN_PERIOD_MS;

                if (btn_ticks[i] >= LONG_PRESS_MS) {
                    if (!btn_hold_fired[i]) {
                        /* 首次 HOLD */
                        btn_hold_fired[i] = true;
                        btn_last_hold[i]  = btn_ticks[i];
                        g_evt    = BTN_EVT_HOLD;
                        g_evt_id = i;
                        return g_evt;
                    } else if ((btn_ticks[i] - btn_last_hold[i])
                                >= HOLD_REPEAT_MS) {
                        /* 重复 HOLD */
                        btn_last_hold[i]  = btn_ticks[i];
                        g_evt    = BTN_EVT_HOLD;
                        g_evt_id = i;
                        return g_evt;
                    }
                }
            }
        } else {
            /* ── 释放 ── */
            btn_debounce[i] = 0;

            if (btn_state[i] == 1) {
                if (btn_ticks[i] >= LONG_PRESS_MS)
                    g_evt = BTN_EVT_LONG;
                else
                    g_evt = BTN_EVT_SHORT;

                g_evt_id          = i;
                btn_state[i]      = 0;
                btn_ticks[i]      = 0;
                btn_hold_fired[i] = false;
                return g_evt;
            }
        }
    }

    return g_evt;
}
