/*
 * gui.c — DMA GUI 框架实现
 */

#include "gui.h"
#include "hw_st7789.h"
#include "tasks/task_sensor.h"
#include <stdio.h>
#include <stdarg.h>

#define W  ST7789_WIDTH
#define H  ST7789_HEIGHT

/* ══════ DMA 原语 ══════ */

void gui_clear(void) {
    ST7789_setWindows(0, 0, W-1, H-1);
    ST7789_clearRawDMA(BLACK, W, H);
}

void gui_fill_win(uint16_t color, uint16_t w, uint16_t h) {
    ST7789_clearRawDMA(color, w, h);
}

void gui_rect(int x, int y, int w, int h, uint16_t c) {
    ST7789_setWindows(x, y, x+w-1, y+h-1);
    ST7789_clearRawDMA(c, w, h);
}

void gui_hline(int y, uint16_t c) {
    ST7789_setWindows(0, y, W-1, y);
    ST7789_clearRawDMA(c, W, 1);
}

void gui_text(int x, int y, const char *s,
              const uint8_t *font, int fw, int fh, uint16_t fg) {
    ST7789_drawStringFast(x, y, s, font, fw, fh, fg, BLACK);
}

void gui_printf(int x, int y, uint16_t fg,
                const uint8_t *font, int fw, int fh,
                const char *fmt, ...) {
    char buf[64];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    gui_text(x, y, buf, font, fw, fh, fg);
}

/* ══════ 屏幕管理 ══════ */

static screen_t g_scr = SCR_STATUS;
static bool     g_dirty = true;   /* 全屏重绘标记 */
static uint32_t g_idle_ticks;     /* 空闲计时 (无操作返回 STATUS) */
static uint8_t  g_cursor, g_scroll;
static float    g_edit_val, g_edit_min, g_edit_max, g_edit_step;
static uint8_t  g_edit_dec;
static char     g_edit_label[16];

/* 传感器数据缓存 (由 gui_feed_sensor 更新) */
static float    g_yaw, g_speed, g_enc1, g_enc2, g_line;
static uint16_t g_gs;
static bool     g_imu_ok, g_gs_ok;

/* ══════ 菜单数据 ══════ */

typedef struct { const char *label; uint8_t target; bool back; } item_t;

static const item_t menu_main[] = {
    {"Contest Mode",   1, false},  /* target = MENU_CONTEST id */
    {"PID Settings",   2, false},
    {"Calibration",    3, false},
    {"Button Remap",   4, false},
    {"Settings",       5, false},
    {"System Info",    99,false},  /* 99 = INFO screen */
    {"Back",           0, true },  /* 0 = STATUS */
};
#define MAIN_N (sizeof(menu_main)/sizeof(menu_main[0]))

static const item_t menu_contest[] = {
    {"Task1: A->B",      10, false},
    {"Task2: 1 Lap",     11, false},
    {"Task3: Fig-8",     12, false},
    {"Task4: Auto 4",    13, false},
    {"Back",              0, true },
};
#define CONT_N (sizeof(menu_contest)/sizeof(menu_contest[0]))

static const item_t menu_pid[] = {
    {"Speed Kp",      20, false},
    {"Speed Ki",      21, false},
    {"Speed Kd",      22, false},
    {"Steer Kp",      23, false},
    {"Steer Kd",      24, false},
    {"Heading Kp",    25, false},
    {"Save to Flash", 30, false},
    {"Back",           0, true },
};
#define PID_N (sizeof(menu_pid)/sizeof(menu_pid[0]))

static const item_t menu_calib[] = {
    {"IMU Yaw Zero",  31, false},
    {"IMU Roll Zero", 32, false},
    {"Gray Thresh",   33, false},
    {"Save to Flash", 30, false},
    {"Back",           0, true },
};
#define CAL_N (sizeof(menu_calib)/sizeof(menu_calib[0]))

static const item_t menu_remap[] = {
    {"Remap UP",      40, false},
    {"Remap DOWN",    41, false},
    {"Remap LEFT",    42, false},
    {"Remap RIGHT",   43, false},
    {"Remap ENTER",   44, false},
    {"Remap BACK",    45, false},
    {"Save to Flash", 30, false},
    {"Back",           0, true },
};
#define REM_N (sizeof(menu_remap)/sizeof(menu_remap[0]))

static const item_t menu_settings[] = {
    {"Buzzer: ON",    50, false},
    {"LED: ON",       51, false},
    {"Back",            0, true },
};
#define SET_N (sizeof(menu_settings)/sizeof(menu_settings[0]))

/* 当前菜单状态 */
static uint8_t       g_menu_id;
static const item_t *g_items;
static uint8_t       g_item_n;

static const char *menu_title(uint8_t id) {
    switch (id) {
    case 1: return "Contest Mode";
    case 2: return "PID Settings";
    case 3: return "Calibration";
    case 4: return "Button Remap";
    case 5: return "Settings";
    default: return "Menu";
    }
}

