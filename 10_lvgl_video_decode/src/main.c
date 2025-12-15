#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "bsp_spi.h"
#include "tft_st7735s.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "ui_mp4.h"

// 运行标志位
static volatile sig_atomic_t keep_running = 1;

// 信号处理
void int_handler(int dummy) { keep_running = 0; }

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <video_file>\n", argv[0]);
        return -1;
    }
    
    // 注册退出信号 (Ctrl+C)
    signal(SIGINT, int_handler);
    printf("Starting LVGL Application...\n");

    // SPI 速率设置为 50MHz
    dev3_0 = spi_init(3, 0, 0, 50 * 1000000, 8);
    if (dev3_0 == NULL)
    {
        printf("SPI Init Failed!\n");
        return -1;
    }

    TFT_Init(); // 屏幕初始化

    lv_init();           // LVGL 初始化
    lv_port_disp_init(); // 注册显示驱动

    // 播放视频
    if (ui_mp4_init(argv[1]) == 0)
    {
        printf("MP4 Decoding started...\n");
    }
    else
    {
        printf("MP4 Init failed!\n");
    }

    printf("MP4 playing...Press Ctrl+C to quit.\n");

    // 由于在 lv_conf.h 中开启了 LV_TICK_CUSTOM，
    // 这里不再需要手动调用 lv_tick_inc()，也不需要手动计算时间差
    // 而是依赖 LVGL 内部的定时器机制，自动触发 lv_timer_handler()
    while (keep_running)
    {
        // 处理 LVGL 任务，返回值为下一次任务还需要等待多久(ms)
        uint32_t time_until_next = lv_timer_handler();

        // 智能休眠：既不占满 CPU，又保证响应速度
        // 限制最大休眠时间为 5ms (可根据实际需求调整)
        if (time_until_next > 5)
            time_until_next = 5;

        usleep(time_until_next * 1000);
    }

    printf("\nExiting...\n");
    ui_mp4_deinit();
    TFT_DeInit();
    spi_deinit(dev3_0);
    printf("Done.\n");

    return 0;
}
