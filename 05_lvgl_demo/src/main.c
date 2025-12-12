#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include "bsp_spi.h"
#include "tft_st7735s.h"
#include "lvgl.h"
#include "lvgl_port.h"

// 运行标志位
static volatile sig_atomic_t keep_running = 1;

// 信号处理
void int_handler(int dummy)
{
    keep_running = 0;
}

int main(void)
{
    // 1. 注册 Ctrl+C 信号
    signal(SIGINT, int_handler);

    printf("Starting LVGL Demo...\n");

    // 2. 硬件初始化 (SPI & GPIO)
    dev3_0 = spi_init(3, 0, 0, 15000000, 8);
    if (dev3_0 == NULL)
    {
        printf("SPI Init Failed!\n");
        return -1;
    }

    TFT_Init(); // 屏幕初始化 (默认全黑)

    // 3. LVGL 核心初始化
    lv_init();

    // 4. 初始化显示缓冲区，注册显示驱动
    lv_port_disp_init();

    // 5. 主循环 (LVGL 心跳)
    printf("Enter main loop. Press Ctrl+C to quit.\n");

    // 调试边界
    lv_obj_t *obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj, 160, 128); // 设为全屏大小
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_make(255, 0, 0), 0); // 纯红色
    lv_obj_set_style_border_width(obj, 4, 0);
    lv_obj_set_style_border_color(obj, lv_color_make(0, 255, 0), 0); // 绿色边框

    while (keep_running)
    {
        // 处理 LVGL 任务 (渲染、动画、输入等)
        lv_timer_handler();

        // 简单的时间管理：每次休眠 5ms
        usleep(5000);

        // 告诉 LVGL 过去了 5ms
        lv_tick_inc(5);
    }

    // 6. 退出清理
    printf("\nExiting...\n");
    TFT_DeInit();       // 关灯、释放 GPIO
    spi_deinit(dev3_0); // 释放 SPI
    printf("Done.\n");

    return 0;
}