static void menu_set(uint8_t id) {
    g_menu_id = id;
    switch (id) {
    case 0: g_items = menu_main;     g_item_n = MAIN_N; break;
    case 1: g_items = menu_contest;  g_item_n = CONT_N; break;
    case 2: g_items = menu_pid;      g_item_n = PID_N;  break;
    case 3: g_items = menu_calib;    g_item_n = CAL_N;  break;
    case 4: g_items = menu_remap;    g_item_n = REM_N;  break;
    case 5: g_items = menu_settings; g_item_n = SET_N;  break;
    default:g_items = NULL;          g_item_n = 0;      break;
    }
    g_cursor = 0; g_scroll = 0;
}

#define MAX_VIS 10  /* 320/(22+6) ≈ 11, 留余量 */

/* ══════ 渲染 ══════ */

static void render_status(void) {
    gui_clear();
    gui_text(2, 2, "MSPM0G3507", Font16.table, 11, 16, CYAN);
    gui_hline(24, LGRAY);

    /* Yaw 大字 */
    gui_printf(2, 30, YELLOW, Font20.table, 14, 20, "%.1f", (double)g_yaw);
    gui_text(2, 52, "deg", Font12.table, 7, 12, LGRAY);

    /* 数据行 */
    gui_printf(2, 72, WHITE, Font12.table, 7, 12,
               "Spd:%.0f mm/s", (double)g_speed);
    gui_printf(2, 90, CYAN, Font12.table, 7, 12,
               "Enc1:%.0f Enc2:%.0f", (double)g_enc1, (double)g_enc2);
    gui_printf(2, 108, LGRAY, Font12.table, 7, 12,
               "Line:%+.1f", (double)g_line);

    /* 灰度条 */
    for (int i = 0; i < 12; i++) {
        uint16_t sx = 2 + i*14;
        uint16_t c = (g_gs & (1 << (11 - i))) ? WHITE : BLACK;
        gui_rect(sx, 126, 12, 14, c);
    }

    /* 底部状态 */
    gui_hline(148, LGRAY);
    gui_printf(2, 154, g_imu_ok ? GREEN : RED, Font12.table, 7, 12,
               "IMU:%s  GS:%s", g_imu_ok?"OK":"ERR", g_gs_ok?"OK":"ERR");
}

static void render_menu(void) {
    gui_clear();
    gui_text(2, 2, menu_title(g_menu_id), Font16.table, 11, 16, CYAN);
    gui_hline(24, LGRAY);

    int y = 30;
    for (int i = g_scroll; i < g_item_n && (i - g_scroll) < MAX_VIS; i++) {
        if (i == g_cursor) {
            gui_rect(0, y, W, 20, YELLOW);
            gui_text(8, y+3, g_items[i].label, Font12.table, 7, 12, BLACK);
        } else {
            gui_text(4, y+3, g_items[i].label, Font12.table, 7, 12, WHITE);
        }
        y += 22;
    }
    gui_hline(300, LGRAY);
    gui_text(2, 308, "UP/DN:Nav ENTER:Sel BACK:Ret", Font8.table, 5, 8, LGRAY);
}

static void render_edit(void) {
    gui_clear();
    gui_text(2, 2, g_edit_label, Font16.table, 11, 16, CYAN);
    gui_hline(24, LGRAY);

    gui_printf(20, 100, YELLOW, Font20.table, 14, 20,
               "%.*f", g_edit_dec, (double)g_edit_val);
    gui_printf(20, 130, LGRAY, Font12.table, 7, 12,
               "[%.*f ~ %.*f]", g_edit_dec, (double)g_edit_min,
               g_edit_dec, (double)g_edit_max);

    gui_hline(290, LGRAY);
    gui_text(2, 298, "UP/DN:chg ENTER/BACK:ok", Font8.table, 5, 8, LGRAY);
}

static void render_info(void) {
    gui_clear();
    gui_text(2, 2, "System Info", Font16.table, 11, 16, CYAN);
    gui_hline(24, LGRAY);
    gui_printf(2, 35, WHITE, Font12.table, 7, 12, "MCU: MSPM0G3507");
    gui_printf(2, 53, WHITE, Font12.table, 7, 12, "CPU: 80 MHz");
    gui_printf(2, 71, WHITE, Font12.table, 7, 12, "SPI: 10 MHz");
    gui_printf(2, 89, WHITE, Font12.table, 7, 12, "LCD: ST7789 170x320");
    gui_text(2, 300, "ENTER/BACK: Return", Font8.table, 5, 8, LGRAY);
}

/* ══════ 渲染调度 ══════ */

static void render(void) {
    switch (g_scr) {
    case SCR_STATUS: render_status(); break;
    case SCR_MENU:   render_menu();   break;
    case SCR_EDIT:   render_edit();   break;
    case SCR_INFO:   render_info();   break;
    }
    g_dirty = false;
}

