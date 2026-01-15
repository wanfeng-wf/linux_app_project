#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

static volatile sig_atomic_t keep_running = 1;
void int_handler(int dummy) { keep_running = 0; }

static void slider_event_cb(lv_event_t *e);
static void create_brightness_slider(void);

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

    // 创建 UI 控件
    create_brightness_slider();

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

/**
 * @brief 滑块事件回调函数
 */
static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);

    // 获取当前滑块的值 (0-100)
    int brightness = (int)lv_slider_get_value(slider);

    // 调用底层 PWM 调节函数
    lv_port_set_brightness(brightness);

    // 打印调试信息
    // printf("Brightness set to: %d%%\n", brightness);
}

/**
 * @brief 创建亮度控制 UI
 */
static void create_brightness_slider(void)
{
    /* 创建一个滑块 */
    lv_obj_t *slider = lv_slider_create(lv_scr_act());
    lv_obj_set_width(slider, 200); // 设置宽度
    lv_obj_center(slider);         // 居中显示

    /* 设置范围 0-100 */
    lv_slider_set_range(slider, 0, 100);

    /* 设置初始位置 (与 pwm_init 中的初始亮度同步) */
    lv_slider_set_value(slider, 80, LV_ANIM_OFF);

    /* 添加事件回调：当值改变时触发 */
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 在滑块上方添加一个标签 */
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Backlight Control");
    lv_obj_align_to(label, slider, LV_ALIGN_OUT_TOP_MID, 0, -15);
}