#include "lv_port_indev.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string.h>

#define TOUCH_INPUT_DEV "/dev/input/event5"        

static int ts_fd = -1;
/* 7个校准系数：a0...a6 */
static int cal[7];

/**
 * @brief 从 /etc/pointercal 读取校准系数
 */
static int load_calibration(void)
{
    FILE *f = fopen("/etc/pointercal", "r");
    if (!f)
    {
        printf("Warning: /etc/pointercal not found, using default.\n");
        // 默认不校准的系数 (通常是 1 0 0 0 1 0 1)
        cal[0] = 1;
        cal[1] = 0;
        cal[2] = 0;
        cal[3] = 0;
        cal[4] = 1;
        cal[5] = 0;
        cal[6] = 1;
        return -1;
    }
    if (fscanf(f, "%d %d %d %d %d %d %d", &cal[0], &cal[1], &cal[2], &cal[3], &cal[4], &cal[5], &cal[6]) != 7)
    {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

/**
 * @brief 触摸读取回调：手动进行矩阵换算
 */
static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    struct input_event ev;
    static int raw_x = 0, raw_y = 0, pressed = 0;

    while (read(ts_fd, &ev, sizeof(ev)) > 0)
    {
        if (ev.type == EV_ABS)
        {
            if (ev.code == ABS_X)
                raw_x = ev.value;
            if (ev.code == ABS_Y)
                raw_y = ev.value;
        }
        else if (ev.type == EV_KEY && ev.code == BTN_TOUCH)
        {
            pressed = ev.value;
        }
        else if (ev.type == EV_SYN && ev.code == SYN_REPORT)
        {
            if (pressed)
            {
                data->state = LV_INDEV_STATE_PR;
                /* 应用校准公式 [a0*x + a1*y + a2] / a6 */
                data->point.x = (cal[0] * raw_x + cal[1] * raw_y + cal[2]) / cal[6];
                data->point.y = (cal[3] * raw_x + cal[4] * raw_y + cal[5]) / cal[6];
            }
            else
            {
                data->state = LV_INDEV_STATE_REL;
            }
            break;
        }
    }
}

void lv_port_indev_init(void)
{
    // 1. 加载校准文件
    load_calibration();

    // 2. 打开原始输入设备（请根据实际修改 /dev/input/eventX）
    ts_fd = open(TOUCH_INPUT_DEV, O_RDONLY | O_NONBLOCK);
    if (ts_fd < 0)
    {
        perror("Error opening touch device");
        return;
    }

    // 3. 注册 LVGL 驱动
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);
}