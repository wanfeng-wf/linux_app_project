#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

static volatile sig_atomic_t keep_running = 1;
void int_handler(int dummy) { keep_running = 0; }

int main(void)
{
    signal(SIGINT, int_handler);

    // LVGL 核心初始化
    lv_init();

    // 打开 /dev/fb1 并注册驱动
    if (lv_port_disp_init() != 0)
    {
        printf("Failed to init display!\n");
        return -1;
    }

    /* 初始化触摸：/dev/input/event5 并加载校准 */
    lv_port_indev_init();

    /* 创建测试按钮 */
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_center(btn);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Click Me!");
    lv_obj_center(label);

    while (keep_running)
    {
        uint32_t time_until_next = lv_timer_handler();
        if (time_until_next > 5)
            time_until_next = 5;
        usleep(time_until_next * 1000);
    }

    lv_port_disp_deinit();

    return 0;
}
