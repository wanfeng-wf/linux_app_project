#include "ui_video.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>

// 屏幕参数
#define VIDEO_W    160
#define VIDEO_H    128
#define FRAME_SIZE (VIDEO_W * VIDEO_H * 2) // RGB565: 2字节/像素 -> 40KB/帧

static FILE *video_fp = NULL;
static lv_obj_t *img_display;
static lv_timer_t *video_timer;

// 帧缓冲区 (静态分配或 malloc 均可)
// 注意：必须保持 static 或全局，因为 LVGL 的 Image 对象直接引用这个指针，不会拷贝数据
static uint8_t frame_buffer[FRAME_SIZE];

// Image Descriptor
static lv_img_dsc_t img_dsc = {
    .header.always_zero = 0,
    .header.w           = VIDEO_W,
    .header.h           = VIDEO_H,
    .data_size          = FRAME_SIZE,
    .header.cf          = LV_IMG_CF_TRUE_COLOR, // RGB565
    .data               = frame_buffer,         // 指向我们的缓冲区
};

// --- 定时器：读取下一帧并刷新 ---
static void video_timer_cb(lv_timer_t *t)
{
    if (!video_fp)
        return;

    // 1. 从文件读取一帧数据 (40KB) 到缓冲区
    size_t read_len = fread(frame_buffer, 1, FRAME_SIZE, video_fp);

    if (read_len < FRAME_SIZE)
    {
        // 读不到数据了，说明播放结束
        // 选项 A: 循环播放
        // fseek(video_fp, 0, SEEK_SET);
        // read_len = fread(frame_buffer, 1, FRAME_SIZE, video_fp);
        // if (read_len < FRAME_SIZE)
        // {
        //     return;
        // }

        // 选项 B: 播放结束，停止播放
        lv_timer_pause(t);
        fclose(video_fp);
        video_fp = NULL;
        return;
    }

    // 2. 标记图片对象为“脏”，通知 LVGL 重绘
    // 因为 img_dsc.data 指针没变，但内存里的数据变了
    lv_obj_invalidate(img_display);
}

void ui_video_init(const char *file_path)
{
    // 1. 打开视频文件
    video_fp = fopen(file_path, "rb");
    if (!video_fp)
    {
        printf("Error: Cannot open video file: %s\n", file_path);
        return;
    }

    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 2. 创建 Image 对象
    img_display = lv_img_create(scr);
    lv_img_set_src(img_display, &img_dsc); // 设置源为自定义的描述符
    lv_obj_center(img_display);

    // 3. 创建定时器，33ms 刷新一次 (约 30 FPS)
    video_timer = lv_timer_create(video_timer_cb, 33, NULL);
}

void ui_video_deinit(void)
{
    if (video_timer)
    {
        lv_timer_del(video_timer);
        video_timer = NULL;
    }
    if (video_fp)
    {
        fclose(video_fp);
        video_fp = NULL;
    }
}