#ifndef __PRINTK_H__
#define __PRINTK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include "font.h"

// 打包ARGB色彩值
#define ARGB_PACK(a,r,g,b) \
    ( ((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | \
      ((uint32_t)(g) << 8)  | ((uint32_t)(b)) )

// 从 32 位整数中解包通道值
#define ARGB_A(color32)  ( ((color32) >> 24) & 0xFF )
#define ARGB_R(color32)  ( ((color32) >> 16) & 0xFF )
#define ARGB_G(color32)  ( ((color32) >>  8) & 0xFF )
#define ARGB_B(color32)  (  (color32)        & 0xFF )

#define WHITE ARGB_PACK(0xFF, 0xFF, 0xFF, 0xFF)
#define BLACK ARGB_PACK(0xFF, 0x00, 0x00, 0x00)
#define RED ARGB_PACK(0xFF, 0xFF, 0x00, 0x00)
#define GREEN ARGB_PACK(0xFF, 0x00, 0xFF, 0x00)
#define BLUE ARGB_PACK(0xFF, 0x00, 0xFF, 0x00)
#define YELLOW ARGB_PACK(0xFF, 0xFF, 0xFF, 0x00)
#define CYAN ARGB_PACK(0xFF, 0xFF, 0xFF, 0xFF)
#define MAGENTA ARGB_PACK(0xFF, 0xFF, 0x00, 0xFF)
#define GRAY ARGB_PACK(0xFF, 0x80, 0x80, 0x80)


#define ZEROPAD	    0x1		    /* 补零 */
#define SIGN	    0x2		    /* 有符号数 */
#define PLUS	    0x4		    /* 显示正负号 */
#define SPACE	    0x8		    /* 正数前显示空格 */
#define LEFT	    0x10		/* 左对齐 */
#define SPECIAL	    0x20		/* 十六进制前导0x */
#define SMALL	    0x40		/* 十六进制使用小写字母 */
#define NEGATIVE    0x80        /* 负数 */

#define is_digit(c)	((c) >= '0' && (c) <= '9')

char printk_buf[4096]={0};      /* 内核打印缓冲区 */

extern unsigned char font_ascii[256][16];

void put_color_char(uint32_t char_color, uint32_t bg_color, uint8_t font);
int32_t color_printk(uint32_t char_color, uint32_t bg_color, const char *fmt, ...);
int32_t printk(const char *fmt, ...);

int32_t logk(const char *fmt, ...);
int32_t warnk(const char *fmt, ...);
int32_t errk(const char *fmt, ...);
int32_t fatalk(const char *fmt, ...);

/**
 * 内核打印信息结构
 */
struct position {
    int32_t XResolution;       // X分辨率
    int32_t YResolution;       // Y分辨率

    int32_t XPosition;         // 下一个字符X坐标（像素）
    int32_t YPosition;         // 下一个字符Y坐标（像素）

    int32_t XCharSize;        // 字符X尺寸（像素/字符）
    int32_t YCharSize;        // 字符Y尺寸（像素/字符）

    uint32_t *FrameBufferAddr;          // 帧缓冲区线性地址
    uint64_t FrameBufferLength;         // 帧缓冲区长度
    uint8_t bpp;                // 每个像素占多少位
} PrintkPos;


#ifdef __cplusplus
}
#endif

#endif
