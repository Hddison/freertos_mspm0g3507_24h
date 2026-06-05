/*
 * ============ app_menu.c =============
 * LCD 多级菜单状态机 + 渲染实现
 */

#include "app_menu.h"
#include "app_config.h"
#include "app_pid.h"
#include "app_flash.h"

#include "bsp_button.h"   /* BTN_EVT_*, BTN_DIR_* */
#include "gui_paint.h"    /* BLACK, GREEN, CYAN, WHITE, YELLOW, MAGENTA */
#include "hw_st7789.h"
#include "util.h"
#include <string.h>
#include <stdio.h>   /* snprintf (TI toolchain supports it) */

/* ══════════ 外部引用 ══════════ */

extern const uint8_t Font8_Table[];
extern const uint8_t Font12_Table[];
#define FW8   6    /* 字符宽 5 + 1px 间距 */
#define FH8   10   /* 行高 8 + 2px 间距   */
#define FW12  8
#define FH12  16

#define MENU_FW  FW8
#define MENU_FH  FH8

/* PID 全局实例 (app_control.c 中定义) */
extern pid_t g_pid_speed;
extern pid_t g_pid_pos;
extern pid_t g_pid_heading;
extern pid_t g_pid_steer;

/* 全局目标速度 (app_control.c) */
extern float g_target_speed;

/* Flash 配置 */
extern flash_config_t g_flash_cfg;

/* 蜂鸣器使能 */
extern bool g_buzzer_enabled;
/* LED 心跳使能 */
extern bool g_led_heartbeat_enabled;

/* ══════════ 前向声明 ══════════ */

static void push_menu(menu_state_t *m);
static void pop_menu(menu_state_t *m);

/* ── 渲染函数 ── */
static void render_status(const menu_state_t *m, const sensor_data_t *s);
static void render_menu(const menu_state_t *m);
static void render_value_edit(const menu_state_t *m);
static void render_confirm(const menu_state_t *m);
static void render_contest(const menu_state_t *m);
static void render_info(const menu_state_t *m,
                        float kf_x, float kf_y, float kf_theta);

/* ══════════ 菜单树定义 (static const) ══════════ */

/* 子菜单 — 竞赛模式 */
static const menu_item_t menu_contest[] = {
    {"Task1: A->B",           MENU_TYPE_ACTION, NULL, 0},
    {"Task2: 1 Lap CW",       MENU_TYPE_ACTION, NULL, 1},
    {"Task3: Diag Cross",     MENU_TYPE_ACTION, NULL, 2},
    {"Task4: Auto 4 Laps",    MENU_TYPE_ACTION, NULL, 3},
    {"Return",                MENU_TYPE_RETURN, NULL, 4},
};
#define MENU_CONTEST_COUNT  (sizeof(menu_contest) / sizeof(menu_item_t))

/* 子菜单 — PID 参数 */
static const menu_item_t menu_pid[] = {
    {"Speed KP",       MENU_TYPE_VALUE,  NULL, 0},
    {"Speed KI",       MENU_TYPE_VALUE,  NULL, 1},
    {"Speed KD",       MENU_TYPE_VALUE,  NULL, 2},
    {"Position KP",    MENU_TYPE_VALUE,  NULL, 3},
    {"Steer KP",       MENU_TYPE_VALUE,  NULL, 4},
    {"Steer KD",       MENU_TYPE_VALUE,  NULL, 5},
    {"Heading KP",     MENU_TYPE_VALUE,  NULL, 6},
    {"Target Speed",   MENU_TYPE_VALUE,  NULL, 7},
    {"Save to Flash",  MENU_TYPE_ACTION, NULL, 8},
    {"Return",         MENU_TYPE_RETURN, NULL, 9},
};
#define MENU_PID_COUNT  (sizeof(menu_pid) / sizeof(menu_item_t))

/* 子菜单 — 校准 */
static const menu_item_t menu_cal[] = {
    {"IMU Yaw Zero",       MENU_TYPE_ACTION, NULL, 0},
    {"IMU Roll Zero",      MENU_TYPE_ACTION, NULL, 1},
    {"Gray Threshold",     MENU_TYPE_VALUE,  NULL, 2},
    {"Save to Flash",      MENU_TYPE_ACTION, NULL, 3},
    {"Return",             MENU_TYPE_RETURN, NULL, 4},
};
#define MENU_CAL_COUNT  (sizeof(menu_cal) / sizeof(menu_item_t))

