#ifndef _oled_H
#define _oled_H

#include "u8g2.h"

#define MAX_LEN 128
#define OLED_ADDRESS 0x78
#define OLED_CMD 0x00
#define OLED_DATA 0x40

uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
void u8g2_init(u8g2_t *u8g2);
void u8g2_deinit(u8g2_t *u8g2);

#endif /*_oled_H */
