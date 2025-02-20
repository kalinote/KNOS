#include "idt.h"
#include "lib.h"
#include "trap.h"
#include "printk.h"
#include "gdt.h"

// 全局IDT表
IDTEntry IDT_Table[IDT_ENTRIES] __attribute__((aligned(16)));

void set_gate(uint8_t vector, void *handler, uint16_t selector, uint8_t type_attr, uint8_t ist) {
    uint64_t addr = (uint64_t)handler;
    IDTEntry *entry = &IDT_Table[vector];
    
    entry->offset_low  = addr & 0xFFFF;
    entry->selector    = selector;
    entry->ist         = ist & 0x07;       // 只使用IST的低3位
    entry->type_attr   = type_attr;
    entry->offset_mid  = (addr >> 16) & 0xFFFF;
    entry->offset_high = (addr >> 32) & 0xFFFFFFFF;
    entry->reserved    = 0;
}

void setup_idt(void) {
    logk("Start setup IDT...\n");
    
    // 初始化所有中断门为默认处理函数ignore_int
    for(int i = 0; i < IDT_ENTRIES; i++) {
        set_gate(i, ignore_interrupt, 0x08, GATE_TYPE_INTERRUPT, 0);
    }

    // 设置特定异常处理程序
    set_gate(0, exception_entry_divide_error, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(1, exception_entry_debug, 0x08, GATE_TYPE_TRAP, 0);
    set_gate(2, exception_entry_nmi, 0x08, GATE_TYPE_INTERRUPT, 1);
    set_gate(3, exception_entry_breakpoint, 0x08, GATE_TYPE_TRAP, 0);
    set_gate(4, exception_entry_overflow, 0x08, GATE_TYPE_TRAP, 0);
    set_gate(5, exception_entry_bound_range, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(6, exception_entry_invalid_opcode, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(7, exception_entry_device_not_available, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(8, exception_entry_double_fault, 0x08, GATE_TYPE_INTERRUPT, 2);
    set_gate(10, exception_entry_invalid_tss, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(11, exception_entry_segment_not_present, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(12, exception_entry_stack_segment, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(13, exception_entry_general_protection, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(14, exception_entry_page_fault, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(16, exception_entry_x87_fpu_error, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(17, exception_entry_alignment_check, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(18, exception_entry_machine_check, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(19, exception_entry_simd_exception, 0x08, GATE_TYPE_INTERRUPT, 0);
    set_gate(20, exception_entry_virtualization, 0x08, GATE_TYPE_INTERRUPT, 0);

    // 加载IDTR
    struct __attribute__((packed)) {
        uint16_t limit;
        uint64_t base;
    } idtr = { .limit = sizeof(IDTEntry) * IDT_ENTRIES - 1, .base = (uint64_t)IDT_Table };
    
    __asm__ volatile("lidt %0" : : "m"(idtr));
    logk("IDT setup done!\n");

    // 暂时屏蔽中断,防止时钟触发中断
    // 配置PIC/APIC屏蔽所有中断
    // 此处需要添加PIC初始化代码，例如：
    io_out8(0x21, 0xFF);  // 屏蔽主PIC所有中断
    io_out8(0xA1, 0xFF);  // 屏蔽从PIC所有中断

    // 完成idt配置后记得启用中断
    __asm__ volatile("sti");
}
