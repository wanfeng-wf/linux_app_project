#ifndef _UI_APP_H
#define _UI_APP_H

#include "sys_model.h" // 引用模型定义

void ui_app_entry(void);

// 外部用来通知 UI 更新的接口
void ui_update_view(const sys_model_t *data);

#endif