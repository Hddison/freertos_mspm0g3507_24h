/*
 *  ============ hw_buzzer.h =============
 *  蜂鸣器驱动 — PB22 (SysConfig BEEP), 高电平响
 *
 *  用于电赛 H 题顶点声光提示
 */

#ifndef HW_BUZZER_H
#define HW_BUZZER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化 (PB22 输出, 初始 LOW) */
void Buzzer_init(void);

/* 开关 (1=响, 0=关) */
void Buzzer_set(uint8_t on);

/* 阻塞蜂鸣 (dur_ms 毫秒) */
void Buzzer_beep(uint16_t dur_ms);

/* 播放模式序列 (pat[] 交替 on/off 时间 ms, n 项) */
void Buzzer_pattern(const uint16_t *pat, uint8_t n);

/* 使能/失能 (菜单控制) */
void Buzzer_enable(bool en);
bool Buzzer_isEnabled(void);

/* ── 预定义蜂鸣模式 ── */

/* 顶点到达: 100ms × 3 */
extern const uint16_t BUZZ_VERTEX[];
/* 任务完成: 500+200+1000ms */
extern const uint16_t BUZZ_DONE[];
/* 错误: 持续 500ms */
extern const uint16_t BUZZ_ERROR[];

#ifdef __cplusplus
}
#endif
#endif
