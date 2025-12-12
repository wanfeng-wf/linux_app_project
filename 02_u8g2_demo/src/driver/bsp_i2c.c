#include "bsp_i2c.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <errno.h>

#define I2C_PATH "/dev/i2c-%d"

int fd_i2c2 = -1;

int i2c_init(int bus)
{
    char path[24];
    snprintf(path, sizeof(path), I2C_PATH, bus);

    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open I2C bus"); // 打印具体错误原因
        return -1;
    }

    return fd;
}

void i2c_deinit(int fd)
{
    if (fd >= 0)
    {
        close(fd);
    }
}

int i2c_write(int fd, uint16_t slave_addr, const uint8_t *buf, uint16_t len)
{
    struct i2c_msg msg;
    msg.addr = slave_addr;
    msg.flags = 0; // 写
    msg.len = len;
    msg.buf = (uint8_t *)buf; // 强转去除 const 警告，内核其实不会改它

    struct i2c_rdwr_ioctl_data packets;
    packets.msgs = &msg;
    packets.nmsgs = 1;

    int ret = ioctl(fd, I2C_RDWR, &packets);
    if (ret != 1)
    {
// 只有调试时才打开
#ifdef DEBUG
        perror("I2C Write Failed");
#endif
        return -1;
    }
    return 0;
}

int i2c_read(int fd, uint16_t slave_addr, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msg;
    msg.addr = slave_addr;
    msg.flags = I2C_M_RD; // 读
    msg.len = len;
    msg.buf = buf;

    struct i2c_rdwr_ioctl_data packets;
    packets.msgs = &msg;
    packets.nmsgs = 1;

    int ret = ioctl(fd, I2C_RDWR, &packets);
    if (ret != 1)
    {
#ifdef DEBUG
        perror("I2C Read Failed");
#endif
        return -1;
    }

    return 0;
}

int i2c_read_reg(int fd, uint16_t slave_addr, uint8_t reg_addr, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];

    // Msg 0: 写寄存器地址
    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg_addr;

    // Msg 1: 读数据
    msgs[1].addr = slave_addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;

    struct i2c_rdwr_ioctl_data packets;
    packets.msgs = msgs;
    packets.nmsgs = 2; // 这里一定要是 2

    int ret = ioctl(fd, I2C_RDWR, &packets);
    if (ret != 2) // 期望成功处理 2 条消息
    {
#ifdef DEBUG
        perror("I2C Read Reg Failed");
#endif
        return -1;
    }
    return 0;
}