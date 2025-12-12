#include "tft_st7735s.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gpiod.h>
#include "bsp_spi.h"

// 定义 SPI 单次传输的最大块大小 (通常 spidev 限制为 4096)
#define SPI_CHUNK_SIZE 4096
// 16位数据高低位互换
#define SWAP_UINT16(x) (((x) >> 8) | ((x) << 8))

// 全局 GPIO 句柄定义
struct gpiod_line *line_rst = NULL;
struct gpiod_line *line_dc = NULL;
struct gpiod_line *line_blk = NULL;

// 延时函数封装
void delay_ms(int ms)
{
    usleep(ms * 1000);
}

// 0:竖屏,1:右转90横屏,2:右转180竖屏,3:右转270横屏
#define TFT_ROTATE 1
// 屏幕显存配置
#if TFT_ROTATE == 0 || TFT_ROTATE == 2
#define TFT_ROW 160
#define TFT_COLUMN 128
uint16_t TFT_GRAM[TFT_ROW][TFT_COLUMN];
#else
#define TFT_ROW 128
#define TFT_COLUMN 160
uint16_t TFT_GRAM[TFT_ROW][TFT_COLUMN];
#endif

// SPI 发送一个字节
static void TFT_SendByte(uint8_t byte)
{
    if (dev3_0)
    {
        spi_write(dev3_0, &byte, 1);
    }
}

/**
 * @brief 使用 libgpiod 初始化 GPIO
 */
static void TFT_GPIO_Init(void)
{
    struct gpiod_chip *chip;

    // 打开 GPIO 控制器
    chip = gpiod_chip_open_by_name(TFT_GPIO_CHIP_NAME);
    if (!chip)
    {
        perror("TFT: Open gpiochip failed");
        return;
    }

    // 获取 RST 引脚
    line_rst = gpiod_chip_get_line(chip, TFT_RST_OFFSET);
    if (!line_rst)
    {
        perror("TFT: Get RST line failed");
    }
    else
    {
        gpiod_line_request_output(line_rst, "tft_rst", 1); // 默认高电平
    }

    // 获取 DC 引脚
    line_dc = gpiod_chip_get_line(chip, TFT_DC_OFFSET);
    if (!line_dc)
    {
        perror("TFT: Get DC line failed");
    }
    else
    {
        gpiod_line_request_output(line_dc, "tft_dc", 1); // 默认高电平(数据)
    }

    // 获取 BLK 引脚 (可选)
#ifdef TFT_BLK_OFFSET
    if (TFT_BLK_OFFSET >= 0)
    {
        line_blk = gpiod_chip_get_line(chip, TFT_BLK_OFFSET);
        if (line_blk)
        {
            gpiod_line_request_output(line_blk, "tft_blk", 1); // 默认开启背光
        }
    }
#endif

    // 注意：在 Linux spidev 中，CS 引脚由 SPI 驱动控制，无需用户态 GPIO 操作
}

static void TFT_SendCmd(uint8_t cmd)
{
    TFT_DC(0); // DC拉低，表示命令
    // CS 自动由驱动拉低
    TFT_SendByte(cmd);
    // CS 自动由驱动拉高
}

static void TFT_SendData(uint8_t data)
{
    TFT_DC(1); // DC拉高，表示数据
    TFT_SendByte(data);
}

// static void TFT_Send16Bit(uint16_t data)
// {
//     uint8_t buf[2];
//     buf[0] = data >> 8;
//     buf[1] = data;

//     TFT_DC(1);
//     // 优化：一次写入 2 字节，减少一次 syscall
//     if (dev3_0)
//         spi_write(dev3_0, buf, 2);
// }

static void TFT_SendArray(uint16_t *array, uint16_t len)
{
    if (len == 0)
        return;

    // 因为 array 中的数据已经是大端序排列的了
    // 我们只需要把 uint16_t* 强转为 uint8_t* 直接发送
    uint8_t *raw_data = (uint8_t *)array;
    uint32_t total_bytes = len * 2;
    uint32_t bytes_sent = 0;

    TFT_DC(1); // 准备发送数据

    while (bytes_sent < total_bytes)
    {
        uint32_t bytes_remain = total_bytes - bytes_sent;
        uint32_t chunk_size = (bytes_remain > SPI_CHUNK_SIZE) ? SPI_CHUNK_SIZE : bytes_remain;

        // 直接发送内存块，没有任何 CPU 运算开销
        if (dev3_0)
        {
            spi_write(dev3_0, raw_data + bytes_sent, chunk_size);
        }

        bytes_sent += chunk_size;
    }
}

