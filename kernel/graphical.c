#include "graphical.h"
#include "memory.h"
#include "printk.h"

void init_graphical(void) {
    int *addr = (int *)0xffff800000a00000;
    int i, count;

    Pos.XResolution = 1440;
    Pos.YResolution = 900;

    Pos.XPosition = 0;
    Pos.YPosition = 0;

    Pos.XCharSize = 8;
    Pos.YCharSize = 16;

    Pos.FB_addr = (int *)0xffff800000a00000;
    Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4 + PAGE_4K_SIZE - 1) &
                    PAGE_4K_MASK;

    for (count = 0; count < 3; count++) {
        /** 测试色条 */
        for (i = 0; i < 1440 * 20; i++) {
            *((char *)addr + 0) = (char)0x00;
            *((char *)addr + 1) = (char)0x00;
            *((char *)addr + 2) = (char)0xff;
            *((char *)addr + 3) = (char)0x00;
            addr += 1;
        }
        for (i = 0; i < 1440 * 20; i++) {
            *((char *)addr + 0) = (char)0x00;
            *((char *)addr + 1) = (char)0xff;
            *((char *)addr + 2) = (char)0x00;
            *((char *)addr + 3) = (char)0x00;
            addr += 1;
        }
        for (i = 0; i < 1440 * 20; i++) {
            *((char *)addr + 0) = (char)0xff;
            *((char *)addr + 1) = (char)0x00;
            *((char *)addr + 2) = (char)0x00;
            *((char *)addr + 3) = (char)0x00;
            addr += 1;
        }
        for (i = 0; i < 1440 * 20; i++) {
            *((char *)addr + 0) = (char)0xff;
            *((char *)addr + 1) = (char)0xff;
            *((char *)addr + 2) = (char)0xff;
            *((char *)addr + 3) = (char)0x00;
            addr += 1;
        }
    }
}