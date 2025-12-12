#ifndef _TFT_ST7735S_H
#define _TFT_ST7735S_H

#include <stdint.h>
#include <gpiod.h>
#include "font.h"
#include "image.h"

// 配置GPIO Chip 和 Line Offset
#define TFT_GPIO_CHIP_NAME "gpiochip3"
#define TFT_RST_OFFSET 3  // RST 引脚号
#define TFT_DC_OFFSET 2   // DC 引脚号
#define TFT_BLK_OFFSET 1 // 背光引脚号 (假设有，如果没有设为 -1)

// 全局 GPIO 句柄 (在 .c 文件中定义)
extern struct gpiod_line *line_rst;
extern struct gpiod_line *line_dc;
extern struct gpiod_line *line_blk;

// clang-format off
// GPIO 操作宏适配
// 注意：libgpiod 的 set_value 1表示高电平，0表示低电平
#define TFT_RST(x)  do { if(line_rst) gpiod_line_set_value(line_rst, x); } while(0)
#define TFT_DC(x)   do { if(line_dc)  gpiod_line_set_value(line_dc, x); } while(0)
#define TFT_BLK(x)  do { if(line_blk) gpiod_line_set_value(line_blk, x); } while(0)

// CS 操作适配：
// 在 Linux spidev 中，ioctl 传输时会自动拉低 CS，传输结束自动拉高。
// 因此这里定义为空，不再手动控制，防止冲突。
#define TFT_CS(x) ((void)0)

// 延时函数适配
void delay_ms(int ms);

// RGB565颜色定义保持不变
#define WHITE 0xffff
#define BLACK 0x0000
#define RED 0xf800
#define GREEN 0x0400
#define BLUE 0x001f
#define DARKBLUE 0x0011
#define PINK 0xfdf9
#define ORANGE 0xfd20
#define YELLOW 0xffe0
#define CYAN 0x07ff
#define PURPLE 0x8010
#define GRAY 0x8410
#define BROWN 0xa145

// 屏幕控制函数声明
void TFT_Init(void);
void TFT_DeInit(void); // 释放 GPIO 资源
void TFT_Reset(void);
void TFT_TurnOff(uint8_t on);
void TFT_SpinScreen(uint8_t rotate);

void TFT_NewFrame(uint16_t color);
void TFT_ShowFrame(void);
void TFT_ShowPartialFrame(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void TFT_SetPixel(uint8_t x, uint8_t y, uint16_t color);
// 供 LVGL 调用的底层接口
void TFT_WriteData(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, void *data, uint32_t len_in_bytes);

void TFT_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t color);
void TFT_DrawRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void TFT_DrawFilledRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void TFT_DrawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint16_t color);
void TFT_DrawFilledTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint16_t color);
void TFT_DrawCircle(uint8_t x, uint8_t y, uint8_t r, uint16_t color);
void TFT_DrawFilledCircle(uint8_t x, uint8_t y, uint8_t r, uint16_t color);
void TFT_DrawEllipse(uint8_t x, uint8_t y, uint8_t a, uint8_t b, uint16_t color);
void TFT_DrawImage(uint8_t x, uint8_t y, const Image *img);

void TFT_PrintASCIIChar(uint8_t x, uint8_t y, char ch, const ASCIIFont *font, uint16_t color, uint16_t bg_color);
void TFT_PrintASCIIString(uint8_t x, uint8_t y, char *str, const ASCIIFont *font, uint16_t color, uint16_t bg_color);
void TFT_PrintString(uint8_t x, uint8_t y, char *str, const Font *font, uint16_t color, uint16_t bg_color);

#endif