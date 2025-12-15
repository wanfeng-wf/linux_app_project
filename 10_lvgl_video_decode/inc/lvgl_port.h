#ifndef _LVGL_PORT_H
#define _LVGL_PORT_H

#include <stdint.h>

#define MY_DISP_HOR_RES 160                                 // 水平分辨率
#define MY_DISP_VER_RES 128                                 // 垂直分辨率
#define DISP_BUF_SIZE   (MY_DISP_HOR_RES * MY_DISP_VER_RES) // 全屏大小

void lv_port_disp_init(void);

#endif // _LVGL_PORT_H