/**
 * @brief 初始化ST7735S屏幕
 */
void TFT_Init(void)
{
    // 1. 初始化 SPI (确保 dev3_0 已被外部 main 函数初始化，或者在这里初始化)
    // 建议在 main 中初始化 spi，这里只检查
    if (!dev3_0)
    {
        fprintf(stderr, "TFT_Init Error: SPI device not initialized!\n");
        // 你也可以选择在这里强制初始化:
        // dev3_0 = spi_init(3, 0, 0, 10000000, 8); // 10MHz
    }

    // 2. 初始化 GPIO
    TFT_GPIO_Init();

    // 3. 硬件复位
    TFT_Reset();

    TFT_SendCmd(0x11); // Sleep Out
    delay_ms(120);

    //----------------------基本配置---------------------------------
    TFT_SendCmd(0x36);  // 【数据访问方式】，RGB/BGR,行列读写方向与水平垂直刷新等
    TFT_SendData(0x00); // 注：172行进行了详细原理注释，0x00非最终设置

    TFT_SendCmd(0x3A);  // 【RGB图像数据格式】，后3位设定
    TFT_SendData(0x05); // 3: 12bit;  5: 16 bit； 6: 18bit ； 7: 未使用

    TFT_SendCmd(0xB1); // 帧率控制（正常模式/全彩） 后跟3个数据
    TFT_SendData(0x05);
    TFT_SendData(0x3C);
    TFT_SendData(0x3C);

    TFT_SendCmd(0xB2); // 帧率控制（空闲模式/ 8色） 后跟3个数据
    TFT_SendData(0x05);
    TFT_SendData(0x3C);
    TFT_SendData(0x3C);

    TFT_SendCmd(0xB3);  // 帧率控制（部分模式/全彩）后跟6个数据
    TFT_SendData(0x05); // 1-3个参数 点反转
    TFT_SendData(0x3C);
    TFT_SendData(0x3C);
    TFT_SendData(0x05); // 4-6个参数 列反转
    TFT_SendData(0x3C);
    TFT_SendData(0x3C);

    TFT_SendCmd(0xB4); // 显示反转控制，D0-D3位有效，分别对应不同模式
    TFT_SendData(0x03);

    //------------------电源控制寄存器1-5------------------
    TFT_SendCmd(0xC0); // 特定颜色模式下的电压参数,调整显示屏的亮度、对比度等显示效果
    TFT_SendData(0x2E);
    TFT_SendData(0x06);
    TFT_SendData(0x04);

    TFT_SendCmd(0xC1); // C1-C4功能同0xC1,更精细的电压调整，以达到更好的视觉效果。
    TFT_SendData(0xC0);
    TFT_SendData(0xC2);

    TFT_SendCmd(0xC2); // 略
    TFT_SendData(0x0D);
    TFT_SendData(0x0D);

    TFT_SendCmd(0xC3); // 略
    TFT_SendData(0x8D);
    TFT_SendData(0xEE);

    TFT_SendCmd(0xC4); // 略
    TFT_SendData(0x8D);
    TFT_SendData(0xEE);

    TFT_SendCmd(0xC5);  // 设置显示屏的VCOM 电压，即显示屏公共电极的电压
    TFT_SendData(0x00); // 影响整体显示效果，调整亮度均匀，减少色彩失真

    //---------------------数据显示方式（重要）-----------------
    TFT_SendCmd(0x36);  // 【数据显示方式格式详解】与屏幕方向息息相关，以0xC0 为例，
    TFT_SendData(0xC0); //   MY行顺序    MX列顺序    MV行列转换   ML垂直刷新  RGB/BGR   MH水平刷新   -   -
                        //      1          1            0           0          0           0       0   0
                        //    上至下      左至右        否         关闭         RGB        关闭
                        // 设置方向函数，只需要调整  MY，MX, MV 的值即可

    //-----------------------伽马序列------------------------
    // 出厂已调好，一般无需额外调整
    TFT_SendCmd(0xe0);  // 伽马极性校正设置，后跟16个8位数据
    TFT_SendData(0x1B); // 涉及高,中，低三个等级调整，有效地址：D0-D5位
    TFT_SendData(0x21);
    TFT_SendData(0x10); // 可使屏幕亮度更符合人眼的感知特性，减少亮度失真导致的视觉疲劳
    TFT_SendData(0x15); // 优化色彩准确性,准确地显示出各种颜色
    TFT_SendData(0x2B); // 增强暗部细节,改善在低灰阶显示效果
    TFT_SendData(0x25);
    TFT_SendData(0x1F);
    TFT_SendData(0x23);
    TFT_SendData(0x22);
    TFT_SendData(0x22);
    TFT_SendData(0x2B);
    TFT_SendData(0x37);
    TFT_SendData(0x00);
    TFT_SendData(0x15);
    TFT_SendData(0x02);
    TFT_SendData(0x3F);

    TFT_SendCmd(0xE1); // 同E0，略
    TFT_SendData(0x1A);
    TFT_SendData(0x20);
    TFT_SendData(0x0F);
    TFT_SendData(0x15);
    TFT_SendData(0x2A);
    TFT_SendData(0x25);
    TFT_SendData(0x1E);
    TFT_SendData(0x23);
    TFT_SendData(0x23);
    TFT_SendData(0x22);
    TFT_SendData(0x2B);
    TFT_SendData(0x37);
    TFT_SendData(0x00);
    TFT_SendData(0x15);
    TFT_SendData(0x02);
    TFT_SendData(0x3F);

    //---------------------自定补充操作------------------
    TFT_SendCmd(0x2C); // 【0x2c作用1】初始化设置时，配置显示参数
    // TFT_SendCmd(0x21); //【打开颜色反转】正常使用无需打开
    TFT_SendCmd(0x29); // 【打开屏幕】，0x28为关闭屏幕

    TFT_SpinScreen(TFT_ROTATE);
    TFT_NewFrame(BLACK);
    TFT_ShowFrame();
}

