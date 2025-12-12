#include "ui_app.h"
#include "lvgl.h"

// --- 外部资源声明 ---
LV_IMG_DECLARE(ui_logo);
LV_FONT_DECLARE(ui_font);

// --- 全局 UI 对象 ---
static lv_obj_t *chart_temp;
static lv_chart_series_t *ser_temp;
static lv_obj_t *label_ip;
static lv_obj_t *label_mem;

// UI 更新函数
void ui_update_view(const sys_model_t *data)
{
    // 1. 更新温度
    if (chart_temp && ser_temp)
    {
        lv_chart_set_next_value(chart_temp, ser_temp, (int)data->cpu_temp);
    }

    // 2. 更新内存
    if (label_mem)
    {
        lv_label_set_text_fmt(label_mem, "Mem: %d %%", data->mem_usage);
    }

    // 3. 更新 IP
    if (label_ip)
    {
        // 只有当 IP 发生变化时才刷新 Label (简单的性能优化)
        // 实际项目中可以加个 static char last_ip[32] 做对比
        lv_label_set_text_fmt(label_ip, "IP: %s", data->ip_addr);
    }
}

// --- 加载主监控界面 ---
static void load_main_screen(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr); // 清除 Logo

    // 1. 创建温度图表 (占据上半部分)
    chart_temp = lv_chart_create(scr);
    lv_obj_set_size(chart_temp, 150, 60);
    lv_obj_align(chart_temp, LV_ALIGN_TOP_MID, 0, 15);
    lv_chart_set_type(chart_temp, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart_temp, LV_CHART_AXIS_PRIMARY_Y, 20, 80); // 温度范围 20-80度
    lv_chart_set_point_count(chart_temp, 20);                        // 显示最近 20 个点
    lv_obj_set_style_bg_color(chart_temp, lv_color_hex(0x181818), 0);

    ser_temp = lv_chart_add_series(chart_temp, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

    // 2. 内存使用率
    label_mem = lv_label_create(scr);
    // 如果有中文字体，取消下面这行的注释
    lv_obj_set_style_text_font(label_mem, &ui_font, 0);
    lv_label_set_text(label_mem, "Mem: -- %");
    lv_obj_align(label_mem, LV_ALIGN_LEFT_MID, 10, 20);
    lv_obj_set_style_text_color(label_mem, lv_palette_main(LV_PALETTE_ORANGE), 0);

    // 3. IP 地址
    label_ip = lv_label_create(scr);
    lv_obj_set_style_text_font(label_ip, &ui_font, 0);
    lv_label_set_text(label_ip, "IP: Checking...");
    lv_obj_align(label_ip, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_text_color(label_ip, lv_palette_main(LV_PALETTE_CYAN), 0);
}

// --- 启动动画定时器 ---
static void splash_timer_cb(lv_timer_t *t)
{
    // 销毁单次定时器
    lv_timer_del(t);
    // 进入主界面
    load_main_screen();
}

// --- 程序入口 ---
void ui_app_entry(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_clean(scr);

    // 显示 Logo
    lv_obj_t *logo = lv_img_create(scr);

    // 如果还没有 img_logo.c，请注释这行，改用下面的 Label 替代
    lv_img_set_src(logo, &ui_logo);

    // 替代方案：如果没有图片，显示文字
    // lv_obj_t * logo = lv_label_create(scr);
    // lv_label_set_text(logo, "Taishan Pi\nSystem Boot");

    lv_obj_center(logo);

    // 创建一个单次定时器，2秒后跳转
    lv_timer_t *t = lv_timer_create(splash_timer_cb, 2000, NULL);
    lv_timer_set_repeat_count(t, 1);
}