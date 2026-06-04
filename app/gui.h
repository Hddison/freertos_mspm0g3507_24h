/*
 * gui.h — DMA GUI 框架 (零 Paint 依赖)
 *
 * 屏幕: 170×320, ST7789, RGB565
 * 字体: Font8(5×8) Font12(7×12) Font16(11×16) Font20(14×20) Font24(17×24)
 */

#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include <stdbool.h>
#include "gui_paint.h"  /* FontXX, 颜色 */

/* ══════ DMA 原语 ══════ */

void gui_clear(void);  /* 全屏 DMA 黑 */

/* DMA 纯色矩形 (需先 setWindows) — 低级函数 */
void gui_fill_win(uint16_t color, uint16_t w, uint16_t h);

/* DMA 矩形: 指定坐标+宽高 */
void gui_rect(int x, int y, int w, int h, uint16_t c);

/* DMA 水平线 */
void gui_hline(int y, uint16_t c);

/* DMA 文字 (背景 BLACK) */
void gui_text(int x, int y, const char *s,
              const uint8_t *font, int fw, int fh, uint16_t fg);

/* DMA 格式化文字 */
void gui_printf(int x, int y, uint16_t fg,
                const uint8_t *font, int fw, int fh,
                const char *fmt, ...)
                __attribute__((format(printf, 7, 8)));

/* ══════ 按键方向 (物理重映射后) ══════ */
#define GUI_UP    0
#define GUI_DOWN  1
#define GUI_LEFT  2
#define GUI_RIGHT 3
#define GUI_ENTER 4
#define GUI_BACK  5

#define GUI_EVT_SHORT 2
#define GUI_EVT_LONG  3
#define GUI_EVT_HOLD  4

/* ══════ 屏幕管理 ══════ */

typedef enum {
    SCR_STATUS = 0,   /* 传感器数据实时页 */
    SCR_MENU,         /* 滚动列表 + 光标 */
    SCR_EDIT,         /* 数值编辑 */
    SCR_INFO,         /* 系统信息 */
} screen_t;

/* 当前活动屏 */
screen_t gui_screen(void);
void     gui_switch(screen_t s);

/* 每帧调用 (LCD 任务, ~50ms) */
void gui_tick(void);

/* 按键输入 (Button 任务通过队列发送) */
void gui_input(uint8_t btn, uint8_t evt);

/* 传感器数据注入 (Sensor 任务通过队列发送) */
void gui_feed_sensor(const void *data);

#endif