/* 子菜单 — 系统设置 */
static const menu_item_t menu_settings[] = {
    {"Buzzer: ",           MENU_TYPE_VALUE,  NULL, 0},  /* 切换 bool */
    {"LED Heartbeat: ",    MENU_TYPE_VALUE,  NULL, 1},
    {"Return",             MENU_TYPE_RETURN, NULL, 2},
};
#define MENU_SETTINGS_COUNT  (sizeof(menu_settings) / sizeof(menu_item_t))

/* 主菜单 */
static const menu_item_t menu_main[] = {
    {"1. Contest Mode",    MENU_TYPE_SUBMENU, menu_contest,   0},
    {"2. PID Parameters",  MENU_TYPE_SUBMENU, menu_pid,       1},
    {"3. Calibration",     MENU_TYPE_SUBMENU, menu_cal,       2},
    {"4. System Settings", MENU_TYPE_SUBMENU, menu_settings,  3},
    {"5. System Info",     MENU_TYPE_SCREEN,  (void*)SCREEN_INFO, 4},
    {"6. Return",          MENU_TYPE_RETURN,  NULL,            5},
};
#define MENU_MAIN_COUNT  (sizeof(menu_main) / sizeof(menu_item_t))

/* ══════════ 辅助: 获取 PID 指针 ══════════ */

float* menu_get_pid_ptr(int pid_id)
{
    switch (pid_id) {
    case 0: return &g_pid_speed.kp;
    case 1: return &g_pid_speed.ki;
    case 2: return &g_pid_speed.kd;
    case 3: return &g_pid_pos.kp;
    case 4: return &g_pid_steer.kp;
    case 5: return &g_pid_steer.kd;
    case 6: return &g_pid_heading.kp;
    case 7: return &g_target_speed;
    default: return NULL;
    }
}

/* ══════════ 菜单动作回调 ══════════ */

static ctrl_cmd_t g_pending_cmd = CMD_NONE;

ctrl_cmd_t menu_get_pending_cmd(const menu_state_t *m)
{
    (void)m;
    ctrl_cmd_t cmd = g_pending_cmd;
    return cmd;
}

void menu_clear_pending_cmd(menu_state_t *m)
{
    (void)m;
    g_pending_cmd = CMD_NONE;
}

void menu_action_save(void)
{
    /* 将当前 PID 值写入 flash_config 并保存 */
    g_flash_cfg.speed_kp     = g_pid_speed.kp;
    g_flash_cfg.speed_ki     = g_pid_speed.ki;
    g_flash_cfg.speed_kd     = g_pid_speed.kd;
    g_flash_cfg.steer_kp     = g_pid_steer.kp;
    g_flash_cfg.steer_kd     = g_pid_steer.kd;
    g_flash_cfg.heading_kp   = g_pid_heading.kp;
    g_flash_cfg.target_speed = g_target_speed;
    flash_config_save(&g_flash_cfg);
}

void menu_action_imu_zero_yaw(void)
{
    /* 设置偏航零偏 — 由 Sensor 任务处理 */
    /* 这里写入 flash_config 标志位 */
}

void menu_action_imu_zero_roll(void)
{
    /* 设置横滚零偏 */
}

/* ══════════ 初始化 ══════════ */

void menu_init(menu_state_t *m)
{
    memset(m, 0, sizeof(menu_state_t));
    m->screen           = SCREEN_STATUS;
    m->needs_full_redraw = true;
    m->last_input_tick   = 0;
}

/* ══════════ 菜单栈操作 ══════════ */

static void push_menu(menu_state_t *m)
{
    if (m->parent_depth < 4) {
        m->parent_menu[m->parent_depth]   = m->menu;
        m->parent_count[m->parent_depth]  = m->menu_count;
        m->parent_cursor[m->parent_depth] = m->cursor;
        m->parent_depth++;
    }
}

