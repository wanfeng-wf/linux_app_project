#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "oled_port.h"
#include "u8g2.h"

#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp" // RK3566 CPU温度节点
#define NETWORK_IF "wlan0"                                // 如果用WIFI，请修改为 "wlan0"

// 全局运行标志位，使用 volatile 确保多线程/中断安全
volatile int keep_running = 1;

void handle_signal(int signo);
float get_cpu_temp();
void get_ip_address(const char *iface_name, char *ip_buffer);
void draw_system_status(u8g2_t *u8g2, float cpu_temp, const char *ip_addr);

int main()
{
    char ip_buf[16] = {0};
    float cpu_temp = 0.0;

    // 注册信号捕获
    // SIGINT  = Ctrl+C
    signal(SIGINT, handle_signal);

    u8g2_t u8g2;
    u8g2_init(&u8g2);

    while (keep_running)
    {
        // --- 获取数据 ---
        cpu_temp = get_cpu_temp();
        get_ip_address(NETWORK_IF, ip_buf);

        u8g2_FirstPage(&u8g2);
        do
        {
            draw_system_status(&u8g2, cpu_temp, ip_buf);
        } while (u8g2_NextPage(&u8g2));
    }

    //退出前的清理工作
    u8g2_deinit(&u8g2);
    printf("\nOLED Cleared.\n");
    return 0;
}

// 信号处理函数
void handle_signal(int signo)
{
    if (signo == SIGINT)
    {
        keep_running = 0; // 将标志位置 0，通知主循环停止
    }
}

void draw_system_status(u8g2_t *u8g2, float cpu_temp, const char *ip_addr)
{
    char temp_buf[32] = {0};

    // u8g2_ClearBuffer(u8g2); // 使用全屏缓冲模式写法

    // 1. 顶部栏：标题
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawBox(u8g2, 0, 0, 128, 14); // 黑色背景条
    u8g2_SetDrawColor(u8g2, 0);        // 反色显示文字
    u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
    u8g2_DrawStr(u8g2, 4, 10, "RK3566 Monitor");

    // 2. 中部：CPU 温度 (大号字体)
    u8g2_SetDrawColor(u8g2, 1);               // 恢复正常颜色
    u8g2_SetFont(u8g2, u8g2_font_helvB18_tr); // 使用 Helvetica 粗体
    sprintf(temp_buf, "%.1f ", cpu_temp);

    // 居中计算 (屏幕宽128)
    int str_width = u8g2_GetStrWidth(u8g2, temp_buf);
    u8g2_DrawStr(u8g2, (128 - str_width) / 2, 40, temp_buf);

    // 3. 底部：IP 地址 (小号字体)
    u8g2_SetFont(u8g2, u8g2_font_profont12_tr);
    u8g2_DrawStr(u8g2, 0, 60, "IP:");
    u8g2_DrawStr(u8g2, 25, 60, ip_addr);

    // 4. 发送数据
    // u8g2_SendBuffer(u8g2);
}

/**
 * @brief 读取 CPU 温度
 * @return 温度值 (float)，读取失败返回 0.0
 */
float get_cpu_temp()
{
    FILE *fp;
    char temp_str[10];
    float temp = 0.0;

    fp = fopen(TEMP_PATH, "r");
    if (fp == NULL)
    {
        perror("Failed to open thermal zone");
        return 0.0;
    }

    if (fgets(temp_str, sizeof(temp_str), fp) != NULL)
    {
        // Linux 返回的是毫摄氏度 (例如 45123 代表 45.123度)
        temp = atof(temp_str) / 1000.0;
    }

    fclose(fp);
    return temp;
}

/**
 * @brief 获取本机 IP 地址
 * @param iface_name 网卡名称 (如 "eth0", "wlan0")
 * @param ip_buffer 用于存储结果的字符数组
 */
void get_ip_address(const char *iface_name, char *ip_buffer)
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        strcpy(ip_buffer, "Socket Err");
        return;
    }

    // 设置要查询的网卡名称
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, iface_name, IFNAMSIZ - 1);

    // 调用 ioctl 获取 IP
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0)
    {
        // 如果网卡未连接或未分配IP
        strcpy(ip_buffer, "No IP");
    }
    else
    {
        // 转换二进制 IP 为字符串
        strcpy(ip_buffer, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    }

    close(fd);
}