/**
 * @brief 释放 GPIO 资源
 */
void TFT_DeInit(void)
{
    // 1. 软件清屏 (涂黑)
    TFT_NewFrame(BLACK);
    TFT_ShowFrame();

    // 2. 关键：增加一个小延时，确保 SPI 总线上的数据彻底发完
    // ST7735S 接收完一帧数据也需要微秒级的处理时间
    usleep(50 * 1000); // 等待 50ms

    // 3. 发送 ST7735S 的关屏指令
    TFT_SendCmd(0x28); // Display OFF (关闭显示输出，显存保持)
    usleep(20 * 1000); // 必须等待

    TFT_SendCmd(0x10);  // Sleep IN (进入低功耗模式)
    usleep(120 * 1000); // ST7735 手册要求 Sleep In 后需要等待 120ms 才能断电

    // 4. 关闭背光
    TFT_TurnOff(0);

    // 5. 释放 GPIO 资源
    if (line_rst)
    {
        gpiod_line_release(line_rst);
        line_rst = NULL;
    }
    if (line_dc)
    {
        gpiod_line_release(line_dc);
        line_dc = NULL;
    }
    if (line_blk)
    {
        gpiod_line_release(line_blk);
        line_blk = NULL;
    }

    // 注意：如果在 TFT_GPIO_Init 中 open 了 chip 但没 close，
    // 这里最好也能 close chip。但由于之前代码 chip 是局部变量，
    // 只要 line 释放了，进程结束时 OS 会自动回收 chip 的 fd。
    // printf("TFT GPIO Released.\n");
}

void TFT_Reset(void)
{
    TFT_RST(0);
    delay_ms(100);
    TFT_RST(1);
    delay_ms(50);
}

void TFT_TurnOff(uint8_t on)
{
    if (on)
        TFT_BLK(1);
    else
        TFT_BLK(0);
}

void TFT_SpinScreen(uint8_t rotate)
{
    TFT_SendCmd(0x36);
    if (rotate == 0)
        TFT_SendData(0xC0);
    if (rotate == 1)
        TFT_SendData(0xA0);
    if (rotate == 2)
        TFT_SendData(0x00);
    if (rotate == 3)
        TFT_SendData(0x60);
}

