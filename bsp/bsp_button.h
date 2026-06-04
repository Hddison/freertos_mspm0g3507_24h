/*
 * ============ bsp_button.h =============
 * 轮询式按键驱动 (6 键, 低电平有效, 内部上拉)
 *
 * 调用者需以固定周期 (建议 20ms) 调用 BSP_Button_Scan()。
 * 状态机自动处理去抖 / 短按 / 长按 / 持续按住。
 *
 * 事件序列:
 *   PRESS_DOWN → (HOLD × N) → SHORT/LONG
 */

#ifndef BSP_BUTTON_H
#define BSP_BUTTON_H

#include <stdint.h>

/* ── 物理按键 ID ── */
#define BTN_ID_UP      0
#define BTN_ID_LEFT    1
#define BTN_ID_DOWN    2
#define BTN_ID_RIGHT   3
#define BTN_ID_CENTER  4
#define BTN_ID_BUTTON  5
#define BTN_COUNT      6

/* ── 事件类型 ── */
#define BTN_EVT_NONE        0   /* 无事件                           */
#define BTN_EVT_PRESS_DOWN  1   /* 按下确认 (去抖完成)              */
#define BTN_EVT_SHORT       2   /* 短按释放 (<500ms)                */
#define BTN_EVT_LONG        3   /* 长按释放 (≥500ms)                */
#define BTN_EVT_HOLD        4   /* 持续按住 (500ms 起, 每 200ms)    */
#define BTN_EVT_RELEASE     5   /* 释放 (预留, 当前不触发)          */

/* ── 逻辑方向 ID (用于按键重映射) ── */
#define BTN_DIR_UP      0
#define BTN_DIR_DOWN    1
#define BTN_DIR_LEFT    2
#define BTN_DIR_RIGHT   3
#define BTN_DIR_ENTER   4
#define BTN_DIR_BACK    5

/* 扫描所有按键, 返回事件类型 (BTN_EVT_*)。
 * 建议调用周期 20ms, 内部计时基于此周期累加。 */
uint8_t BSP_Button_Scan(void);

/* 返回最近一次事件的物理按键 ID */
uint8_t BSP_Button_ID(void);

#endif
