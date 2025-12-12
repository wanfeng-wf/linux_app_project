#ifndef _GET_SYS_STATUS_H
#define _GET_SYS_STATUS_H

#include <stddef.h>

// 1. 获取 CPU 温度 (单位: °C)
float get_cpu_temp(void);
// 2. 获取内存使用率 (单位: %)
int get_mem_usage(void);
// 3. 获取本机 IP 地址 (优先获取 wlan0，其次 eth0)
void get_ip_address(char *buffer, size_t len);

#endif