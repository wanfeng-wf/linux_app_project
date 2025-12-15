#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <errno.h>
#include "bsp_spi.h"

#define SPI_PATH "/dev/spidev%d.%d"

spi_dev_t *dev3_0;

spi_dev_t *spi_init(int bus, int cs, uint8_t mode, uint32_t speed, uint8_t bits)
{
    spi_dev_t *dev = (spi_dev_t *)malloc(sizeof(spi_dev_t));
    if (!dev)
    {
        perror("spi_init: Malloc failed");
        return NULL;
    }

    // 1. 打开 SPI 设备
    char device[32];
    snprintf(device, sizeof(device), SPI_PATH, bus, cs);

    dev->fd = open(device, O_RDWR);
    if (dev->fd < 0)
    {
        fprintf(stderr, "spi_init: Cannot open %s: %s\n", device, strerror(errno));
        free(dev);
        return NULL;
    }

    // 2. 设置并回读参数 (Verify configuration)

    // --- Mode ---
    uint8_t temp_mode = mode;
    if (ioctl(dev->fd, SPI_IOC_WR_MODE, &temp_mode) < 0)
    {
        perror("spi_init: Set SPI mode failed");
        goto error;
    }
    // 回读内核实际接受的 Mode
    if (ioctl(dev->fd, SPI_IOC_RD_MODE, &dev->mode) < 0)
    {
        perror("spi_init: Read SPI mode failed");
        goto error;
    }

    // --- Bits per word ---
    uint8_t temp_bits = bits;
    if (ioctl(dev->fd, SPI_IOC_WR_BITS_PER_WORD, &temp_bits) < 0)
    {
        perror("spi_init: Set bits per word failed");
        goto error;
    }
    if (ioctl(dev->fd, SPI_IOC_RD_BITS_PER_WORD, &dev->bits) < 0)
    {
        perror("spi_init: Read bits per word failed");
        goto error;
    }

    // --- Speed ---
    uint32_t temp_speed = speed;
    if (ioctl(dev->fd, SPI_IOC_WR_MAX_SPEED_HZ, &temp_speed) < 0)
    {
        perror("spi_init: Set max speed failed");
        goto error;
    }
    if (ioctl(dev->fd, SPI_IOC_RD_MAX_SPEED_HZ, &dev->speed) < 0)
    {
        perror("spi_init: Read max speed failed");
        goto error;
    }

    // 初始化默认延时
    dev->delay = 0;

    // 打印实际配置以供调试
    // printf("SPI Initialized: Mode=%d, Bits=%d, Speed=%d Hz\n", dev->mode, dev->bits, dev->speed);

    return dev;

error:
    close(dev->fd);
    free(dev);
    return NULL;
}

void spi_deinit(spi_dev_t *dev)
{
    if (dev)
    {
        if (dev->fd >= 0)
        {
            close(dev->fd);
        }
        free(dev);
    }
}

int spi_transfer(spi_dev_t *dev, const uint8_t *tx, uint8_t *rx, size_t len)
{
    if (!dev || dev->fd < 0)
    {
        fprintf(stderr, "spi_transfer: Invalid device handle\n");
        return -1;
    }

    if (len == 0)
        return 0;

    struct spi_ioc_transfer tr;

    // 显式清零，防止 padding 区域有垃圾数据
    memset(&tr, 0, sizeof(tr));

    // 强制转换指针，去除 const 警告 (spidev 驱动只会读 tx_buf)
    tr.tx_buf = (unsigned long)((void *)tx);
    tr.rx_buf = (unsigned long)rx;
    tr.len = (__u32)len; // 确保类型匹配
    tr.speed_hz = dev->speed;
    tr.delay_usecs = dev->delay;
    tr.bits_per_word = dev->bits;
    tr.cs_change = 0; // 默认传输结束后释放 CS (若为1则保持)

    // 执行 ioctl
    int ret = ioctl(dev->fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0)
    {
        // 这里可以处理特定的错误，比如 EMSGSIZE (消息太长)
        if (errno == EMSGSIZE)
        {
            fprintf(stderr, "spi_transfer: Data too long (limit is usually 4096 bytes)\n");
        }
        else
        {
            perror("spi_transfer: ioctl failed");
        }
        return -1;
    }

    return 0;
}

int spi_write(spi_dev_t *dev, const uint8_t *buf, size_t len)
{
    return spi_transfer(dev, buf, NULL, len);
}

int spi_read(spi_dev_t *dev, uint8_t *buf, size_t len)
{
    // 读取时，发送缓冲区传 NULL，驱动会自动发送 0x00
    return spi_transfer(dev, NULL, buf, len);
}