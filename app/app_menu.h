/*
 * ============ app_menu.h =============
 * LCD 多级菜单状态机 + 屏幕渲染
 *
 * 按钮驱动: 消费 button_queue 事件, 更新菜单导航 & 数值编辑状态。
 * 渲染: 通过 ST7789 DMA 接口绘制, 仅刷新变化区域。
 *
 * 屏幕类型:
 *   SCREEN_STATUS      — 传感器仪表盘 (默认)
 *   SCREEN_MENU         — 可滚动菜单列表 + 光标
 *   SCREEN_VALUE_EDIT   — 数值编辑 (浮点, 粗/细步进)
 *   SCREEN_CONFIRM      — Y/N 确认对话框
 *   SCREEN_CONTEST      — 竞赛运行显示
 *   SCREEN_INFO         — 系统信息
 */

#ifndef APP_MENU_H
#define APP_MENU_H

#include <stdint.h>
#include <stdbool.h>
#include "task_button.h"   /* button_event_t, sensor_data_t */

/* ── 屏幕类型 ── */
typedef enum {
    SCREEN_STATUS = 0,
    SCREEN_MENU,
    SCREEN_VALUE_EDIT,
    SCREEN_CONFIRM,
    SCREEN_CONTEST,
    SCREEN_INFO,
    SCREEN_DEBUG_SENSOR,
    SCREEN_DEBUG_KALMAN,
    SCREEN_DEBUG_PID,
    SCREEN_DEBUG_SLIP,
    SCREEN_DEBUG_HEADING,
    SCREEN_DEBUG_SPEED,
    SCREEN_DEBUG_MOTOR_ID,
    SCREEN_DEBUG_POS_HOLD,
    SCREEN_DEBUG_LINE_TRACK
} screen_type_t;

/* ── 菜单项类型 ── */
#define MENU_TYPE_SUBMENU   0    /* 进入子菜单                     */
#define MENU_TYPE_VALUE     1    /* 编辑 float 值                  */
#define MENU_TYPE_ACTION    2    /* 执行回调函数                   */
#define MENU_TYPE_RETURN    3    /* 返回上级菜单                   */
#define MENU_TYPE_SCREEN    4    /* 切换到指定屏幕                 */

/* ── 菜单项 ── */
struct menu_state;  /* 前向声明 */

typedef struct menu_item {
    const char          *title;        /* 显示文本                    */
    uint8_t              type;         /* MENU_TYPE_*                  */
    const void          *target;       /* 子菜单数组 / float* / 回调  */
    uint8_t              id;           /* 标识符                      */
} menu_item_t;

/* ── 菜单状态机 ── */
#define MENU_VISIBLE_ROWS   7          /* 一屏可见 7 行              */
#define MENU_ROW_H          14         /* 行高 (px)                  */

typedef struct menu_state {
    screen_type_t       screen;        /* 当前屏幕类型               */
    const menu_item_t  *menu;          /* 当前菜单数组               */
    uint8_t             menu_count;    /* 菜单项数量                 */
    uint8_t             cursor;        /* 光标位置 (0-based)         */
    uint8_t             scroll_offset; /* 滚动偏移 (首可见项)        */

    /* 数值编辑 */
    float              *edit_value;    /* 正在编辑的 float*          */
    float               edit_step;     /* 细步进 (UP/DOWN)           */
    float               edit_coarse;   /* 粗步进 (LEFT/RIGHT)        */
    const char          *edit_label;   /* 参数名称                   */

    /* 确认对话框 */
    const char          *confirm_msg;
    bool                 confirm_yes;

    /* 竞赛状态 (LCD 本地副本) */
    uint8_t             task_id;
    uint8_t             lap;
    uint32_t            elapsed_ms;
    float               cur_speed;
    float               cur_heading;

    /* auto-return 计时 */
    uint32_t            last_input_tick;

    /* 渲染脏标记 */
    bool                needs_full_redraw;
    int16_t             prev_cursor;
    int16_t             prev_offset;

    /* 父菜单栈 (支持多级返回) */
    const menu_item_t  *parent_menu[4];
    uint8_t             parent_count[4];
    uint8_t             parent_cursor[4];
    uint8_t             parent_depth;
} menu_state_t;

/* ── API ── */

/* 初始化菜单 (默认 SCREEN_STATUS) */
void menu_init(menu_state_t *m);

/* 处理一个按键事件, 更新状态机
 * 返回: true = 屏幕内容可能变化, 需要重绘 */
bool menu_process_event(menu_state_t *m, const button_event_t *evt);

/* 渲染当前屏幕到 LCD (需已持有 SPI 锁)
 *  sensor: STATUS 屏用的传感器数据 (可为 NULL 表示无新数据)
 *  kf_x, kf_y, kf_theta: Kalman 状态 (INFO 屏用) */
void menu_render(const menu_state_t *m, const sensor_data_t *sensor,
                 float kf_x, float kf_y, float kf_theta);

/* 检查是否应触发竞赛命令 (用户选择了 Task1-4)
 * 返回: CMD_START_TASK1~4 / CMD_NONE */
ctrl_cmd_t menu_get_pending_cmd(const menu_state_t *m);

/* 清除待处理命令 */
void menu_clear_pending_cmd(menu_state_t *m);

/* 获取指向 PID float 的指针 (供 LCD 菜单编辑) */
float* menu_get_pid_ptr(int pid_id);

/* 保存回调 (供菜单 ACTION 绑定) */
void menu_action_save(void);

#endif /* APP_MENU_H */