void TFT_NewFrame(uint16_t color)
{
    uint8_t i, j;
    for (i = 0; i < TFT_ROW; i++)
        for (j = 0; j < TFT_COLUMN; j++)
            TFT_GRAM[i][j] = color;
}

void TFT_ShowFrame(void)
{
    TFT_SendCmd(0x2A);
    TFT_SendData(0x00);
    TFT_SendData(0x00);
    TFT_SendData(0x00);
    TFT_SendData(TFT_COLUMN - 1);
    TFT_SendCmd(0x2B);
    TFT_SendData(0x00);
    TFT_SendData(0x00);
    TFT_SendData(0x00);
    TFT_SendData(TFT_ROW - 1);
    TFT_SendCmd(0x2C);

    TFT_SendArray((uint16_t *)TFT_GRAM, TFT_ROW * TFT_COLUMN);
}

void TFT_ShowPartialFrame(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    TFT_SendCmd(0x2A);
    TFT_SendData(0x00);
    TFT_SendData(x0);
    TFT_SendData(0x00);
    TFT_SendData(x1);
    TFT_SendCmd(0x2B);
    TFT_SendData(0x00);
    TFT_SendData(y0);
    TFT_SendData(0x00);
    TFT_SendData(y1);
    TFT_SendCmd(0x2C);

    TFT_SendArray((uint16_t *)TFT_GRAM + y0 * TFT_COLUMN + x0, (y1 - y0 + 1) * (x1 - x0 + 1));
}

void TFT_SetPixel(uint8_t x, uint8_t y, uint16_t color)
{
    if (x >= TFT_COLUMN || y >= TFT_ROW)
        return;

    // 存入显存时，直接存为大端序（Big Endian）
    // 这样内存里的字节顺序就变成了 [高字节] [低字节]
    // SPI 发送时直接发出去即可，不需要再运算
    TFT_GRAM[y][x] = SWAP_UINT16(color);
}

/**
 * @brief 供 LVGL 调用的底层接口：设置窗口并发送数据
 * @param x1 起始列索引
 * @param y1 起始行索引
 * @param x2 结束列索引
 * @param y2 结束行索引
 * @param data 数据指针
 * @param len_in_bytes 数据长度（字节数）
 */
void TFT_WriteData(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, void *data, uint32_t len_in_bytes)
{
    // 1. 设置 ST7735 的显示窗口
    TFT_SendCmd(0x2A);
    TFT_SendData(0x00);
    TFT_SendData(x1);
    TFT_SendData(0x00);
    TFT_SendData(x2);

    TFT_SendCmd(0x2B);
    TFT_SendData(0x00);
    TFT_SendData(y1);
    TFT_SendData(0x00);
    TFT_SendData(y2);

    TFT_SendCmd(0x2C); // 开始写内存

    // 2. 发送数据 (复用之前的 SPI 分包发送逻辑)
    TFT_DC(1);

    uint8_t *raw_data = (uint8_t *)data;
    uint32_t bytes_sent = 0;

    while (bytes_sent < len_in_bytes)
    {
        uint32_t bytes_remain = len_in_bytes - bytes_sent;
        uint32_t chunk = (bytes_remain > SPI_CHUNK_SIZE) ? SPI_CHUNK_SIZE : bytes_remain;

        if (dev3_0)
            spi_write(dev3_0, raw_data + bytes_sent, chunk);

        bytes_sent += chunk;
    }
}

// -----------------------------------------------------------
// 以下绘图函数逻辑与硬件无关
// -----------------------------------------------------------

/**
 * @brief 绘制一条直线
 * @param x1 直线起点坐标x1
 * @param y1 直线起点坐标y1
 * @param x2 直线终点坐标x2
 * @param y2 直线终点坐标y2
 * @param color 16位颜色值
 * @retval None
 */