static void pop_menu(menu_state_t *m)
{
    if (m->parent_depth > 0) {
        m->parent_depth--;
        m->menu       = m->parent_menu[m->parent_depth];
        m->menu_count = m->parent_count[m->parent_depth];
        m->cursor     = m->parent_cursor[m->parent_depth];
        m->screen     = SCREEN_MENU;
    } else {
        /* 已到顶级, 返回 STATUS */
        m->screen = SCREEN_STATUS;
    }
    m->scroll_offset  = 0;
    m->needs_full_redraw = true;
}

/* ══════════ 按键事件处理 ══════════ */

bool menu_process_event(menu_state_t *m, const button_event_t *evt)
{
    /* 任何按键复位 auto-return 计时 */
    m->last_input_tick = evt->tick;

    uint8_t dir = evt->logical;
    uint8_t typ = evt->event_type;

    /* ── STATUS 屏: CENTER → 进入主菜单 ── */
    if (m->screen == SCREEN_STATUS) {
        if (typ == BTN_EVT_SHORT && dir == BTN_DIR_ENTER) {
            m->screen     = SCREEN_MENU;
            m->menu       = menu_main;
            m->menu_count = MENU_MAIN_COUNT;
            m->cursor     = 0;
            m->scroll_offset = 0;
            m->needs_full_redraw = true;
            return true;
        }
        return false;
    }

    /* ── 确认对话框: LEFT=Y RIGHT=N ── */
    if (m->screen == SCREEN_CONFIRM) {
        if (typ == BTN_EVT_SHORT) {
            if (dir == BTN_DIR_LEFT) {
                m->confirm_yes = true;
                /* 触发对应动作 */
                if (m->confirm_msg && strstr(m->confirm_msg, "Save")) {
                    menu_action_save();
                }
                pop_menu(m);
                return true;
            } else if (dir == BTN_DIR_RIGHT || dir == BTN_DIR_BACK) {
                m->confirm_yes = false;
                pop_menu(m);
                return true;
            }
        }
        return false;
    }

    /* ── 数值编辑 ── */
    if (m->screen == SCREEN_VALUE_EDIT) {
        if (!m->edit_value) return false;

        float step = (typ == BTN_EVT_HOLD) ? m->edit_step :
                     (dir == BTN_DIR_LEFT || dir == BTN_DIR_RIGHT)
                        ? m->edit_coarse : m->edit_step;

        if (dir == BTN_DIR_UP) {
            *m->edit_value += step;
            return true;
        } else if (dir == BTN_DIR_DOWN) {
            *m->edit_value -= step;
            return true;
        } else if (dir == BTN_DIR_BACK || dir == BTN_DIR_ENTER) {
            /* 退出编辑, 返回上级菜单 */
            pop_menu(m);
            return true;
        }
        return false;
    }

    /* ── 竞赛屏: BACK → 返回菜单 ── */
    if (m->screen == SCREEN_CONTEST) {
        if (typ == BTN_EVT_SHORT && dir == BTN_DIR_BACK) {
            pop_menu(m);
            return true;
        }
        return false;
    }

    /* ── INFO 屏: 任意键返回 ── */
    if (m->screen == SCREEN_INFO) {
        if (typ == BTN_EVT_SHORT) {
            pop_menu(m);
            return true;
        }
        return false;
    }

    /* ── 菜单屏 ── */
    if (m->screen == SCREEN_MENU && m->menu && m->menu_count > 0) {
        switch (dir) {
        case BTN_DIR_UP:
            if (typ == BTN_EVT_SHORT || typ == BTN_EVT_HOLD) {
                if (m->cursor > 0) {
                    m->cursor--;
                    if (m->cursor < m->scroll_offset)
                        m->scroll_offset = m->cursor;
                    return true;
                } else {
                    m->cursor = m->menu_count - 1;
                    m->scroll_offset = (m->menu_count > MENU_VISIBLE_ROWS)
                        ? m->menu_count - MENU_VISIBLE_ROWS : 0;
                    return true;
                }
            }
            break;

        case BTN_DIR_DOWN:
            if (typ == BTN_EVT_SHORT || typ == BTN_EVT_HOLD) {
                if (m->cursor < m->menu_count - 1) {
                    m->cursor++;
                    if (m->cursor >= m->scroll_offset + MENU_VISIBLE_ROWS)
                        m->scroll_offset = m->cursor - MENU_VISIBLE_ROWS + 1;
                    return true;
                } else {
                    m->cursor = 0;
                    m->scroll_offset = 0;
                    return true;
                }
            }
            break;

        case BTN_DIR_ENTER:
        case BTN_DIR_RIGHT:
            if (typ == BTN_EVT_SHORT) {
                const menu_item_t *it = &m->menu[m->cursor];
                switch (it->type) {
                case MENU_TYPE_SUBMENU:
                    push_menu(m);
                    m->menu       = (const menu_item_t*)it->target;
                    m->menu_count = 0;
                    /* 计算子菜单条目数 (哨兵法: 遍历直到 title==NULL) */
                    {
                        const menu_item_t *p = (const menu_item_t*)it->target;
                        while (p->title) { m->menu_count++; p++; }
                    }
                    m->cursor   = 0;
                    m->scroll_offset = 0;
                    m->needs_full_redraw = true;
                    return true;

                case MENU_TYPE_VALUE: {
                    m->screen     = SCREEN_VALUE_EDIT;
                    m->edit_label = it->title;
                    /* 获取编辑目标 */
                    if (it->id < 8) {
                        m->edit_value = menu_get_pid_ptr(it->id);
                    } else if (it->id == 100) {
                        /* ignore for now */
                    }
                    m->edit_step   = (it->id == 7) ? 10.0f : 0.1f;
                    m->edit_coarse = m->edit_step * 10.0f;
                    m->needs_full_redraw = true;
                    return true;
                }

                case MENU_TYPE_ACTION:
                    if (it->id == 8) {
                        /* Save to Flash — 确认 */
                        m->screen = SCREEN_CONFIRM;
                        m->confirm_msg = "Save to Flash?";
                        m->needs_full_redraw = true;
                    } else if (it->id <= 3) {
                        /* Task 1-4 */
                        g_pending_cmd = (ctrl_cmd_t)(CMD_START_TASK1 + it->id);
                        m->screen = SCREEN_CONTEST;
                        m->task_id = it->id + 1;
                        m->lap = 0;
                        m->elapsed_ms = 0;
                        m->needs_full_redraw = true;
                    }
                    return true;

                case MENU_TYPE_SCREEN:
                    m->screen = (screen_type_t)(uintptr_t)it->target;
                    push_menu(m);  /* 保存当前位置, 以便返回 */
                    m->menu = NULL;
                    m->menu_count = 0;
                    m->needs_full_redraw = true;
                    return true;

                case MENU_TYPE_RETURN:
                    pop_menu(m);
                    return true;
                }
            }
            break;

        case BTN_DIR_BACK:
        case BTN_DIR_LEFT:
            if (typ == BTN_EVT_SHORT) {
                pop_menu(m);
                return true;
            }
            break;
        }
    }

    return false;
}

