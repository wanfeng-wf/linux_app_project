#include <unistd.h>
#include "oled_port.h"
#include "bsp_i2c.h"

uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    /* u8g2/u8x8 will never send more than 32 bytes between START_TRANSFER and END_TRANSFER */
    static uint8_t buffer[128];
    static uint8_t buf_idx;
    uint8_t *data;

    switch (msg)
    {
    case U8X8_MSG_BYTE_INIT:
    {
        /* add your custom code to init i2c subsystem */
        fd_i2c2 = i2c_init(2);
    }
    break;

    case U8X8_MSG_BYTE_START_TRANSFER:
    {
        buf_idx = 0;
    }
    break;

    case U8X8_MSG_BYTE_SEND:
    {
        data = (uint8_t *)arg_ptr;

        while (arg_int > 0)
        {
            buffer[buf_idx++] = *data;
            data++;
            arg_int--;
        }
    }
    break;

    case U8X8_MSG_BYTE_END_TRANSFER:
    {
        if (i2c_write(fd_i2c2, OLED_ADDRESS >> 1, buffer, buf_idx) != 0)
            return 0;
    }
    break;

    case U8X8_MSG_BYTE_SET_DC:
        break;

    default:
        return 0;
    }

    return 1;
}

uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg)
    {
    case U8X8_MSG_DELAY_100NANO: // delay arg_int * 100 nano seconds
        /**
         * Linux 用户空间无法实现纳秒级精准延时。
         * 但对于 I2C 初始化复位来说，延时多一点没关系。
         * 这里可以留空（因为 CPU 执行指令本身就有消耗），或者给一个最小的 usleep。
         */
        break;

    case U8X8_MSG_DELAY_10MICRO: // delay arg_int * 10 micro seconds
        /**
         * 使用 for 循环 + NOP 在 Linux 上是不准的。
         * 直接用 usleep 替代。arg_int 是倍数，单位是 10us。
         */
        usleep(10 * arg_int);
        break;

    case U8X8_MSG_DELAY_MILLI: // delay arg_int * 1 milli second
        usleep(1000 * arg_int);
        break;

    case U8X8_MSG_DELAY_I2C: // arg_int is the I2C speed in 100KHz, e.g. 4 = 400 KHz
        // Linux 硬件 I2C 驱动会自动处理时序，这里通常不需要手动延时
        // 如果需要，可以保留 usleep(5)
        usleep(5);
        break;

    case U8X8_MSG_GPIO_I2C_CLOCK:
        break;
    case U8X8_MSG_GPIO_I2C_DATA:
        break;
    case U8X8_MSG_GPIO_MENU_SELECT:
        u8x8_SetGPIOResult(u8x8, /* get menu select pin state */ 0);
        break;
    case U8X8_MSG_GPIO_MENU_NEXT:
        u8x8_SetGPIOResult(u8x8, /* get menu next pin state */ 0);
        break;
    case U8X8_MSG_GPIO_MENU_PREV:
        u8x8_SetGPIOResult(u8x8, /* get menu prev pin state */ 0);
        break;
    case U8X8_MSG_GPIO_MENU_HOME:
        u8x8_SetGPIOResult(u8x8, /* get menu home pin state */ 0);
        break;
    default:
        u8x8_SetGPIOResult(u8x8, 1); // default return value
        break;
    }
    return 1;
}

void u8g2_init(u8g2_t *u8g2)
{
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2, U8G2_R0, u8x8_byte_hw_i2c, u8x8_gpio_and_delay);
    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
    u8g2_ClearBuffer(u8g2);
}

void u8g2_deinit(u8g2_t *u8g2)
{
    u8g2_ClearBuffer(u8g2);     // 清空显存缓冲区
    u8g2_SendBuffer(u8g2);      // 发送缓冲区，屏幕变黑
    u8g2_SetPowerSave(u8g2, 1); // 进入省电模式 (关闭屏幕电压，虽然OLED没有背光，但这能彻底断开显示)

    i2c_deinit(fd_i2c2);
}
