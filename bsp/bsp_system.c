/*
 * ============ bsp_system.c =============
 */

#include "bsp_system.h"
#include "ti_msp_dl_config.h"        /* CPUCLK_FREQ */
#include <ti/driverlib/dl_common.h>  /* DL_Common_delayCycles */

/* ── BSP_delay_ms ── */
void BSP_delay_ms(uint32_t ms)
{
    uint32_t cycles_per_ms = CPUCLK_FREQ / 1000UL;
    for (uint32_t i = 0; i < ms; i++) {
        DL_Common_delayCycles(cycles_per_ms);
    }
}

/* ── BSP_clock_hz ── */
uint32_t BSP_clock_hz(void)
{
    return CPUCLK_FREQ;
}