/* ══════════ 渲染入口 ══════════ */

void menu_render(const menu_state_t *m, const sensor_data_t *sensor,
                 float kf_x, float kf_y, float kf_theta)
{
    /* 屏幕切换时全屏清除, 避免旧内容残留 */
    static screen_type_t prev_screen = SCREEN_STATUS;
    if (m->screen != prev_screen) {
        prev_screen = m->screen;
        ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
        ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);
    }

    switch (m->screen) {
    case SCREEN_STATUS:     render_status(m, sensor);    break;
    case SCREEN_MENU:       render_menu(m);              break;
    case SCREEN_VALUE_EDIT: render_value_edit(m);        break;
    case SCREEN_CONFIRM:    render_confirm(m);           break;
    case SCREEN_CONTEST:    render_contest(m);           break;
    case SCREEN_INFO:       render_info(m, kf_x, kf_y, kf_theta); break;
    }
}

/* ══════════ SCREEN_STATUS: 传感器仪表盘 ══════════ */

static char g_status_cache[8][28];   /* 8 行 × 28 字符缓存 */

static void draw_row(int y, const char *label, const char *val, uint16_t color)
{
    char buf[28];
    int p = 0;
    while (*label) buf[p++] = *label++;
    buf[p++] = ' '; buf[p] = 0;
    ST7789_setWindows(0, y, ST7789_WIDTH - 1, y + FH8 - 1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, FH8);
    ST7789_drawStringFast(0, y, buf, Font8_Table, FW8, 8, color, BLACK);
    ST7789_drawStringFast(55, y, val, Font8_Table, FW8, 8, color, BLACK);
}

