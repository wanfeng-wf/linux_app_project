#ifndef _SYS_MODEL_H
#define _SYS_MODEL_H

#include <stdint.h>

typedef struct
{
    float cpu_temp;   // CPU 温度
    int mem_usage;    // 内存使用率 0-100
    char ip_addr[32]; // IP 地址字符串
} sys_model_t;

#endif