void TFT_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t color)
{
    static uint8_t temp = 0;
    if (x1 == x2)
    {
        if (y1 > y2)
        {
            temp = y1;
            y1   = y2;
            y2   = temp;
        }
        for (uint8_t y = y1; y <= y2; y++)
        {
            TFT_SetPixel(x1, y, color);
        }
    }
    else if (y1 == y2)
    {
        if (x1 > x2)
        {
            temp = x1;
            x1   = x2;
            x2   = temp;
        }
        for (uint8_t x = x1; x <= x2; x++)
        {
            TFT_SetPixel(x, y1, color);
        }
    }
    else
    {
        // Bresenham直线算法
        int16_t dx = x2 - x1;
        int16_t dy = y2 - y1;
        int16_t ux = ((dx > 0) << 1) - 1;
        int16_t uy = ((dy > 0) << 1) - 1;
        int16_t x = x1, y = y1, eps = 0;
        dx = abs(dx);
        dy = abs(dy);
        if (dx > dy)
        {
            for (x = x1; x != x2; x += ux)
            {
                TFT_SetPixel(x, y, color);
                eps += dy;
                if ((eps << 1) >= dx)
                {
                    y += uy;
                    eps -= dx;
                }
            }
        }
        else
        {
            for (y = y1; y != y2; y += uy)
            {
                TFT_SetPixel(x, y, color);
                eps += dx;
                if ((eps << 1) >= dy)
                {
                    x += ux;
                    eps -= dy;
                }
            }
        }
    }
}

/**
 * @brief 绘制一个矩形
 * @param x 矩形左上角坐标x
 * @param y 矩形左上角坐标y
 * @param w 矩形宽度
 * @param h 矩形高度
 * @param color 16位颜色值
 * @retval None
 */
void TFT_DrawRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color)
{
    TFT_DrawLine(x, y, x + w, y, color);
    TFT_DrawLine(x, y + h, x + w, y + h, color);
    TFT_DrawLine(x, y, x, y + h, color);
    TFT_DrawLine(x + w, y, x + w, y + h, color);
}

/**
 * @brief 绘制一个填充矩形
 * @param x 矩形左上角坐标x
 * @param y 矩形左上角坐标y
 * @param w 矩形宽度
 * @param h 矩形高度
 * @param color 16位颜色值
 * @retval None
 */
void TFT_DrawFilledRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color)
{
    for (uint8_t i = 0; i < h; i++)
        TFT_DrawLine(x, y + i, x + w, y + i, color);
}

/**
 * @brief 绘制一个三角形
 * @param x1 三角形顶点坐标x1
 * @param y1 三角形顶点坐标y1
 * @param x2 三角形顶点坐标x2
 * @param y2 三角形顶点坐标y2
 * @param x3 三角形顶点坐标x3
 * @param y3 三角形顶点坐标y3
 * @param color 16位颜色值
 * @retval None
 */
void TFT_DrawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint16_t color)
{
    TFT_DrawLine(x1, y1, x2, y2, color);
    TFT_DrawLine(x2, y2, x3, y3, color);
    TFT_DrawLine(x3, y3, x1, y1, color);
}

// clang-format off
#define SWAP(a, b)  {uint8_t temp = a; a = b; b = temp;} // 交换a和b的值
// clang-format on

/**
 * @brief 绘制一个填充三角形
 * @param x1 三角形顶点坐标x1
 * @param y1 三角形顶点坐标y1
 * @param x2 三角形顶点坐标x2
 * @param y2 三角形顶点坐标y2
 * @param x3 三角形顶点坐标x3
 * @param y3 三角形顶点坐标y3
 * @param color 16位颜色值
 * @retval None
 */