static void render_status(const menu_state_t *m, const sensor_data_t *s)
{
    (void)m;
    char buf[28];
    int y = FH8 * 2;

    /* 无传感器数据时显示提示 (不清屏, 保留启动信息) */
    if (!s) {
        ST7789_drawStringFast(0, y, "No sensor data", Font8_Table, FW8, 8, YELLOW, BLACK);
        ST7789_drawStringFast(0, y + FH8 * 2, "Check IMU/Gray", Font8_Table, FW8, 8, WHITE, BLACK);
        return;
    }

    /* 首次获得数据: 全屏清除旧 splash/启动提示 */
    {
        static bool first_clear_done = false;
        if (!first_clear_done) {
            first_clear_done = true;
            ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
            ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);
        }
    }

    /* Row 0: Heading */
    {
        int p = 0;
        p += util_ftoa(s->yaw, 1, buf + p);
        buf[p++] = (char)0xB0;  /* ° symbol — may not render, use 'd' */
        buf[p] = 0;
        const char *new_val = buf;
        if (strcmp(new_val, g_status_cache[0])) {
            strcpy(g_status_cache[0], new_val);
            draw_row(y, "YAW:", buf, GREEN);
        }
        y += FH8;
    }

    /* Row 1: Speed + Dist */
    {
        int p = 0;
        p += util_ftoa(s->enc_speed, 1, buf + p);
        buf[p++] = 'm'; buf[p++] = 'm'; buf[p++] = '/'; buf[p++] = 's';
        buf[p] = 0;
        if (strcmp(buf, g_status_cache[1])) {
            strcpy(g_status_cache[1], buf);
            draw_row(y, "SPD:", buf, CYAN);
        }
        y += FH8;
    }

    /* Row 2: Encoder dist */
    {
        int p = 0;
        p += util_ftoa(s->enc1_dist, 1, buf + p);
        buf[p++] = ' '; buf[p++] = ' ';
        p += util_ftoa(s->enc2_dist, 1, buf + p);
        buf[p] = 0;
        if (strcmp(buf, g_status_cache[2])) {
            strcpy(g_status_cache[2], buf);
            draw_row(y, "ENC:", buf, YELLOW);
        }
        y += FH8;
    }

    /* Row 3: Grayscale */
    {
        util_bits12(s->grayscale, buf);
        int cnt = 0;
        for (int i = 0; i < 12; i++) if (buf[i] == '1') cnt++;
        buf[12] = ' '; buf[13] = 'C'; buf[14] = '=';
        int p = 15;
        p += util_itoa(cnt, buf + p);
        buf[p] = 0;
        if (strcmp(buf, g_status_cache[3])) {
            strcpy(g_status_cache[3], buf);
            draw_row(y, "GRY:", buf, WHITE);
        }
        y += FH8 * 2;
    }

    /* Row 4: Kalman state */
    {
        int p = 0;
        p += util_ftoa(s->total_yaw, 1, buf + p);
        buf[p++] = 'd';
        buf[p] = 0;
        if (strcmp(buf, g_status_cache[4])) {
            strcpy(g_status_cache[4], buf);
            draw_row(y, "T-YAW:", buf, GREEN);
        }
        y += FH8;
    }

    /* Row 5: IMU gyro */
    {
        int p = 0;
        buf[p++] = 'G'; buf[p++] = 'z'; buf[p++] = ':';
        p += util_ftoa(s->gz, 1, buf + p);
        buf[p] = 0;
        if (strcmp(buf, g_status_cache[5])) {
            strcpy(g_status_cache[5], buf);
            draw_row(y, "GYRO:", buf, CYAN);
        }
        y += FH8;
    }

    /* Row 6: hint */
    {
        const char *hint = "[CENTER] Menu";
        if (strcmp(hint, g_status_cache[6])) {
            strcpy(g_status_cache[6], hint);
            draw_row(y, "", hint, MAGENTA);
        }
    }
}

