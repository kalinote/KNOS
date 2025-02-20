#include "trap.h"
#include "printk.h"

// 异常处理函数指针数组
static exception_handler_t exception_handlers[32] = {
    [0]  = divide_error_handler,
    [1]  = debug_handler,
    [2]  = nmi_handler,
    [3]  = breakpoint_handler,
    [4]  = overflow_handler,
    [5]  = bound_range_exceeded_handler,
    [6]  = invalid_opcode_handler,
    [7]  = device_not_available_handler,
    [8]  = double_fault_handler,
    [9]  = reserved_exception_handler,       // 保留
    [10] = invalid_tss_handler,
    [11] = segment_not_present_handler,
    [12] = stack_segment_fault_handler,
    [13] = general_protection_handler,
    [14] = page_fault_handler,
    [15] = reserved_exception_handler,       // 保留
    [16] = x87_fp_exception_handler,
    [17] = alignment_check_handler,
    [18] = machine_check_handler,
    [19] = simd_fp_exception_handler,
    [20] = virtualization_exception_handler
};

/* 异常处理程序入口 */
void dispatch_exception(struct register_frame* frame) {
    uint8_t vector = frame->vector;
    if (vector < 32 && exception_handlers[vector]) {
        exception_handlers[vector](frame->error_code, frame);
    } else {
        fatalk("Unhandled exception %d\n", vector);
    }
}

void divide_error_handler(uint64_t error_code, void* frame) {                 /* 0 - #DE */
    struct register_frame* ctx = frame;
    fatalk("#DE(0) Divide error RSP=%#llx, RIP=%#llx, Error Code=%#llx\n", 
           ctx->rsp, ctx->rip, error_code);
    for(;;);
}

void debug_handler(uint64_t error_code, void* frame) {                        /* 1 - #DB */
    struct register_frame* ctx = frame;
    fatalk("#DB(1) Debug RIP=%#llx, Error Code=%#llx\n", 
           ctx->rip, error_code);
    for(;;);
}

void nmi_handler(uint64_t error_code, void* frame) {                          /* 2 - NMI */
    struct register_frame* ctx = frame;
    fatalk("#NMI(2) Non-Maskable Interrupt! RIP=%#llx\n", ctx->rip);
    for(;;);
}

void breakpoint_handler(uint64_t error_code, void* frame) {                   /* 3 - #BP */
    struct register_frame* ctx = frame;
    fatalk("#BP(3) Breakpoint RIP=%#llx\n", ctx->rip);
    for(;;);
}

void overflow_handler(uint64_t error_code, void* frame) {                     /* 4 - #OF */
    struct register_frame* ctx = frame;
    fatalk("#OF(4) Overflow! RIP=%#llx, RSP=%#llx\n", ctx->rip, ctx->rsp);
    for(;;);
}

void bound_range_exceeded_handler(uint64_t error_code, void* frame) {         /* 5 - #BR */
    struct register_frame* ctx = frame;
    fatalk("#BR(5) BOUND Range Exceeded RIP=%#llx\n", ctx->rip);
    for(;;);
}

void invalid_opcode_handler(uint64_t error_code, void* frame) {               /* 6 - #UD */
    struct register_frame* ctx = frame;
    uint16_t opcode = *(uint16_t*)ctx->rip; // 捕获导致异常的指令
    fatalk("#UD(6) Invalid Opcode 0x%04x RIP=%#llx\n", opcode, ctx->rip);
    for(;;);
}

void device_not_available_handler(uint64_t error_code, void* frame) {         /* 7 - #NM */
    struct register_frame* ctx = frame;
    fatalk("#NM(7) FPU Not Available RIP=%#llx\n", ctx->rip);
    for(;;);
}

void double_fault_handler(uint64_t error_code, void* frame) {                 /* 8 - #DF */
    struct register_frame* ctx = frame;
    // 注意：#DF的错误码总是0，但参数仍需保留
    fatalk("#DF(8) Double Fault! RIP=%#llx, RSP=%#llx\n", ctx->rip, ctx->rsp);
    for(;;);
}

