#include "lvgl_port.h"
#include "lvgl.h"
#include "tft_st7735s.h"

/**
 * @brief 显示刷新回调函数，LVGL 渲染好一部分区域后，会调用这个函数让我们推送到屏幕
 * @param disp 显示驱动指针
 * @param area 要刷新的区域 (x1, y1, x2, y2)
 * @param color_p 像素颜色数组指针 (RGB565 格式)
 */
static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    // 计算当前刷新区域的宽高
    // 注意：ST7735S 是 128x160，LVGL 可能会分块渲染
    uint16_t w = area->x2 - area->x1 + 1;
    uint16_t h = area->y2 - area->y1 + 1;

    // 调用我们在 tft_st7735s.c 中写的底层函数
    // color_p 是像素数组，每个像素 2 字节 (RGB565)
    TFT_WriteData(area->x1, area->y1, area->x2, area->y2, (void *)color_p, w * h * 2);

    // 这一步非常重要！告诉 LVGL “我画完了，你可以继续计算下一块了”
    lv_disp_flush_ready(disp);
}

void lv_port_disp_init(void)
{
    // 1. 初始化显示缓冲区
    // 定义两个缓冲区，大小为屏幕大小
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[DISP_BUF_SIZE];
    static lv_color_t buf2[DISP_BUF_SIZE];
    // 如果想更丝滑，可以开双缓冲 buf2，但单缓冲对 ST7735 足够了
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DISP_BUF_SIZE);

    // 2. 初始化并注册显示驱动
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);        // 初始化结构体

    // 开启 full_refresh 可以避免局部刷新带来的残影，但会增加 SPI 负载
    // 对于播放视频，建议开启 partial (默认)，但对于全屏 UI，full_refresh 可能更稳
    disp_drv.full_refresh = 1;

    disp_drv.hor_res = MY_DISP_HOR_RES; // 设置水平分辨率
    disp_drv.ver_res = MY_DISP_VER_RES; // 设置垂直分辨率
    disp_drv.flush_cb = my_disp_flush;  // 设置上面的回调函数
    disp_drv.draw_buf = &draw_buf;      // 设置缓冲区

    // 修正偏移
    disp_drv.offset_x = 0;
    disp_drv.offset_y = 0;

    lv_disp_drv_register(&disp_drv);
}