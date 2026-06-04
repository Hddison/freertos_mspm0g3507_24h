/*
 * ============ hal_spi.c =============
 * HAL 层 — SPI 总线互斥 (FreeRTOS mutex)
 * 初始化必须在调度器启动前调用 HAL_SPI_init()。
 */

#include "hal_spi.h"

#include <FreeRTOS.h>
#include <semphr.h>

static SemaphoreHandle_t g_spi_mutex = NULL;

void HAL_SPI_init(void)
{
    if (g_spi_mutex == NULL) {
        g_spi_mutex = xSemaphoreCreateMutex();
    }
}

void HAL_SPI_lock(void)
{
    if (g_spi_mutex) {
        xSemaphoreTake(g_spi_mutex, portMAX_DELAY);
    }
}

void HAL_SPI_unlock(void)
{
    if (g_spi_mutex) {
        xSemaphoreGive(g_spi_mutex);
    }
}
