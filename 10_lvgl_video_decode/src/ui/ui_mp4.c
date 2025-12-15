#include "ui_mp4.h"
#include "lvgl.h"
#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

// 目标参数 (屏幕参数)
#define SCREEN_W       160
#define SCREEN_H       128
#define SCREEN_PIX_FMT AV_PIX_FMT_RGB565BE // ST7735 通常是小端 RGB565

// 全局变量
static AVFormatContext *pFormatCtx = NULL;
static AVCodecContext *pCodecCtx   = NULL;
static const AVCodec *pCodec       = NULL;
static AVFrame *pFrame             = NULL; // 解码后的原始帧 (YUV)
static AVFrame *pFrameRGB          = NULL; // 转换后的目标帧 (RGB565)
static AVPacket *packet            = NULL;
static struct SwsContext *sws_ctx  = NULL;
static int videoStream             = -1;
static int scaled_w, scaled_h; // 缩放后的实际视频尺寸
static int offset_x, offset_y; // 居中需要的偏移量
static int screen_linesize;    // 屏幕一行的字节数

// LVGL 相关
static lv_obj_t *img_display;
static lv_timer_t *mp4_timer;
static lv_img_dsc_t img_dsc;
static uint8_t *display_buffer = NULL; // 指向 pFrameRGB->data[0]

// --- 定时器：解码下一帧 ---
static void mp4_timer_cb(lv_timer_t *t)
{
    int ret;

    // 1. 读取一帧压缩数据 (Packet)
    while (av_read_frame(pFormatCtx, packet) >= 0)
    {
        if (packet->stream_index == videoStream)
        {
            // 2. 发送 Packet 到解码器
            ret = avcodec_send_packet(pCodecCtx, packet);
            if (ret < 0)
            {
                printf("Error sending packet for decoding\n");
                av_packet_unref(packet);
                continue;
            }

            // 3. 从解码器接收解码后的 Frame (YUV)
            ret = avcodec_receive_frame(pCodecCtx, pFrame);
            if (ret == 0) // 解码成功
            {
                // 3.1 [Pad] 涂黑背景
                // RGB565 中黑色是 0x0000，直接 memset 清零即可
                // 这一步模拟了 pad filter 的 "black"
                memset(display_buffer, 0, SCREEN_W * SCREEN_H * 2);

                // 3.2 [Scale & Position] 构造目标数据指针
                uint8_t *dst_data[4];
                int dst_linesize[4];

                // 技巧：告诉 sws_scale 目标的一行有多长（即屏幕全宽 160像素的字节数）
                // 这样 sws_scale 画完一行 scaled_w 后，会自动跳过剩下的像素，去下一行
                dst_linesize[0] = screen_linesize;

                // 技巧：把写入的起始指针偏移到 (offset_x, offset_y)
                // RGB565 每个像素 2 字节
                dst_data[0] = display_buffer + (offset_y * screen_linesize) + (offset_x * 2);

                // 3.3 执行转换
                // sws_scale 会把图片画在 display_buffer 的中间区域
                sws_scale(sws_ctx, (const uint8_t *const *)pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height,
                          dst_data, dst_linesize);

                // 3.4 刷新 LVGL
                lv_obj_invalidate(img_display);

                av_packet_unref(packet);
                return;
            }
        }
        av_packet_unref(packet);
    }

    // 如果读不到 packet 了 (文件结尾)
    // 循环播放:
    avio_seek(pFormatCtx->pb, 0, SEEK_SET);
    av_seek_frame(pFormatCtx, videoStream, 0, AVSEEK_FLAG_BACKWARD);
}