/* ══════════ SCREEN_MENU: 可滚动列表 ══════════ */

static void render_menu_row(int y, const char *title, bool highlighted)
{
    char buf[28];
    int p = 0;
    if (highlighted) buf[p++] = '>';
    else             buf[p++] = ' ';
    buf[p++] = ' ';
    while (*title && p < 26) buf[p++] = *title++;
    buf[p] = 0;

    uint16_t fg = highlighted ? BLACK : WHITE;
    uint16_t bg = highlighted ? CYAN  : BLACK;

    ST7789_setWindows(0, y, ST7789_WIDTH - 1, y + FH8 - 1);
    ST7789_clearRawDMA(bg, ST7789_WIDTH, FH8);
    ST7789_drawStringFast(0, y, buf, Font8_Table, FW8, 8, fg, bg);
}

static void render_menu(const menu_state_t *m)
{
    int y = 0;

    /* 标题栏 */
    {
        ST7789_setWindows(0, 0, ST7789_WIDTH - 1, FH8 - 1);
        ST7789_clearRawDMA(BLACK, ST7789_WIDTH, FH8);
        ST7789_drawStringFast(0, 0, "== MENU ==", Font8_Table, FW8, 8, GREEN, BLACK);
    }
    y = FH8 + 2;

    int visible = (m->menu_count < MENU_VISIBLE_ROWS) ? m->menu_count
                                                      : MENU_VISIBLE_ROWS;

    for (int i = 0; i < visible; i++) {
        int idx = m->scroll_offset + i;
        if (idx >= (int)m->menu_count) break;
        render_menu_row(y, m->menu[idx].title, (idx == m->cursor));
        y += MENU_ROW_H;
    }

    /* 清空剩余区域 */
    if (y < ST7789_HEIGHT) {
        ST7789_setWindows(0, y, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
        ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT - y);
    }
}

/* ══════════ SCREEN_VALUE_EDIT ══════════ */

static void render_value_edit(const menu_state_t *m)
{
    char buf[32];
    int p = 0;
    const char *label = m->edit_label ? m->edit_label : "?";
    while (*label && p < 15) buf[p++] = *label++;
    buf[p++] = ':'; buf[p++] = ' ';
    p += util_ftoa(m->edit_value ? *m->edit_value : 0.0f, 2, buf + p);
    buf[p] = 0;

    ST7789_setWindows(0, FH8 * 4, ST7789_WIDTH - 1, FH8 * 6);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, FH8 * 2);
    ST7789_drawStringFast(10, FH8 * 4, buf, Font12_Table, FW12, 12, CYAN, BLACK);

    const char *hint = "UP/DN:fine  L/R:coarse  ENT:ok";
    ST7789_drawStringFast(0, FH8 * 8, hint, Font8_Table, FW8, 8, WHITE, BLACK);
}

/* ══════════ SCREEN_CONFIRM ══════════ */

static void render_confirm(const menu_state_t *m)
{
    ST7789_setWindows(0, FH8 * 4, ST7789_WIDTH - 1, FH8 * 8);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, FH8 * 4);

    ST7789_drawStringFast(10, FH8 * 4, m->confirm_msg ? m->confirm_msg : "Confirm?",
                          Font12_Table, FW12, 12, YELLOW, BLACK);
    ST7789_drawStringFast(10, FH8 * 6, "[LEFT]=Yes  [RIGHT]=No",
                          Font8_Table, FW8, 8, WHITE, BLACK);
}

/* ══════════ SCREEN_CONTEST ══════════ */

