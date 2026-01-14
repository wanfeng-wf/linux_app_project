#include "lv_port_indev.h"
#include <tslib.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

static struct tsdev *ts = NULL;

static void touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    struct ts_sample samp;
    int ret;

    // 使用 ts_read 读取数据
    // 在非阻塞模式下，如果没有数据，ret 会立即返回 0 或 -1
    ret = ts_read(ts, &samp, 1);

    if (ret > 0)
    {
        if (samp.pressure > 0)
        {
            data->point.x = (lv_coord_t)samp.x;
            data->point.y = (lv_coord_t)samp.y;
            data->state   = LV_INDEV_STATE_PR;

            // 调试打印
            // printf("Touch: x=%d, y=%d\n", samp.x, samp.y);
        }
        else
        {
            // 抬起手指时，tslib 会上报 pressure 为 0 的样本
            data->state = LV_INDEV_STATE_REL;
        }
    }
    // 注意：如果 ret <= 0，不需要改变 data->state，
    // LVGL 会保留上一次的状态（通常是 REL）
}

void lv_port_indev_init(void)
{
    // 在初始化驱动之前，强制设置环境变量
    // 参数说明：变量名, 变量值, 1(表示如果已存在则覆盖)
    setenv("TSLIB_TSDEVICE", "/dev/input/event5", 1);
    setenv("TSLIB_CALIBFILE", "/etc/pointercal", 1);
    setenv("TSLIB_CONFFILE", "/etc/ts.conf", 1);
    setenv("TSLIB_PLUGINDIR", "/usr/lib/aarch64-linux-gnu/ts0", 1);

    /* 添加 TS_NOTBLOCK 标志 */
    /* 这会让 ts_setup 以非阻塞方式打开设备节点 */
    ts = ts_setup(NULL, 1); // 第二个参数非0表示非阻塞

    if (!ts)
    {
        perror("ts_setup failed");
        return;
    }

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);
}