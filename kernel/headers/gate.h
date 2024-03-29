#ifndef __GATE_H__
#define __GATE_H__

struct desc_struct {
    unsigned char x[8];
};

struct gate_struct {
    unsigned char x[16];
};

extern struct desc_struct GDT_Table[];
extern struct gate_struct IDT_Table[];
extern unsigned int TSS64_Table[26];

/*

*/

/**
 * @brief 设置门描述符的宏
 *
 * 此宏用于设置门描述符，包括选择子地址、属性、中断栈表指针、代码地址等信息。
 * 中断程序或异常处理程序会通过这个宏设置门描述符。
 *
 * @param gate_selector_addr 选择子地址
 * @param attr 属性
 * @param ist 中断栈表指针
 * @param code_addr 代码地址
 */
#define _set_gate(gate_selector_addr, attr, ist, code_addr)                    \
    do {                                                                       \
        unsigned long __d0, __d1;                                              \
        __asm__ __volatile__(                                                  \
            "movw	%%dx,	%%ax	\n\t"                                             \
            "andq	$0x7,	%%rcx	\n\t"                                            \
            "addq	%4,	%%rcx	\n\t"                                              \
            "shlq	$32,	%%rcx	\n\t"                                             \
            "addq	%%rcx,	%%rax	\n\t"                                           \
            "xorq	%%rcx,	%%rcx	\n\t"                                           \
            "movl	%%edx,	%%ecx	\n\t"                                           \
            "shrq	$16,	%%rcx	\n\t"                                             \
            "shlq	$48,	%%rcx	\n\t"                                             \
            "addq	%%rcx,	%%rax	\n\t"                                           \
            "movq	%%rax,	%0	\n\t"                                              \
            "shrq	$32,	%%rdx	\n\t"                                             \
            "movq	%%rdx,	%1	\n\t"                                              \
            : "=m"(*((unsigned long *)(gate_selector_addr))),                  \
              "=m"(*(1 + (unsigned long *)(gate_selector_addr))), "=&a"(__d0), \
              "=&d"(__d1)                                                      \
            : "i"(attr << 8), "3"((unsigned long *)(code_addr)),               \
              "2"(0x8 << 16), "c"(ist)                                         \
            : "memory");                                                       \
    } while (0)

/**
 * 将任务寄存器（Task Register）加载为指定值。
 *
 * @param n 指定的值
 */
#define load_TR(n)                                                             \
    do {                                                                       \
        __asm__ __volatile__("ltr	%%ax" : : "a"(n << 3) : "memory");     \
    } while (0)

/*

*/

static inline void set_intr_gate(unsigned int n, unsigned char ist,
                                 void *addr) {
    _set_gate(IDT_Table + n, 0x8E, ist, addr); // P,DPL=0,TYPE=E
}

/*

*/

static inline void set_trap_gate(unsigned int n, unsigned char ist,
                                 void *addr) {
    _set_gate(IDT_Table + n, 0x8F, ist, addr); // P,DPL=0,TYPE=F
}

/*

*/

static inline void set_system_gate(unsigned int n, unsigned char ist,
                                   void *addr) {
    _set_gate(IDT_Table + n, 0xEF, ist, addr); // P,DPL=3,TYPE=F
}

/*

*/

static inline void set_system_intr_gate(unsigned int n, unsigned char ist,
                                        void *addr) // int3
{
    _set_gate(IDT_Table + n, 0xEE, ist, addr); // P,DPL=3,TYPE=E
}

/**
 * @brief 设置TSS64表的值
 *
 * 该函数通过指针直接修改TSS64表的值，每个参数对应TSS64表的一个特定位置。
 * 
 * @param rsp0 TSS64表的第1个值
 * @param other_rsp 与rsp0相同
 */
static void set_tss64(unsigned long rsp0, unsigned long rsp1,
                      unsigned long rsp2, unsigned long ist1,
                      unsigned long ist2, unsigned long ist3,
                      unsigned long ist4, unsigned long ist5,
                      unsigned long ist6, unsigned long ist7) {
    *(unsigned long *)(TSS64_Table + 1) = rsp0;
    *(unsigned long *)(TSS64_Table + 3) = rsp1;
    *(unsigned long *)(TSS64_Table + 5) = rsp2;

    *(unsigned long *)(TSS64_Table + 9) = ist1;
    *(unsigned long *)(TSS64_Table + 11) = ist2;
    *(unsigned long *)(TSS64_Table + 13) = ist3;
    *(unsigned long *)(TSS64_Table + 15) = ist4;
    *(unsigned long *)(TSS64_Table + 17) = ist5;
    *(unsigned long *)(TSS64_Table + 19) = ist6;
    *(unsigned long *)(TSS64_Table + 21) = ist7;
}

#endif
