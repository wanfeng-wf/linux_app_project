#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include "lvgl.h"
#include "lv_port_disp.h"

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
        printf("Failed to init framebuffer!\n");
        return -1;
    }

    // 创建 UI
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0); // 黑底

    // 按钮
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_center(btn);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Kernel FB!");

    // 顶部标题
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Success!");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);

    printf("Running...\n");

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