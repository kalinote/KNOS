#include "cpu.h"
#include "gate.h"
#include "graphical.h"
#include "lib.h"
#include "memory.h"
#include "printk.h"
#include "task.h"
#include "trap.h"

#if APIC
#include "APIC.h"
#else
#include "8259A.h"
#endif

struct Global_Memory_Descriptor memory_management_struct = {{0}, 0};

void Start_Kernel(void) {
    init_graphical();

    color_printk(YELLOW, BLACK, "Hello, KNOS!\n");

    load_TR(10);

    set_tss64(_stack_start, _stack_start, _stack_start, 0xffff800000007c00,
              0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
              0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

    sys_vector_init();

    color_printk(RED, BLACK, "CPU init \n");
    init_cpu();

    memory_management_struct.start_code = (unsigned long)&_text;
    memory_management_struct.end_code = (unsigned long)&_etext;
    memory_management_struct.end_data = (unsigned long)&_edata;
    memory_management_struct.end_brk = (unsigned long)&_end;

    color_printk(RED, BLACK, "memory init \n");
    init_memory();

    color_printk(RED, BLACK, "slab init \n");
    slab_init();

    color_printk(RED, BLACK, "frame buffer init \n");
    frame_buffer_init();
    color_printk(WHITE, BLACK, "frame_buffer_init() is OK \n");

    color_printk(RED, BLACK, "pagetable init \n");
    pagetable_init();

    color_printk(RED, BLACK, "interrupt init \n");
#if APIC
    APIC_IOAPIC_init();
#else
    init_8259A();
#endif

    // color_printk(RED,BLACK,"task_init \n");
    // task_init();

    while (1)
        ;
}