void TFT_DrawFilledTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint16_t color)
{
    // 确保y1 <= y2 <= y3
    if (y1 > y2)
    {
        SWAP(y1, y2);
        SWAP(x1, x2);
    }
    if (y1 > y3)
    {
        SWAP(y1, y3);
        SWAP(x1, x3);
    }
    if (y2 > y3)
    {
        SWAP(y2, y3);
        SWAP(x2, x3);
    }

    uint8_t total_height = y3 - y1;
    for (uint8_t i = 0; i < total_height; i++)
    {
        uint8_t y              = y1 + i;
        uint8_t second_half    = i > (y2 - y1) || y2 == y1;
        uint8_t segment_height = second_half ? (y3 - y2) : (y2 - y1);

        // 避免除零
        if (segment_height <= 0)
            continue;
        if (total_height <= 0)
            continue;

        // 使用浮点计算提高精度
        float x_left, x_right;

        if (second_half)
        {
            // 从y2到y3
            x_left  = x1 + (float)((x3 - x1) * i) / total_height;
            x_right = x2 + (float)((x3 - x2) * (i - (y2 - y1))) / segment_height;
        }
        else
        {
            // 从y1到y2
            x_left  = x1 + (float)((x2 - x1) * i) / total_height;
            x_right = x1 + (float)((x3 - x1) * i) / total_height;
        }

        // 确保x_left <= x_right
        if (x_left > x_right)
        {
            SWAP(x_left, x_right);
        }

        // 确保x坐标在有效范围内
        if (x_left < 0)
            x_left = 0;
        if (x_right >= TFT_COLUMN)
            x_right = TFT_COLUMN - 1;

        // 绘制扫描线
        TFT_DrawLine(x_left, y, x_right, y, color);
    }
}
/**
 * @brief 绘制一个圆
 * @param x 圆中心坐标x
 * @param y 圆中心坐标y
 * @param r 圆半径
 * @param color 16位颜色值
 * @retval None
 */
void TFT_DrawCircle(uint8_t x, uint8_t y, uint8_t r, uint16_t color)
{
    // 使用Bresenham算法绘制圆
    int16_t x0 = r, y0 = 0;
    int16_t d = 3 - 2 * r;
    while (x0 >= y0)
    {
        TFT_SetPixel(x + x0, y + y0, color);
        TFT_SetPixel(x + y0, y + x0, color);
        TFT_SetPixel(x - y0, y + x0, color);
        TFT_SetPixel(x - x0, y + y0, color);
        TFT_SetPixel(x - x0, y - y0, color);
        TFT_SetPixel(x - y0, y - x0, color);
        TFT_SetPixel(x + y0, y - x0, color);
        TFT_SetPixel(x + x0, y - y0, color);
        y0++;
        if (d > 0)
        {
            x0--;
            d = d + 4 * (y0 - x0) + 10;
        }
        else
            d = d + 4 * y0 + 6;
    }
}

/**
 * @brief 绘制一个填充圆
 * @param x 圆中心坐标x
 * @param y 圆中心坐标y
 * @param r 圆半径
 * @param color 16位颜色值
 * @retval None
 */
void TFT_DrawFilledCircle(uint8_t x, uint8_t y, uint8_t r, uint16_t color)
{
    // 使用Bresenham算法绘制填充圆
    int16_t a = 0, b = r, di = 3 - (r << 1);
    while (a <= b)
    {
        for (int16_t i = x - b; i <= x + b; i++)
        {
            TFT_SetPixel(i, y + a, color);
            TFT_SetPixel(i, y - a, color);
        }
        for (int16_t i = x - a; i <= x + a; i++)
        {
            TFT_SetPixel(i, y + b, color);
            TFT_SetPixel(i, y - b, color);
        }
        a++;
        if (di < 0)
        {
            di += 4 * a + 6;
        }
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
    }
}

/**
 * @brief 绘制一个椭圆
 * @param x 椭圆中心坐标x
 * @param y 椭圆中心坐标y
 * @param a 椭圆长轴半径
 * @param b 椭圆短轴半径
 * @param color 16位颜色值
 * @retval None
 */
void TFT_DrawEllipse(uint8_t x, uint8_t y, uint8_t a, uint8_t b, uint16_t color)
{
    int xpos = 0, ypos = b;
    int a2 = a * a, b2 = b * b;
    int d = b2 + a2 * (0.25 - b);
    while (a2 * ypos > b2 * xpos)
    {
        TFT_SetPixel(x + xpos, y + ypos, color);
        TFT_SetPixel(x - xpos, y + ypos, color);
        TFT_SetPixel(x + xpos, y - ypos, color);
        TFT_SetPixel(x - xpos, y - ypos, color);
        if (d < 0)
        {
            d = d + b2 * ((xpos << 1) + 3);
            xpos += 1;
        }
        else
        {
            d = d + b2 * ((xpos << 1) + 3) + a2 * (-(ypos << 1) + 2);
            xpos += 1, ypos -= 1;
        }
    }
    d = b2 * (xpos + 0.5) * (xpos + 0.5) + a2 * (ypos - 1) * (ypos - 1) - a2 * b2;
    while (ypos > 0)
    {
        TFT_SetPixel(x + xpos, y + ypos, color);
        TFT_SetPixel(x - xpos, y + ypos, color);
        TFT_SetPixel(x + xpos, y - ypos, color);
        TFT_SetPixel(x - xpos, y - ypos, color);
        if (d < 0)
        {
            d = d + b2 * ((xpos << 1) + 2) + a2 * (-(ypos << 1) + 3);
            xpos += 1, ypos -= 1;
        }
        else
        {
            d = d + a2 * (-(ypos << 1) + 3);
            ypos -= 1;
        }
    }
}