int ui_mp4_init(const char *file_path)
{
    // 1. 初始化 LVGL Image 对象 (先占位)
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    img_display = lv_img_create(scr);
    lv_obj_center(img_display);

    // 2. 打开视频文件
    if (avformat_open_input(&pFormatCtx, file_path, NULL, NULL) != 0)
    {
        printf("Could not open file %s\n", file_path);
        return -1;
    }

    // 3. 获取流信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return -1;

    // 4. 寻找视频流
    videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
            break;
        }
    }
    if (videoStream == -1)
        return -1;

    // 5. 查找解码器
    AVCodecParameters *pCodecPar = pFormatCtx->streams[videoStream]->codecpar;
    pCodec                       = avcodec_find_decoder(pCodecPar->codec_id);
    if (pCodec == NULL)
        return -1;

    // 6. 初始化解码器上下文
    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pCodecPar);
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        return -1;

    // 7. 分配帧内存
    pFrame    = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    packet    = av_packet_alloc();

    // 计算 RGB565 缓冲区大小并分配
    int numBytes    = av_image_get_buffer_size(SCREEN_PIX_FMT, SCREEN_W, SCREEN_H, 1);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

    // 将 pFrameRGB 关联到 buffer
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer,
                         SCREEN_PIX_FMT, SCREEN_W, SCREEN_H, 1);

    // 记录 buffer 指针给 LVGL 用
    display_buffer = buffer;

    // 8. 初始化格式转换上下文 (重点：缩放 + 格式转换)
    // === 核心算法：计算缩放尺寸 (模拟 force_original_aspect_ratio=decrease) ===
    // 目标是放入 SCREEN_W x SCREEN_H
    int src_w = pCodecCtx->width;
    int src_h = pCodecCtx->height;

    // 算法：比较宽高比，决定是以宽为基准还是以高为基准
    // 使用浮点数计算更精确，最后转回 int
    float scale_w = (float)SCREEN_W / src_w;
    float scale_h = (float)SCREEN_H / src_h;
    float scale   = (scale_w < scale_h) ? scale_w : scale_h; // 取较小值(Decrease)

    scaled_w = (int)(src_w * scale);
    scaled_h = (int)(src_h * scale);

    // 确保宽高是偶数 (有些格式转换不喜欢奇数)
    scaled_w &= ~1;
    scaled_h &= ~1;

    // 计算居中偏移量 (模拟 pad)
    offset_x = (SCREEN_W - scaled_w) / 2;
    offset_y = (SCREEN_H - scaled_h) / 2;

    printf("Src: %dx%d -> Scaled: %dx%d, Offset: (%d, %d)\n",
           src_w, src_h, scaled_w, scaled_h, offset_x, offset_y);

    // 计算屏幕缓冲区大小 (固定为 160x128)
    numBytes   = av_image_get_buffer_size(SCREEN_PIX_FMT, SCREEN_W, SCREEN_H, 1);
    display_buffer = (uint8_t *)av_malloc(numBytes);

    // 这里的 linesize 是屏幕全宽的步长 (160 * 2 = 320 字节)
    screen_linesize = av_image_get_linesize(SCREEN_PIX_FMT, SCREEN_W, 0);

    // sws_getContext 的目标尺寸必须是【scaled_w, scaled_h】
    // 而不是屏幕的全尺寸！否则 sws_scale 会把视频强行拉伸填满，导致变形。
    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                             scaled_w, scaled_h, SCREEN_PIX_FMT,
                             SWS_BILINEAR, NULL, NULL, NULL);

    // 9. 配置 LVGL Image Descriptor
    img_dsc.header.w  = SCREEN_W; // 告诉 LVGL 图片是 160
    img_dsc.header.h  = SCREEN_H; // 告诉 LVGL 图片是 128
    img_dsc.data_size = numBytes;
    img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    img_dsc.data      = display_buffer;

    lv_img_set_src(img_display, &img_dsc);

    // 10. 启动定时器 (33ms = 30FPS)
    // 实际可以通过 av_q2d(pFormatCtx->streams[videoStream]->avg_frame_rate) 动态计算
    mp4_timer = lv_timer_create(mp4_timer_cb, 33, NULL);

    return 0;
}

void ui_mp4_deinit(void)
{
    if (mp4_timer)
        lv_timer_del(mp4_timer);

    // 释放 FFmpeg 资源
    if (display_buffer)
        av_free(display_buffer);
    if (pFrame)
        av_frame_free(&pFrame);
    if (pFrameRGB)
        av_frame_free(&pFrameRGB);
    if (packet)
        av_packet_free(&packet);
    if (pCodecCtx)
        avcodec_close(pCodecCtx);
    if (pFormatCtx)
        avformat_close_input(&pFormatCtx);
    if (sws_ctx)
        sws_freeContext(sws_ctx);
}