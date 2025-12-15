#ifndef _UI_MP4_H
#define _UI_MP4_H

// 初始化并播放 MP4
int ui_mp4_init(const char *file_path);

// 停止并释放资源
void ui_mp4_deinit(void);

#endif