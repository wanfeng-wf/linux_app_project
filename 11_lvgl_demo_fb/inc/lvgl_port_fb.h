#ifndef _LVGL_PORT_FB_H
#define _LVGL_PORT_FB_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

// 定义缓冲区大小 (例如 10 行的高度)，减小内存占用
// ST7735S (160px宽 * 10行 * 2字节 = 3200字节)
// 如果内存充足，可以设为全屏大小以获得更高性能
#define FB_BUF_SIZE_IN_PIXELS (160 * 128)

// 初始化 Linux Framebuffer 和 LVGL 显示驱动
int lv_port_fb_init(void);

// 退出清理 (清屏、关闭文件、释放映射)
void lv_port_fb_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // _LVGL_PORT_FB_H