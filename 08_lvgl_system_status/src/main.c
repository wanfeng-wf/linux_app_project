#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "bsp_spi.h"
#include "get_sys_status.h" // 底层驱动
#include "lvgl.h"
#include "lvgl_port.h"
#include "sys_model.h" // 模型
#include "tft_st7735s.h"
#include "ui_app.h" // UI 接口

// 运行标志位
static volatile sig_atomic_t keep_running = 1;

// 函数声明
void int_handler(int dummy);
void controller_timer_cb(lv_timer_t *timer);

int main(void)
{
    // 1. 注册退出信号 (Ctrl+C)
    signal(SIGINT, int_handler);
    printf("Starting LVGL Application...\n");

    // 2. 硬件初始化
    // SPI 速率设置为 50MHz (已验证的最佳速率)
    dev3_0 = spi_init(3, 0, 0, 50 * 1000000, 8);
    if (dev3_0 == NULL)
    {
        printf("SPI Init Failed!\n");
        return -1;
    }

    TFT_Init(); // 屏幕初始化

    // 3. LVGL 初始化
    lv_init();
    lv_port_disp_init(); // 注册显示驱动

    // 4. 创建 UI 界面
    ui_app_entry();
    lv_timer_create(controller_timer_cb, 1000, NULL);

    printf("Enter main loop. Press Ctrl+C to quit.\n");

    // 5. 主循环
    // 注意：由于在 lv_conf.h 中开启了 LV_TICK_CUSTOM，
    // 这里不再需要手动调用 lv_tick_inc()，也不需要手动计算时间差。
    while (keep_running)
    {
        // 处理 LVGL 任务，返回值为下一次任务还需要等待多久(ms)
        uint32_t time_until_next = lv_timer_handler();

        // 智能休眠：既不占满 CPU，又保证响应速度
        // 限制最大休眠时间为 5ms (可根据实际需求调整，例如 10ms)
        if (time_until_next > 5)
            time_until_next = 5;

        usleep(time_until_next * 1000);
    }

    // 6. 退出清理
    printf("\nExiting...\n");
    TFT_DeInit();
    spi_deinit(dev3_0);
    printf("Done.\n");

    return 0;
}

// 信号处理
void int_handler(int dummy) { keep_running = 0; }

// 定义一个 LVGL 定时器，充当 Controller 的角色
void controller_timer_cb(lv_timer_t *timer)
{
    static sys_model_t current_status;

    // 1. [Backend] 从底层获取数据
    // 这一步完全可以在此处做缓存逻辑，比如 IP 不需要每秒都读
    current_status.cpu_temp  = get_cpu_temp();
    current_status.mem_usage = get_mem_usage();

    // 简单的降频处理：每 5 秒才去读一次 IP，因为 ioctl 比较耗时
    static int ip_tick = 0;
    if (ip_tick++ >= 5)
    {
        get_ip_address(current_status.ip_addr, sizeof(current_status.ip_addr));
        ip_tick = 0;
    }
    else if (current_status.ip_addr[0] == 0)
    {
        // 初始化读取
        get_ip_address(current_status.ip_addr, sizeof(current_status.ip_addr));
    }

    // 2. [Frontend] 将数据推给 UI
    ui_update_view(&current_status);
}