void invalid_tss_handler(uint64_t error_code, void* frame) {                  /* 10 - #TS */
    struct register_frame* ctx = frame;
    fatalk("#TS(10) Invalid TSS! Error Code=%#llx RIP=%#llx\n", error_code, ctx->rip);
    for(;;);
}

void segment_not_present_handler(uint64_t error_code, void* frame) {          /* 11 - #NP */
    struct register_frame* ctx = frame;
    fatalk("#NP(11) Segment Not Present! Error Code=%#llx RIP=%#llx\n", error_code, ctx->rip);
    for(;;);
}

void stack_segment_fault_handler(uint64_t error_code, void* frame) {          /* 12 - #SS */
    struct register_frame* ctx = frame;
    fatalk("#SS(12) Stack Segment Fault! Error Code=%#llx RIP=%#llx\n", error_code, ctx->rip);
    for(;;);
}

void general_protection_handler(uint64_t error_code, void* frame) {           /* 13 - #GP */
    struct register_frame* ctx = frame;
    fatalk("#GP(13) General Protection Fault! Error Code=%#llx RIP=%#llx\n", error_code, ctx->rip);
    for(;;);
}

void page_fault_handler(uint64_t error_code, void* frame) {                   /* 14 - #PF */
    // struct register_frame* ctx = frame;
    uint64_t cr2;
    __asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));  // 必须通过汇编获取CR2
    fatalk("#PF(14) Page Fault CR2=%#llx  Error Code=%#llx [%c%c%c]\n",
           cr2, error_code,
           (error_code & 0x01) ? 'P' : '-',  // Present
           (error_code & 0x02) ? 'W' : '-',  // Write
           (error_code & 0x04) ? 'U' : '-'); // User
    for(;;);
}

void x87_fp_exception_handler(uint64_t error_code, void* frame) {             /* 16 - #MF */
    struct register_frame* ctx = frame;
    fatalk("#MF(16) x87 FPU Fault RIP=%#llx, Status=%#llx\n", ctx->rip, error_code);
    for(;;);
}

void alignment_check_handler(uint64_t error_code, void* frame) {              /* 17 - #AC */
    struct register_frame* ctx = frame;
    uint64_t cr2;
    __asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));
    fatalk("#AC(17) Alignment Check RIP=%#llx (Addr=%#llx)\n", ctx->rip, cr2);
    for(;;);
}

void machine_check_handler(uint64_t error_code, void* frame) {                /* 18 - #MC */
    // struct register_frame* ctx = frame;
    // 注意：实际实现需要读取MCG_CAP/MSR寄存器
    fatalk("#MC(18) Machine Check! System Halted\n");
    __asm__ __volatile__("cli; hlt");  // 立即停止执行
}

void simd_fp_exception_handler(uint64_t error_code, void* frame) {            /* 19 - #XM */
    struct register_frame* ctx = frame;
    uint32_t mxcsr;
    __asm__ __volatile__("stmxcsr %0" : "=m"(mxcsr));
    fatalk("#XM(19) SIMD FP Exception RIP=%#llx\n  MXCSR=0x%08x\n", ctx->rip, mxcsr);
    for(;;);
}

void virtualization_exception_handler(uint64_t error_code, void* frame) {      /* 20 - #VE */
    struct register_frame* ctx = frame;
    fatalk("#VE(20) Virtualization Exception RIP=%#llx\n", ctx->rip);
    for(;;);
}

/* 保留的异常号触发 */
void reserved_exception_handler(uint64_t error_code, void* frame) {
    struct register_frame* ctx = frame;
    fatalk("Reserved Exception RIP=%#llx\n", ctx->rip);
    for(;;);
}

// 忽略中断触发
void ignore_interrupt_handler(void* frame) {
    fatalk("Unknown interrupt or fault at RIP\n");
    // 此处可访问frame结构体分析寄存器状态
    for(;;);
}
