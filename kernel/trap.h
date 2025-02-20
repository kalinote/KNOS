#ifndef __TRAP_H__
#define __TRAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "printk.h"

/* 寄存器上下文结构体 */
struct register_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rdx, rcx, rbx, rax;
    uint64_t es, ds;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

/* 异常处理函数通用原型 */
typedef void (*exception_handler_t)(uint64_t error_code, void* frame);

__attribute__((naked)) void ignore_interrupt(void) {
    asm volatile(
        "cld\n\t"
        // 保存所有通用寄存器
        "pushq %%rax\n\t"
        "pushq %%rbx\n\t"
        "pushq %%rcx\n\t"
        "pushq %%rdx\n\t"
        "pushq %%rbp\n\t"
        "pushq %%rdi\n\t"
        "pushq %%rsi\n\t"
        "pushq %%r8\n\t"
        "pushq %%r9\n\t"
        "pushq %%r10\n\t"
        "pushq %%r11\n\t"
        "pushq %%r12\n\t"
        "pushq %%r13\n\t"
        "pushq %%r14\n\t"
        "pushq %%r15\n\t"
        // 保存段寄存器（实际在64位模式下可能不需要）
        "mov %%ds, %%rax\n\t"
        "pushq %%rax\n\t"
        "mov %%es, %%rax\n\t"
        "pushq %%rax\n\t"
        // 加载内核数据段（确保与GDT一致）
        "mov $0x10, %%rax\n\t"
        "mov %%rax, %%ds\n\t"
        "mov %%rax, %%es\n\t"
        // 调用处理函数
        "mov %%rsp, %%rdi\n\t"  // 传递保存的寄存器指针
        "callq ignore_interrupt_handler\n\t" // 非naked函数处理逻辑
        // 恢复环境
        "popq %%rax\n\t"
        "mov %%rax, %%es\n\t"
        "popq %%rax\n\t"
        "mov %%rax, %%ds\n\t"
        "popq %%r15\n\t"
        "popq %%r14\n\t"
        "popq %%r13\n\t"
        "popq %%r12\n\t"
        "popq %%r11\n\t"
        "popq %%r10\n\t"
        "popq %%r9\n\t"
        "popq %%r8\n\t"
        "popq %%rsi\n\t"
        "popq %%rdi\n\t"
        "popq %%rbp\n\t"
        "popq %%rdx\n\t"
        "popq %%rcx\n\t"
        "popq %%rbx\n\t"
        "popq %%rax\n\t"
        "iretq\n\t"
        ::: "memory"
    );
}

void ignore_interrupt_handler(void* frame);

/* 特定异常处理函数 */
void divide_error_handler(uint64_t error_code, void* frame);                    /* 0 - #DE 除零错误 */
void debug_handler(uint64_t error_code, void* frame);                           /* 1 - #DB 调试 */
void nmi_handler(uint64_t error_code, void* frame);                             /* 2 - NMI 不可屏蔽中断 */
void breakpoint_handler(uint64_t error_code, void* frame);                      /* 3 - #BP 断点 */
void overflow_handler(uint64_t error_code, void* frame);                        /* 4 - #OF 溢出 */
void bound_range_exceeded_handler(uint64_t error_code, void* frame);            /* 5 - #BR 边界范围溢出 */
void invalid_opcode_handler(uint64_t error_code, void* frame);                  /* 6 - #UD 无效指令 */
void device_not_available_handler(uint64_t error_code, void* frame);            /* 7 - #NM FPU浮点运算单元不可用 */
void double_fault_handler(uint64_t error_code, void* frame);                    /* 8 - #DF 双重错误 */
void invalid_tss_handler(uint64_t error_code, void* frame);                     /* 10 - #TS 无效TSS */
void segment_not_present_handler(uint64_t error_code, void* frame);             /* 11 - #NP 段不存在 */
void stack_segment_fault_handler(uint64_t error_code, void* frame);             /* 12 - #SS 栈段错误 */
void general_protection_handler(uint64_t error_code, void* frame);              /* 13 - #GP 通用保护错误 */
void page_fault_handler(uint64_t error_code, void* frame);                      /* 14 - #PF 页错误 */
void x87_fp_exception_handler(uint64_t error_code, void* frame);                /* 16 - #MF x87浮点异常 */
void alignment_check_handler(uint64_t error_code, void* frame);                 /* 17 - #AC 对齐检查 */
void machine_check_handler(uint64_t error_code, void* frame);                   /* 18 - #MC 机器检查 */
void simd_fp_exception_handler(uint64_t error_code, void* frame);               /* 19 - #XM/#XF SSE/SIMD浮点异常 */
void virtualization_exception_handler(uint64_t error_code, void* frame);        /* 20 - #VE 虚拟化异常 */
void reserved_exception_handler(uint64_t error_code, void* frame);              /* 保留的异常号触发 */

/* 声明汇编入口点 */
extern void exception_entry_divide_error(void);
extern void exception_entry_debug(void);
extern void exception_entry_nmi(void);
extern void exception_entry_breakpoint(void);
extern void exception_entry_overflow(void);
extern void exception_entry_bound_range(void);
extern void exception_entry_invalid_opcode(void);
extern void exception_entry_device_not_available(void);
extern void exception_entry_double_fault(void);
extern void exception_entry_invalid_tss(void);
extern void exception_entry_segment_not_present(void);
extern void exception_entry_stack_segment(void);
extern void exception_entry_general_protection(void);
extern void exception_entry_page_fault(void);
extern void exception_entry_x87_fpu_error(void);
extern void exception_entry_alignment_check(void);
extern void exception_entry_machine_check(void);
extern void exception_entry_simd_exception(void);
extern void exception_entry_virtualization(void);

#ifdef __cplusplus
}
#endif

#endif