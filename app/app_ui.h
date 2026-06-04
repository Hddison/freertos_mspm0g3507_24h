/*
 *  ============ app_ui.h =============
 *  UI 工具函数: 数值格式化 + 区域擦写
 *
 *  从 main.c 提取, 保持原有逻辑不变
 */

#ifndef APP_UI_H
#define APP_UI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* float → string (1-2 位小数), 返回字符串长度 */
int app_ftoa(char *buf, float val, uint8_t dec);

/* DMA 擦 + 写: 在指定区域显示浮点数, 字体大小可变 */
void app_draw_val(uint16_t x, uint16_t y, float val, uint8_t dec,
                  const uint8_t *font_table, uint16_t font_w, uint16_t font_h,
                  uint16_t fg_color);

#ifdef __cplusplus
}
#endif

#endif /* APP_UI_H */
