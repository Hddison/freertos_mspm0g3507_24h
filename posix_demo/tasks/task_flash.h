/*
 *  ============ task_flash.h =============
 *  W25Q128 Flash 测试任务 — DMA 读写参考
 *
 *  上电后读取 Flash ID 验证通信,
 *  可通过按键触发读写测试.
 */

#ifndef TASK_FLASH_H
#define TASK_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

void TaskFlash_create(void);

#ifdef __cplusplus
}
#endif

#endif
