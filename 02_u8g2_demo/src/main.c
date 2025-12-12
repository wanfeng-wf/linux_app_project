#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include "oled_port.h"
#include "u8g2.h"

volatile int keep_running = 1;

void handle_signal(int signo);
void draw(u8g2_t *u8g2);

int main()
{
    // 注册信号捕获
    // SIGINT  = Ctrl+C
    signal(SIGINT, handle_signal);
    
    u8g2_t u8g2;
    u8g2_init(&u8g2);

    while (keep_running)
    {
        u8g2_FirstPage(&u8g2);
        do
        {
            draw(&u8g2);
        } while (u8g2_NextPage(&u8g2));
    }

    //退出前的清理工作
    u8g2_deinit(&u8g2);
    printf("\nOLED Cleared.\n");
    return 0;
}

void handle_signal(int signo)
{
    if (signo == SIGINT)
    {
        keep_running = 0;
    }
}

void draw(u8g2_t *u8g2)
{
    u8g2_SetFontMode(u8g2, 1);              /*字体模式选择*/
    u8g2_SetFontDirection(u8g2, 0);         /*字体方向选择*/
    u8g2_SetFont(u8g2, u8g2_font_inb24_mf); /*字库选择*/
    u8g2_DrawStr(u8g2, 0, 20, "U");

    u8g2_SetFontDirection(u8g2, 1);
    u8g2_SetFont(u8g2, u8g2_font_inb30_mn);
    u8g2_DrawStr(u8g2, 21, 8, "8");

    u8g2_SetFontDirection(u8g2, 0);
    u8g2_SetFont(u8g2, u8g2_font_inb24_mf);
    u8g2_DrawStr(u8g2, 51, 30, "g");
    u8g2_DrawStr(u8g2, 67, 30, "\xb2");

    u8g2_DrawHLine(u8g2, 2, 35, 47);
    u8g2_DrawHLine(u8g2, 3, 36, 47);
    u8g2_DrawVLine(u8g2, 45, 32, 12);
    u8g2_DrawVLine(u8g2, 46, 33, 12);

    u8g2_SetFont(u8g2, u8g2_font_4x6_tr);
    u8g2_DrawStr(u8g2, 1, 54, "github.com/olikraus/u8g2");
}