/* ══════ 输入处理 ══════ */

static void input_menu(uint8_t btn, uint8_t evt) {
    if (evt != GUI_EVT_SHORT) return;
    switch (btn) {
    case GUI_UP:
        if (g_cursor > 0) { g_cursor--; if (g_cursor < g_scroll) g_scroll = g_cursor; }
        break;
    case GUI_DOWN:
        if (g_cursor+1 < g_item_n) { g_cursor++; if (g_cursor >= g_scroll+MAX_VIS) g_scroll = g_cursor-MAX_VIS+1; }
        break;
    case GUI_ENTER: {
        if (!g_items || g_cursor >= g_item_n) break;
        const item_t *it = &g_items[g_cursor];
        if (it->back) {
            /* 返回 */
            if (g_menu_id == 0) { gui_switch(SCR_STATUS); return; }
            menu_set(0); /* back to main menu */
        } else if (it->target < 10) {
            /* 子菜单 */
            menu_set(it->target);
        } else if (it->target >= 20 && it->target <= 25) {
            /* PID 编辑 */
            snprintf(g_edit_label, sizeof(g_edit_label), "%s", it->label);
            g_edit_val = 0.0f; g_edit_min = 0.0f; g_edit_max = 100.0f;
            g_edit_step = 0.1f; g_edit_dec = 1;
            gui_switch(SCR_EDIT);
        } else if (it->target == 33) {
            /* 灰度阈值编辑 */
            snprintf(g_edit_label, sizeof(g_edit_label), "Gray Threshold");
            g_edit_val = 2000; g_edit_min = 0; g_edit_max = 4095;
            g_edit_step = 1; g_edit_dec = 0;
            gui_switch(SCR_EDIT);
        } else if (it->target == 50) {
            /* Buzzer toggle */
            gui_switch(SCR_STATUS);
        } else if (it->target == 99) {
            gui_switch(SCR_INFO);
        } else {
            /* 竞赛任务 (10-13) 或 动作 (30-45), 暂不实现 */
            gui_switch(SCR_STATUS);
        }
        return;
    }
    case GUI_BACK:
        if (g_menu_id == 0) { gui_switch(SCR_STATUS); return; }
        menu_set(0);
        break;
    }
    g_dirty = true;
}

static void input_edit(uint8_t btn, uint8_t evt) {
    if (evt == GUI_EVT_SHORT || evt == GUI_EVT_HOLD) {
        if (btn == GUI_UP)   { g_edit_val += g_edit_step; if (g_edit_val > g_edit_max) g_edit_val = g_edit_max; }
        if (btn == GUI_DOWN) { g_edit_val -= g_edit_step; if (g_edit_val < g_edit_min) g_edit_val = g_edit_min; }
        g_dirty = true;
    }
    if (evt == GUI_EVT_SHORT && (btn == GUI_ENTER || btn == GUI_BACK)) {
        /* 保存并返回 */
        gui_switch(SCR_MENU); return;
    }
}

/* ══════ API ══════ */

screen_t gui_screen(void) { return g_scr; }

void gui_switch(screen_t s) {
    g_scr = s; g_dirty = true; g_idle_ticks = 0;
    if (s == SCR_MENU && g_menu_id == 0) menu_set(0);
}

void gui_input(uint8_t btn, uint8_t evt) {
    g_idle_ticks = 0;
    switch (g_scr) {
    case SCR_STATUS:
        if (btn == GUI_ENTER && evt == GUI_EVT_SHORT) { menu_set(0); gui_switch(SCR_MENU); }
        break;
    case SCR_MENU:   input_menu(btn, evt); break;
    case SCR_EDIT:   input_edit(btn, evt); break;
    case SCR_INFO:
        if (evt == GUI_EVT_SHORT) { gui_switch(SCR_MENU); }
        break;
    }
}

void gui_feed_sensor(const void *d) {
    const sensor_data_t *sd = (const sensor_data_t *)d;
    g_imu_ok = sd->jy61p_ok; g_gs_ok = sd->nchd12_ok;
    g_yaw = sd->total_yaw; g_speed = sd->enc_speed;
    g_enc1 = sd->enc1_dist; g_enc2 = sd->enc2_dist;
    g_gs   = sd->grayscale; g_line = sd->line_position;
}

void gui_tick(void) {
    /* 空闲 5s 返回 STATUS */
    if (g_scr != SCR_STATUS) {
        g_idle_ticks++;
        if (g_idle_ticks > 100) { /* 100 * 50ms = 5s */
            gui_switch(SCR_STATUS);
            return;
        }
    }
    /* 只在脏标记或 STATUS 屏时渲染 (一次) */
    if (g_dirty || g_scr == SCR_STATUS) render();
}
