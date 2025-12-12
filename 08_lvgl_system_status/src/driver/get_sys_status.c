#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>

// 1. 获取 CPU 温度 (单位: 摄氏度)
float get_cpu_temp(void)
{
    // RK3566 的温度节点通常在这里
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp == NULL)
        return 0.0;

    char buf[16];
    if (fgets(buf, sizeof(buf), fp))
    {
        fclose(fp);
        return atoi(buf) / 1000.0; // 原始数据是毫摄氏度
    }
    fclose(fp);
    return 0.0;
}

// 2. 获取内存使用率 (单位: %)
int get_mem_usage(void)
{
    struct sysinfo info;
    if (sysinfo(&info) != 0)
        return 0;

    long total_ram = info.totalram;
    long free_ram = info.freeram; // 注意：Linux下 free_ram 不包含 buffer/cache，这里仅做简单演示
    // 更准确的做法是读取 /proc/meminfo 计算 MemAvailable
    // 但 sysinfo 对于嵌入式简单监控足够了

    long used_ram = total_ram - free_ram;
    return (int)((used_ram * 100) / total_ram);
}

// 3. 获取本机 IP 地址 (优先获取 wlan0，其次 eth0)
void get_ip_address(char *buffer, size_t len)
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        snprintf(buffer, len, "No Socket");
        return;
    }

    // 尝试获取 wlan0
    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0)
    {
        strncpy(buffer, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), len);
        close(fd);
        return;
    }

    // 尝试获取 eth0
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0)
    {
        strncpy(buffer, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), len);
        close(fd);
        return;
    }

    strncpy(ifr.ifr_name, "usb0", IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0)
    {
        strncpy(buffer, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), len);
        close(fd);
        return;
    }

    snprintf(buffer, len, "No Network");
    close(fd);
}