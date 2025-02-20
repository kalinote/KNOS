#include "gdt.h"
#include "printk.h"
#include "gate.h"

void setup_tss64(void) {
    logk("Start setup TSS64...\n");

    volatile uint64_t *gdt = (volatile uint64_t*)GDT_Table;
    
    uint64_t base = (uint64_t)TSS64_Table;
    printk("base: %#016llx\n", base);          // 这里打印有点问题，需要进一步检查
    uint32_t limit = 104 - 1;       // TSS 104字节

    uint16_t base_low    = (base >> 0)  & 0xFFFF;  // 0x8440
    uint8_t  base_mid    = (base >> 16) & 0xFF;    // 0x00（正确值，不是0x10!）
    uint8_t  base_high   = (base >> 24) & 0xFF;    // 0x00
    uint32_t base_upper  = (base >> 32);           // 0xffff8000

    // 构造低64位描述符 (对应GDT条目8)
    uint64_t low = 
        (limit & 0xFFFF) |                  // Limit Low @bit 0-15
        (base_low << 16) |                  // Base Low @bit 16-31
        ((uint64_t)base_mid << 32) |        // Base Mid @bit 32-39
        (0x9ULL << 40) |                    // Type=0x9 @bit 40-43
        (0ULL << 44) |                      // S=0 @bit 44
        (1ULL << 47) |                      // Present=1 @bit47
        (1ULL << 55) |                      // Granularity=1 @bit55
        ((uint64_t)base_high << 56);        // Base High @bit56-63
    // 构造高64位描述符 (对应GDT条目9)
    uint64_t high = base_upper;

    printk("[TSS] base address:\n");
    printk("BaseLow:  0x%04x\n", base_low);
    printk("BaseMid:  0x%02x\n", base_mid);
    printk("BaseHigh: 0x%02x\n", base_high);
    printk("BaseUpper32: 0x%08x\n", base_upper);

    // 写入内存
    __asm__ volatile("cli");        // 关闭中断保证原子性
    gdt[8] = low;
    gdt[9] = high;
    __asm__ volatile("mfence" ::: "memory"); // 内存屏障
    __asm__ volatile("sti");

    printk("gdt8: 0x%016llx\n", GDT_Table[8]);
    printk("gdt9: 0x%016llx\n", GDT_Table[9]);

    // 加载TR寄存器（选择子0x40）
    load_TR(0x40);

    // 初始化(暂时不进行初始化)
    // struct TSS64 *tss = (struct TSS64*)TSS64_Table;
    // tss->rsp[0] = 0xffff800000007E00;  // 设置内核栈地址
    // tss->iomap_base = sizeof(struct TSS64);  // 禁用I/O位图

    logk("TSS64 setup done!\n");
}
