#ifndef _SPI_LIB_H
#define _SPI_LIB_H

#include <stdint.h>
#include <stddef.h>

// 定义 SPI 设备句柄结构体
typedef struct
{
    int fd;         // 文件描述符
    uint32_t speed; // 该设备的速率
    uint8_t mode;   // 该设备的模式 (0,1,2,3)
    uint8_t bits;   // 该设备的字长
    uint16_t delay; // 传输延时
} spi_dev_t;

extern spi_dev_t *dev3_0;

/**
 * @brief 初始化 SPI 设备
 * @param bus SPI 总线号，如 3
 * @param cs SPI 从设备选择号，如 0
 * @param mode SPI 模式 (0,1,2,3)
 * @param speed SPI 速率，如 1000000 表示 1MHz
 * @param bits SPI 字长，如 8 表示 8 位
 * @return spi_dev_t* 成功返回 SPI 设备句柄指针，失败返回 NULL
 */
spi_dev_t *spi_init(int bus, int cs, uint8_t mode, uint32_t speed, uint8_t bits);

/**
 * @brief 释放 SPI 设备
 * @param dev SPI 设备句柄指针
 * @return void
 */
void spi_deinit(spi_dev_t *dev);

/**
 * @brief 向 SPI 设备写入数据
 * @param fd 文件描述符
 * @param buf 数据缓冲区指针
 * @param len 数据长度
 * @return int 成功返回 0，失败返回 -1
 */
int spi_write(spi_dev_t *dev, const uint8_t *buf, size_t len);

/**
 * @brief 从 SPI 设备读取数据
 * @param fd 文件描述符
 * @param buf 数据缓冲区指针
 * @param len 数据长度
 * @return int 成功返回 0，失败返回 -1
 */
int spi_read(spi_dev_t *dev, uint8_t *buf, size_t len);

/**
 * @brief 同时向 SPI 设备写入和读取数据
 * @param dev SPI 设备句柄指针
 * @param tx 发送数据缓冲区指针
 * @param rx 接收数据缓冲区指针
 * @param len 数据长度
 * @return int 成功返回 0，失败返回 -1
 */
int spi_transfer(spi_dev_t *dev, const uint8_t *tx, uint8_t *rx, size_t len);

#endif
