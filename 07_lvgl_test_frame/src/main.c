#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "bsp_spi.h"
#include "tft_st7735s.h"
#include "lvgl.h"
#include "lvgl_port.h"

// 声明外部图片资源
LV_IMG_DECLARE(img_heart);

// 运行标志位
static volatile sig_atomic_t keep_running = 1;

// 图表对象句柄 (用于定时更新)
static lv_obj_t *chart;
static lv_chart_series_t *ser1;

// 函数原型
void int_handler(int dummy);
void ui_smart_watch_demo(void);

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
    ui_smart_watch_demo();

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
void int_handler(int dummy)
{
    keep_running = 0;
}

// 定时器回调：模拟数据跳动
static void timer_cb(lv_timer_t *timer)
{
    // 1. 更新图表数据
    lv_chart_set_next_value(chart, ser1, rand() % 100);

    // 2. 暴力测试：每帧修改全屏背景色
    // 这会迫使 LVGL 计算并传输 160x128 的每一个像素
    static bool toggle = false;
    lv_obj_t *scr = lv_scr_act();
    if (toggle)
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x101010), 0);
    else
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    toggle = !toggle;

    // 强制重绘 (虽然改样式会自动重绘，但为了保险)
    lv_obj_invalidate(scr);
}

// 智能仪表盘演示界面
void ui_smart_watch_demo(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr); // 清空屏幕
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 1. 创建主容器 (列布局)
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, 160, 128);
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont, 5, 0);
    lv_obj_set_style_pad_gap(cont, 5, 0);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    // 2. 顶部标题栏
    lv_obj_t *header = lv_obj_create(cont);
    lv_obj_set_size(header, lv_pct(100), 25);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x202020), 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);

    lv_obj_t *time_label = lv_label_create(header);
    lv_label_set_text(time_label, "12:45");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);

    lv_obj_t *bat_label = lv_label_create(header);
    lv_label_set_text(bat_label, "85%");
    lv_obj_set_style_text_color(bat_label, lv_palette_main(LV_PALETTE_GREEN), 0);

    // 3. 中间图表
    chart = lv_chart_create(cont);
    lv_obj_set_size(chart, lv_pct(100), 60);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x101010), 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x404040), LV_PART_MAIN);

    ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_PRIMARY_Y);
    // 预填数据
    for (int i = 0; i < 10; i++)
    {
        lv_chart_set_next_value(chart, ser1, rand() % 100);
    }

    // 4. 底部状态栏
    lv_obj_t *footer = lv_obj_create(cont);
    lv_obj_set_size(footer, lv_pct(100), 30);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(footer, lv_color_black(), 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 0, 0);
    lv_obj_set_style_pad_gap(footer, 5, 0);

    // 图标
    lv_obj_t *icon = lv_img_create(footer);
    lv_img_set_src(icon, &img_heart);

    // 文字
    lv_obj_t *status_label = lv_label_create(footer);
    lv_label_set_text(status_label, "Health 99%");
    lv_obj_set_style_text_color(status_label, lv_palette_main(LV_PALETTE_BLUE_GREY), 0);

    // 5. 启动动态更新定时器 (200ms 刷新一次数据)
    lv_timer_create(timer_cb, 200, NULL);
}