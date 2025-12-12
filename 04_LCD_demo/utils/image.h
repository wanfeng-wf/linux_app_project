#ifndef __IMAGE_H
#define __IMAGE_H

#include "stdint.h"

/**
 * @brief 图片结构体
 */
typedef struct Image
{
    uint8_t w;           // 图片宽度
    uint8_t h;           // 图片高度
    const uint8_t *data; // 图片数据
} Image;

extern const Image Img1;
extern const Image Img2;

#endif // __IMAGE_H
