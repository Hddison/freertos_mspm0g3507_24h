/*
 *  ============ bsp_system.h =============
 *  BSP 系统 — DL_Common_* 直调封装
 */

#ifndef BSP_SYSTEM_H
#define BSP_SYSTEM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void     BSP_delay_ms(uint32_t ms);
uint32_t BSP_clock_hz(void);

#ifdef __cplusplus
}
#endif

#endif
