#ifndef _I2C_LIB_H
#define _I2C_LIB_H

#include <stdint.h>

extern int fd_i2c2;

/**
* @brief 初始化 I2C 总线
* @param bus I2C 总线号，例如 2 对应 /dev/i2c-2
* @return int 成功返回文件描述符fd，失败返回 -1
*/
int i2c_init(int bus);

/**
* @brief 释放 I2C 总线
* @param fd I2C 总线文件描述符
*/
void i2c_deinit(int fd);

/**
* @brief 向 I2C 设备写入数据
* @param fd I2C 总线文件描述符
* @param slave_addr 从机地址
* @param buf 数据缓冲区
* @param len 数据长度
* @return int 成功返回 0，失败返回 -1
*/
int i2c_write(int fd, uint16_t slave_addr, const uint8_t *buf, uint16_t len);

/**
* @brief 从 I2C 设备读取数据
* @param fd I2C 总线文件描述符
* @param slave_addr 从机地址
* @param buf 数据缓冲区
* @param len 数据长度
* @return int 成功返回 0，失败返回 -1
*/
int i2c_read(int fd, uint16_t slave_addr, uint8_t *buf, uint16_t len);

/**
* @brief 从 I2C 设备读取寄存器数据
* @param fd I2C 总线文件描述符
* @param slave_addr 从机地址
* @param reg_addr 寄存器地址
* @param buf 数据缓冲区
* @param len 数据长度
* @return int 成功返回 0，失败返回 -1
*/
int i2c_read_reg(int fd, uint16_t slave_addr, uint8_t reg_addr, uint8_t *buf, uint16_t len);

#endif