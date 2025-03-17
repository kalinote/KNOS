#include "vbe.h"
#include "printk.h"

/**
 * 初始化VBE信息
 */
void init_vbe_info(void) {
    printk_pos.x_resolution = vbe_info->XResolution;
    printk_pos.y_resolution = vbe_info->YResolution;

    printk_pos.x_position = 0;
    printk_pos.y_position = 0;

    printk_pos.x_char_size = 8;
    printk_pos.y_char_size = 16;

    // 这里要在初始化内存管理以后再设置，现在还没办法把物理地址转换成虚拟地址
    printk_pos.frame_buffer_addr = (uint32_t *)0xffff800001a00000;
    printk_pos.frame_buffer_length = (printk_pos.x_resolution * printk_pos.y_resolution * 4);
    printk_pos.bpp = vbe_info->BitsPerPixel;
}
