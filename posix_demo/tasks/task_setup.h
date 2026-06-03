/*
 *  ============ task_setup.h =============
 *  首次上电按键绑定向导 (vTaskStartScheduler 前调用)
 */

#ifndef TASK_SETUP_H
#define TASK_SETUP_H

#include <stdbool.h>
#include "app/app_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

bool TaskSetup_runWizard(app_flash_config_t *cfg);

#ifdef __cplusplus
}
#endif
#endif
