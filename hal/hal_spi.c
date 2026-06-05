/*
 * ============ hal_spi.c =============
 * HAL 层 — SPI 总线互斥 (FreeRTOS 递归互斥锁)
 *
 * 使用递归锁 (Recursive Mutex):
 *   允许同一任务嵌套加锁 — HW 层内部可以再次调用 HAL_SPI_lock/unlock
 *   例如 flash_config_save() 持有锁后调用 HW_W25Q128_eraseSector()
 *   后者内部也调 HAL_SPI_lock(), 递归锁不会死锁。
 *
 * 初始化必须在调度器启动前调用 HAL_SPI_init()。
 */

#include "hal_spi.h"

#include <FreeRTOS.h>
#include <semphr.h>

static SemaphoreHandle_t g_spi_mutex = NULL;

void HAL_SPI_init(void)
{
    if (g_spi_mutex == NULL) {
        g_spi_mutex = xSemaphoreCreateRecursiveMutex();
    }
}

void HAL_SPI_lock(void)
{
    if (g_spi_mutex) {
        xSemaphoreTakeRecursive(g_spi_mutex, portMAX_DELAY);
    }
}

void HAL_SPI_unlock(void)
{
    if (g_spi_mutex) {
        xSemaphoreGiveRecursive(g_spi_mutex);
    }
}
