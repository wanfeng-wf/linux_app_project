#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include "bsp_spi.h"
#include "tft_st7735s.h" //ST7735S驱动，与TFT操作相关
#include "font.h"        //字体数据
#include "image.h"       //图片数据

static volatile sig_atomic_t keep_running = 1;

void int_handler(int dummy)
{
    keep_running = 0; // 收到 Ctrl+C 时，将标志置为 0
}

int main(void)
{
    signal(SIGINT, int_handler);

    dev3_0 = spi_init(3, 0, 0, 15 * 1000000, 8);
    if (dev3_0 == NULL)
    {
        printf("SPI Init Failed!\n");
        return -1;
    }
    TFT_Init(); // 初始化TFT,默认黑屏

    while (keep_running)
    {
        TFT_SpinScreen(0);
        TFT_NewFrame(BLACK); // 新帧
        // TFT_PrintString(0, 0, "你~ABC", &font12x12, WHITE, RED);
        // TFT_PrintString(0, 12, "鸡你太美", &font12x12, BLACK, YELLOW);
        // TFT_PrintString(0, 24, "你~ABC", &font16x16, WHITE, RED);
        // TFT_PrintString(0, 40, "鸡你太美", &font16x16, BLACK, YELLOW);
        // TFT_PrintString(0, 56, "你~ABC", &font24x24, WHITE, RED);
        // TFT_PrintString(0, 80, "鸡你太美", &font24x24, BLACK, YELLOW);
        TFT_DrawImage(0, 0, &Img1);
        TFT_ShowFrame(); // 显示帧

        // 使用较短的 sleep 并在中间检查 keep_running
        // 如果 sleep 太长，按 Ctrl+C 会有明显延迟
        for (int i = 0; i < 5; i++)
        {
            if (!keep_running)
                break;
            usleep(100 * 1000);
        }
        if (!keep_running)
            break;

        TFT_SpinScreen(1);
        TFT_NewFrame(BLACK);
        TFT_DrawImage(0, 0, &Img2);
        TFT_ShowFrame();

        for (int i = 0; i < 5; i++)
        {
            if (!keep_running)
                break;
            usleep(100 * 1000);
        }
    }

    // 释放 GPIO
    TFT_DeInit();
    // 释放 SPI
    spi_deinit(dev3_0);
    printf("\nExiting...\n");
    return 0;
}
