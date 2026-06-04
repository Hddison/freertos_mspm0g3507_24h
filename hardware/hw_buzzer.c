/*
 *  ============ hw_buzzer.c =============
 *  蜂鸣器驱动 — PB22, GPIO 已由 SysConfig 配置
 */

#include "hw_buzzer.h"
#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>
#include "bsp_system.h"

/* 蜂鸣模式定义 */
const uint16_t BUZZ_VERTEX[] = {100, 100, 100, 100, 100, 0};
const uint16_t BUZZ_DONE[]   = {500, 200, 1000, 0};
const uint16_t BUZZ_ERROR[]  = {500, 0};

static bool g_buzzer_enabled = true;

void Buzzer_init(void)
{
    /* GPIO 已由 SysConfig 配置, 仅初始化软件状态 */
    g_buzzer_enabled = true;
}

void Buzzer_set(uint8_t on)
{
    if (!g_buzzer_enabled && on) return;
    if (on) {
        DL_GPIO_setPins(GPIO_BEEP_PORT, GPIO_BEEP_PIN_BEEP_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_BEEP_PORT, GPIO_BEEP_PIN_BEEP_PIN);
    }
}

void Buzzer_beep(uint16_t dur_ms)
{
    if (!g_buzzer_enabled) return;
    Buzzer_set(1);
    BSP_delay_ms(dur_ms);
    Buzzer_set(0);
}

void Buzzer_pattern(const uint16_t *pat, uint8_t n)
{
    if (!g_buzzer_enabled || !pat) return;
    /* pat[0]=on, pat[1]=off, pat[2]=on, ...  0=终止 */
    for (uint8_t i = 0; i < n; i++) {
        if (pat[i] == 0) break;
        if (i & 1) {
            Buzzer_set(0);
            BSP_delay_ms(pat[i]);
        } else {
            Buzzer_set(1);
            BSP_delay_ms(pat[i]);
            Buzzer_set(0);
        }
    }
    Buzzer_set(0);
}

void Buzzer_enable(bool en)  { g_buzzer_enabled = en; }
bool Buzzer_isEnabled(void)  { return g_buzzer_enabled; }
