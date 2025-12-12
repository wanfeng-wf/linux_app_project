#include <stdio.h>
#include <unistd.h>
#include "i2c_lib.h"
#include "oled.h"
#include "font.h"

int main()
{
    fd_i2c2 = i2c_init(2);
    if (fd_i2c2 < 0)
    {
        printf("I2C init failed\n");
        return -1;
    }

    // 1. 初始化 OLED
    OLED_Init();

    while (1)
    {
        // 中英文字符串混合显示
        OLED_NewFrame();
        OLED_PrintString(0, 0, "感谢关注", &font16x16, OLED_COLOR_REVERSED);
        OLED_ShowFrame();
        usleep(500 * 1000);
        OLED_PrintString(0, 22, "B站-KEYSKING", &font16x16, OLED_COLOR_NORMAL);
        OLED_ShowFrame();
        usleep(500 * 1000);
        OLED_PrintString(0, 44, "\\^o^/", &font16x16, OLED_COLOR_NORMAL);
        OLED_ShowFrame();
        usleep(1500 * 1000);
        // 显示变量值
        int count = 0;
        char buf[10] = {0};
        OLED_NewFrame();
        for (;;)
        {
            sprintf(buf, "%d %%", count);
            OLED_PrintASCIIString(40, 20, buf, &afont24x12, OLED_COLOR_NORMAL);
            OLED_ShowFrame();
            usleep(15 * 1000);
            if (count++ > 99)
            {
                break;
            }
        }
        usleep(1000 * 1000);
        // 直线绘制
        OLED_NewFrame();
        for (int i = 0; i < 128; i += 8)
        {
            OLED_DrawLine(0, 0, i, 63, OLED_COLOR_NORMAL);
            OLED_DrawLine(127, 0, 127 - i, 63, OLED_COLOR_NORMAL);
            OLED_ShowFrame();
            usleep(30 * 1000);
        }
        usleep(1500 * 1000);
        // 矩形绘制
        OLED_NewFrame();
        for (int i = 0; i < 64; i += 8)
        {
            OLED_DrawRectangle(i, i / 2, 127 - i * 2, 63 - i, OLED_COLOR_NORMAL);
            OLED_ShowFrame();
            usleep(35 * 1000);
        }
        usleep(1500 * 1000);
        // 矩形圆形
        OLED_NewFrame();
        for (int i = 63; i > 0; i -= 8)
        {
            OLED_DrawCircle(64, 32, i / 2, OLED_COLOR_NORMAL);
            OLED_ShowFrame();
            usleep(40 * 1000);
        }
        usleep(1500 * 1000);
        // 图片显示1
        OLED_NewFrame();
        OLED_DrawImage((128 - (bilibiliImg.w)) / 2, 0, &bilibiliImg, OLED_COLOR_NORMAL);
        OLED_ShowFrame();
        usleep(1700 * 1000);
        // 图片显示2
        OLED_NewFrame();
        OLED_DrawImage((128 - (bilibiliTVImg.w)) / 2, 0, &bilibiliTVImg, OLED_COLOR_NORMAL);
        OLED_ShowFrame();
        usleep(1700 * 1000);
    }

    // 4. 清理资源
    i2c_deinit(fd_i2c2);

    return 0;
}