#ifndef __FONT_H
#define __FONT_H

#include "stdint.h"

/**
 * @brief ASCII字体
 */
typedef struct ASCIIFont
{
    uint8_t h;      // 字高度
    uint8_t w;      // 字宽度
    uint8_t *chars; // 字库
} ASCIIFont;

extern const ASCIIFont afont12x6;
extern const ASCIIFont afont16x8;
extern const ASCIIFont afont24x12;

typedef struct FontIndex
{
    const uint8_t *sample; // 中文查找表
    uint8_t code_size;     // 每个中文字符的编码字节数(UTF-8汉字为3字节)
    uint16_t len;          // 表长度
} FontIndex;

/**
 * @brief 字体结构体
 * @note  字库前4字节存储utf8编码 剩余字节存储字模数据
 */
typedef struct Font
{
    uint8_t h;              // 字高度
    uint8_t w;              // 字宽度
    const uint8_t *chars;   // 字库
    uint8_t char_size;      // 每个字符占用的字节数
    uint8_t len;            // 字库长度 超过256则请改为uint16_t
    const FontIndex *index; // 中文字查找表
    const ASCIIFont *ascii; // 缺省ASCII字体 当字库中没有对应字符且需要显示ASCII字符时使用
} Font;

extern const Font font12x12;
extern const Font font16x16;
extern const Font font24x24;

#endif // __FONT_H