/**
 * @brief 绘制一个图片
 * @param x 图片左上角坐标x
 * @param y 图片左上角坐标y
 * @param img 图片结构体指针
 * @retval None
 */
void TFT_DrawImage(uint8_t x, uint8_t y, const Image *img)
{
    if (!img || !img->data) // 空指针检查
        return;

    uint8_t picH, picL;
    for (uint16_t i = 0; i < img->w * img->h; i++)
    {
        // 计算像素坐标
        uint8_t pixel_x = x + (i % img->w);
        uint8_t pixel_y = y + (i / img->w);

        // 从data数组获取颜色数据（每像素占用2字节）
        picL = img->data[2 * i];
        picH = img->data[2 * i + 1];

        // 设置像素颜色
        TFT_SetPixel(pixel_x, pixel_y, (picH << 8) | picL); // 图像取模时：低位在前
        // TFT_SetPixel(pixel_x, pixel_y, (picL << 8) | picH);    //图像取模时：高位在前
    }
}

/**
 * @brief 打印一个ASCII字符
 * @param x 字符左上角坐标x
 * @param y 字符左上角坐标y
 * @param ch ASCII字符
 * @param font ASCII字体结构体指针
 * @param color 16位颜色值
 * @param bg_color 16位背景颜色值
 * @retval None
 */
void TFT_PrintASCIIChar(uint8_t x, uint8_t y, char ch, const ASCIIFont *font, uint16_t color, uint16_t bg_color)
{
    if (ch < 0x20 || ch > 0x7E) // 只处理可打印ASCII字符
        return;

    uint8_t bytes_per_row = (font->w + 7) / 8;                     // 计算每行需要的字节数
    uint16_t index        = (ch - 0x20) * font->h * bytes_per_row; // 计算字符在字库中的起始索引

    // 逐行解析
    for (uint8_t row = 0; row < font->h; row++)
    {
        for (uint8_t col = 0; col < font->w; col++)
        {
            uint8_t byte_idx   = col / 8;       // 计算当前像素所在的字节索引
            uint8_t bit_idx    = 7 - (col % 8); // 计算当前像素在字节中的位索引
            uint8_t pixel_byte = font->chars[index + row * bytes_per_row + byte_idx];

            if (pixel_byte & (1 << bit_idx)) // 判断当前像素是否需要点亮
            {
                TFT_SetPixel(x + col, y + row, color);
            }
            else
            {
                TFT_SetPixel(x + col, y + row, bg_color);
            }
        }
    }
}

/**
 * @brief 打印一个ASCII字符串
 * @param x 字符串左上角坐标x
 * @param y 字符串左上角坐标y
 * @param str ASCII字符串
 * @param font ASCII字体结构体指针
 * @param color 16位颜色值
 * @param bg_color 16位背景颜色值
 * @retval None
 */
void TFT_PrintASCIIString(uint8_t x, uint8_t y, char *str, const ASCIIFont *font, uint16_t color, uint16_t bg_color)
{
    uint8_t x0 = x;
    while (*str)
    {
        TFT_PrintASCIIChar(x0, y, *str, font, color, bg_color);
        x0 += font->w;
        str++;
    }
}

/**
 * @brief 在索引表中查找字符
 * @param ch 要查找的字符(UTF-8编码)
 * @param index 索引表结构体指针
 * @retval 找到的字符索引，未找到返回-1
 */
static int16_t _GetIndex(const char *ch, const FontIndex *index)
{
    if (!ch || !index || !index->sample)
    {
        return -1;
    }

    // 线性查找(可根据需要优化为二分查找)
    for (uint16_t i = 0; i < index->len; i += index->code_size)
    {
        if (memcmp(&index->sample[i], ch, index->code_size) == 0)
        {
            return i / index->code_size;
        }
    }

    return -1;
}

