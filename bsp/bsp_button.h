/*
 *  ============ bsp_button.h =============
 *  轻量按键驱动 — 轮询式, 短按 + 长按
 */

#ifndef BSP_BUTTON_H
#define BSP_BUTTON_H

#include <stdint.h>

/* 按键 ID */
#define BTN_ID_UP      0
#define BTN_ID_LEFT    1
#define BTN_ID_DOWN    2
#define BTN_ID_RIGHT   3
#define BTN_ID_CENTER  4
#define BTN_ID_BUTTON  5

/* 事件 */
#define BTN_EVT_NONE   0
#define BTN_EVT_SHORT  1
#define BTN_EVT_LONG   2

/* 每次调用扫描所有按键, 返回事件类型 (0/BTN_EVT_SHORT/BTN_EVT_LONG) */
uint8_t Button_Scan(void);

/* 获取上次事件对应的按键 ID (事件有效期内调用) */
uint8_t Button_ID(void);

#endif
