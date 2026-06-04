/*
 * main.c — 最小入口 (逐模块 debug)
 */
#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();
    for (;;) {}
}
