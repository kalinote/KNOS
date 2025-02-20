#include "vbe.h"
#include "printk.h"

/**
 * 初始化VBE信息
 */
void init_vbe_info(void) {
    PrintkPos.XResolution = vbe_info->XResolution;
    PrintkPos.YResolution = vbe_info->YResolution;

    PrintkPos.XPosition = 0;
    PrintkPos.YPosition = 0;

    PrintkPos.XCharSize = 8;
    PrintkPos.YCharSize = 16;

    // 这里要在初始化内存管理以后再设置，现在还没办法把物理地址转换成虚拟地址
    PrintkPos.FrameBufferAddr = (uint32_t *)0xffff800001a00000;
    PrintkPos.FrameBufferLength = (PrintkPos.XResolution * PrintkPos.YResolution * 4);
    PrintkPos.bpp = vbe_info->BitsPerPixel;
}
