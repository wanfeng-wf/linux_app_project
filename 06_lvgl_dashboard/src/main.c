#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include "bsp_spi.h"
#include "tft_st7735s.h"
#include "lvgl.h"
#include "lvgl_port.h"

static volatile sig_atomic_t keep_running = 1; // 运行标志位
static lv_obj_t *chart;
static lv_chart_series_t *ser1;

void int_handler(int dummy);
void ui_smart_watch_demo(void);

int main(void)
{
    // 1. 注册 Ctrl+C 信号
    signal(SIGINT, int_handler);

    printf("Starting LVGL Demo...\n");

    // 2. 硬件初始化 (SPI & GPIO)
    dev3_0 = spi_init(3, 0, 0, 15 * 1000000, 8);
    if (dev3_0 == NULL)
    {
        printf("SPI Init Failed!\n");
        return -1;
    }

    TFT_Init(); // 屏幕初始化 (默认全黑)

    // 3. LVGL 核心初始化
    lv_init();

    // 4. 初始化显示缓冲区，注册显示驱动
    lv_port_disp_init();

    // 5. 主循环 (LVGL 心跳)
    printf("Enter main loop. Press Ctrl+C to quit.\n");

    ui_smart_watch_demo();

    while (keep_running)
    {
        // 处理 LVGL 任务 (渲染、动画、输入等)
        lv_timer_handler();

        // 简单的时间管理：每次休眠 5ms
        usleep(5000);

        // 告诉 LVGL 过去了 5ms
        lv_tick_inc(5);
    }

    // 6. 退出清理
    printf("\nExiting...\n");
    TFT_DeInit();       // 关灯、释放 GPIO
    spi_deinit(dev3_0); // 释放 SPI
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
    // 更新图表数据
    lv_chart_set_next_value(chart, ser1, rand() % 100);
}

// 智能仪表盘演示界面
void ui_smart_watch_demo(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);                                   // 清空屏幕
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0); // 纯黑背景

    // 1. 创建主容器 (列布局)
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, 160, 128);                 // 强制设为屏幕大小
    lv_obj_center(cont);                             // 居中
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN); // 垂直排列
    lv_obj_set_style_pad_all(cont, 5, 0);            // 只有 5px 边距
    lv_obj_set_style_pad_gap(cont, 5, 0);            // 控件间距 5px
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont, 0, 0);              // 去除边框
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF); // 隐藏滚动条

    // 2. 顶部标题栏 (行布局)
    lv_obj_t *header = lv_obj_create(cont);
    lv_obj_set_size(header, lv_pct(100), 25); // 宽100%, 高25px
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x202020), 0); // 深灰色
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);

    lv_obj_t *time_label = lv_label_create(header);
    lv_label_set_text(time_label, "12:45");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);

    lv_obj_t *bat_label = lv_label_create(header);
    lv_label_set_text(bat_label, "85%");
    lv_obj_set_style_text_color(bat_label, lv_palette_main(LV_PALETTE_GREEN), 0);

    // 3. 中间图表 (Chart)
    chart = lv_chart_create(cont);
    lv_obj_set_size(chart, lv_pct(100), 60); // 宽100%, 高60px
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x101010), 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x404040), LV_PART_MAIN); // 网格颜色

    // 添加数据系列
    ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_PRIMARY_Y);
    // 预填一些数据
    for (int i = 0; i < 10; i++)
    {
        lv_chart_set_next_value(chart, ser1, rand() % 100);
    }

    // 4. 底部状态文字
    lv_obj_t *status_label = lv_label_create(cont);
    lv_label_set_text(status_label, "System Normal");
    lv_obj_set_style_text_color(status_label, lv_palette_main(LV_PALETTE_BLUE_GREY), 0);
    lv_obj_center(status_label); // 在容器交叉轴居中(水平居中)

    // 创建定时器让图表动起来
    lv_timer_create(timer_cb, 200, NULL);
}