/**
 * @brief 绘制一个汉字
 * @param x 汉字左上角x坐标
 * @param y 汉字左上角y坐标
 * @param char_index 汉字在字库中的索引
 * @param font 字体结构体指针
 * @param color 字符颜色
 * @param bg_color 背景颜色
 * @retval None
 */
static void _DrawChineseChar(uint8_t x, uint8_t y, uint16_t char_index, const Font *font, uint16_t color, uint16_t bg_color)
{
    // 每行字节数 = (宽度 + 7) / 8，向上取整
    uint8_t bytes_per_row = (font->w + 7) / 8;

    // 绘制汉字点阵
    for (uint8_t i = 0; i < font->h; i++)
    {
        // 遍历每个字节
        for (uint8_t byte_idx = 0; byte_idx < bytes_per_row; byte_idx++)
        {
            // 计算当前字节在字符数据中的偏移量
            uint16_t data_offset = char_index * font->char_size + i * bytes_per_row + byte_idx;

            // 获取当前字节数据
            uint8_t byte_data = font->chars[data_offset];

            // 遍历当前字节的每一位
            for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++)
            {
                // 计算实际像素列位置
                uint8_t pixel_col = byte_idx * 8 + bit_idx;

                // 根据位值设置像素颜色
                if (byte_data & (0x80 >> bit_idx))
                {
                    TFT_SetPixel(x + pixel_col, y + i, color);
                }
                else
                {
                    TFT_SetPixel(x + pixel_col, y + i, bg_color);
                }
            }
        }
    }
}

/**
 * @brief 打印字符串(UTF-8编码)
 * @param x 字符串左上角x坐标
 * @param y 字符串左上角y坐标
 * @param str 要显示的字符串
 * @param font 字体结构体指针
 * @param color 字符颜色
 * @param bg_color 背景颜色
 * @retval None
 */
void TFT_PrintString(uint8_t x, uint8_t y, char *str, const Font *font, uint16_t color, uint16_t bg_color)
{
    // 参数有效性检查
    if (!str || !font || !font->ascii)
    {
        return;
    }

    uint8_t current_x = x;
    uint8_t current_y = y;
    uint16_t str_len  = strlen(str);

    for (uint16_t i = 0; i < str_len; i++)
    {
        // ASCII字符处理(0x20-0x7E)
        if (str[i] >= 0x20 && str[i] <= 0x7E)
        {
            // 自动换行检查
            if (current_x + font->ascii->w >= TFT_COLUMN)
            {
                current_x = x;               // 回到起始X坐标
                current_y += font->ascii->h; // 下移一行
            }

            // 绘制ASCII字符
            TFT_PrintASCIIChar(current_x, current_y, str[i], font->ascii, color, bg_color);
            current_x += font->ascii->w; // 更新X坐标
        }
        // UTF-8汉字处理(首字节范围：0xE0-0xEF表示3字节汉字)
        else if ((str[i] & 0xE0) == 0xE0 && (i + 2) < str_len)
        {
            // 自动换行检查
            if (current_x + font->w >= TFT_COLUMN)
            {
                current_x = x;
                current_y += font->h;

                if (current_y + font->h >= TFT_ROW)
                {
                    return;
                }
            }

            // 提取UTF-8汉字(3字节)
            char utf8_char[4] = {0};
            utf8_char[0]      = str[i];
            utf8_char[1]      = str[i + 1];
            utf8_char[2]      = str[i + 2];

            // 在索引表中查找该汉字
            int k = _GetIndex(utf8_char, font->index);

            // 字库中没有该汉字时，显示问号
            if (k == -1)
            {
                TFT_PrintASCIIChar(current_x, current_y, '?', font->ascii, color, bg_color);
                current_x += font->ascii->w;
                i += 2; // 跳过汉字的后两个字节
                continue;
            }

            // 输出该汉字
            _DrawChineseChar(current_x, current_y, k, font, color, bg_color);
            current_x += font->w;
            i += 2; // 跳过汉字的后两个字节
        }
        // 其他UTF-8编码或控制字符处理
        else
        {
            // 跳过无法识别的字符
            i++;
        }
    }
}
