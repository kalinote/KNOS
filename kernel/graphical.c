#include "graphical.h"
#include "memory.h"
#include "printk.h"

void frame_buffer_init() {
    ////re init frame buffer;
    unsigned long i;
    unsigned long *tmp;
    unsigned long *tmp1;
    unsigned int *FB_addr = (unsigned int *)Phy_To_Virt(0xe0000000);

    Global_CR3 = Get_gdt();

    tmp =
        Phy_To_Virt((unsigned long *)((unsigned long)Global_CR3 & (~0xfffUL)) +
                    (((unsigned long)FB_addr >> PAGE_GDT_SHIFT) & 0x1ff));
    if (*tmp == 0) {
        unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
        set_mpl4t(tmp, mk_mpl4t(Virt_To_Phy(virtual), PAGE_KERNEL_GDT));
    }

    tmp = Phy_To_Virt((unsigned long *)(*tmp & (~0xfffUL)) +
                      (((unsigned long)FB_addr >> PAGE_1G_SHIFT) & 0x1ff));
    if (*tmp == 0) {
        unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
        set_pdpt(tmp, mk_pdpt(Virt_To_Phy(virtual), PAGE_KERNEL_Dir));
    }

    for (i = 0; i < Pos.FB_length; i += PAGE_2M_SIZE) {
        tmp1 = Phy_To_Virt(
            (unsigned long *)(*tmp & (~0xfffUL)) +
            (((unsigned long)((unsigned long)FB_addr + i) >> PAGE_2M_SHIFT) &
             0x1ff));

        unsigned long phy = 0xe0000000 + i;
        set_pdt(tmp1, mk_pdt(phy, PAGE_KERNEL_Page | PAGE_PWT | PAGE_PCD));
    }

    Pos.FB_addr = (unsigned int *)Phy_To_Virt(0xe0000000);

    flush_tlb();
}

void init_graphical(void) {
    int *addr = (int *)0xffff800003000000;
    int i, count;
    struct Page *page = NULL;
    void *tmp = NULL;
    struct Slab *slab = NULL;

    Pos.XResolution = 1440;
    Pos.YResolution = 900;

    Pos.XPosition = 0;
    Pos.YPosition = 0;

    Pos.XCharSize = 8;
    Pos.YCharSize = 16;

    Pos.FB_addr = (int *)0xffff800003000000;
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