static void render_contest(const menu_state_t *m)
{
    char buf[28];
    ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);

    int y = FH8;

    /* 任务名 */
    {
        const char *tasks[] = {"","Task1: A->B","Task2: 1 Lap","Task3: Diag","Task4: 4Lap"};
        ST7789_drawStringFast(0, y, tasks[m->task_id & 3],
                              Font8_Table, FW8, 8, GREEN, BLACK);
        y += FH8 * 2;
    }

    /* 圈数 */
    {
        int p = 0;
        p += snprintf(buf, sizeof(buf), "Lap: %u", m->lap);
        (void)p;
        ST7789_drawStringFast(0, y, buf, Font8_Table, FW8, 8, WHITE, BLACK);
        y += FH8 * 2;
    }

    /* 时间 */
    {
        uint32_t sec = m->elapsed_ms / 1000;
        uint32_t ms  = m->elapsed_ms % 1000;
        int p = snprintf(buf, sizeof(buf), "Time: %u.%03u s", (unsigned)sec, (unsigned)ms);
        (void)p;
        ST7789_drawStringFast(0, y, buf, Font8_Table, FW8, 8, YELLOW, BLACK);
        y += FH8 * 2;
    }

    /* 速度 */
    {
        int p = 0;
        p += util_ftoa(m->cur_speed, 1, buf + p);
        buf[p++] = 'm'; buf[p++] = 'm'; buf[p++] = '/'; buf[p++] = 's';
        buf[p] = 0;
        ST7789_drawStringFast(0, y, "Spd:", Font8_Table, FW8, 8, CYAN, BLACK);
        ST7789_drawStringFast(60, y, buf, Font8_Table, FW8, 8, CYAN, BLACK);
        y += FH8 * 2;
    }

    /* 航向 */
    {
        int p = 0;
        p += util_ftoa(m->cur_heading, 1, buf + p);
        buf[p] = 0;
        ST7789_drawStringFast(0, y, "Head:", Font8_Table, FW8, 8, WHITE, BLACK);
        ST7789_drawStringFast(60, y, buf, Font8_Table, FW8, 8, WHITE, BLACK);
    }

    /* 提示 */
    ST7789_drawStringFast(0, ST7789_HEIGHT - FH8 - 2, "[BACK] Stop",
                          Font8_Table, FW8, 8, MAGENTA, BLACK);
}

/* ══════════ SCREEN_INFO ══════════ */

#include <FreeRTOS.h>
#include <task.h>

static void render_info(const menu_state_t *m, float kf_x, float kf_y,
                        float kf_theta)
{
    (void)m;
    char buf[32];

    ST7789_setWindows(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    ST7789_clearRawDMA(BLACK, ST7789_WIDTH, ST7789_HEIGHT);

    int y = FH8;
    ST7789_drawStringFast(0, y, "=== System Info ===", Font8_Table, FW8, 8, GREEN, BLACK);
    y += FH8 * 2;

    /* 版本 */
    ST7789_drawStringFast(0, y, "FW: v2.0-car-svc", Font8_Table, FW8, 8, WHITE, BLACK);
    y += FH8;

    /* Flash ID */
    ST7789_drawStringFast(0, y, "Flash: W25Q128", Font8_Table, FW8, 8, WHITE, BLACK);
    y += FH8;

    /* Free heap */
    {
        size_t heap = xPortGetFreeHeapSize();
        int p = snprintf(buf, sizeof(buf), "Heap free: %u B", (unsigned)heap);
        (void)p;
        ST7789_drawStringFast(0, y, buf, Font8_Table, FW8, 8, WHITE, BLACK);
        y += FH8;
    }

    /* Kalman 状态 */
    {
        ST7789_drawStringFast(0, y, "Kalman:", Font8_Table, FW8, 8, CYAN, BLACK);
        y += FH8;
        snprintf(buf, sizeof(buf), "  x=%.0f y=%.0f", kf_x, kf_y);
        ST7789_drawStringFast(0, y, buf, Font8_Table, FW8, 8, WHITE, BLACK);
        y += FH8;
        snprintf(buf, sizeof(buf), "  th=%.1f deg", kf_theta * 57.29578f);
        ST7789_drawStringFast(0, y, buf, Font8_Table, FW8, 8, WHITE, BLACK);
        y += FH8;
    }

    /* 任务栈高水位 */
    {
        y += FH8;
        ST7789_drawStringFast(0, y, "Stacks: (TBD)", Font8_Table, FW8, 8, CYAN, BLACK);
    }
}
