/*
 * ============ bsp_system.h =============
 * 系统时钟工具
 *
 *   BSP_delay_ms() — 阻塞毫秒延迟 (DL_Common_delayCycles)
 *   BSP_clock_hz() — 返回 CPUCLK_FREQ
 */

#ifndef BSP_SYSTEM_H
#define BSP_SYSTEM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 阻塞延迟 ms 毫秒。使用 CPU 周期忙等, 不依赖 SysTick。
 * 注意: 在 FreeRTOS 任务中会阻塞所有低优先级任务。 */
void BSP_delay_ms(uint32_t ms);

/* 返回 CPU 主频 (Hz), 来自 SysConfig 的 CPUCLK_FREQ */
uint32_t BSP_clock_hz(void);

#ifdef __cplusplus
}
#endif

#endif
