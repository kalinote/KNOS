#include "gate.h"
#include "lib.h"
#include "memory.h"
#include "printk.h"
#include "task.h"
#include "trap.h"

struct Global_Memory_Descriptor memory_management_struct = {{0}, 0};

void Start_Kernel(void) {
    int *addr = (int *)0xffff800000a00000;
    int i;

    Pos.XResolution = 1440;
    Pos.YResolution = 900;

    Pos.XPosition = 0;
    Pos.YPosition = 0;

    Pos.XCharSize = 8;
    Pos.YCharSize = 16;

    Pos.FB_addr = (int *)0xffff800000a00000;
    Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4 + PAGE_4K_SIZE - 1) &
                    PAGE_4K_MASK;

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

    color_printk(YELLOW, BLACK, "Hello, KNOS!\n");

    load_TR(10);

    set_tss64(_stack_start, _stack_start, _stack_start, 0xffff800000007c00,
              0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
              0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

    sys_vector_init();

    memory_management_struct.start_code = (unsigned long)&_text;
    memory_management_struct.end_code = (unsigned long)&_etext;
    memory_management_struct.end_data = (unsigned long)&_edata;
    memory_management_struct.end_brk = (unsigned long)&_end;

    color_printk(RED, BLACK, "memory init \n");
    init_memory();

    color_printk(RED, BLACK, "interrupt init \n");
    init_interrupt();

    color_printk(RED, BLACK, "task_init \n");
    task_init();

    while (1)
        ;
}