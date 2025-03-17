#ifndef __LIB__H__
#define __LIB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define do_div(n, base)                                                                                                \
    ({                                                                                                                 \
        int32_t __res;                                                                                                 \
        __asm__("divq %%rcx" : "=a"(n), "=d"(__res) : "0"(n), "1"(0), "c"(base));                                      \
        __res;                                                                                                         \
    })

static inline void __attribute__((always_inline)) *memcpy(void *dest, void *src, size_t n) {
    int32_t d0, d1, d2;
    __asm__ __volatile__("cld	\n\t"
                         "rep	\n\t"
                         "movsq	\n\t"
                         "testb	$4,%b4	\n\t"
                         "je	1f	\n\t"
                         "movsl	\n\t"
                         "1:\ttestb	$2,%b4	\n\t"
                         "je	2f	\n\t"
                         "movsw	\n\t"
                         "2:\ttestb	$1,%b4	\n\t"
                         "je	3f	\n\t"
                         "movsb	\n\t"
                         "3:	\n\t"
                         : "=&c"(d0), "=&D"(d1), "=&S"(d2)
                         : "0"(n / 8), "q"(n), "1"(dest), "2"(src)
                         : "memory");
    return dest;
}

static inline int32_t strlen(const char *str) {
    register int32_t __res;
    __asm__ __volatile__("cld	\n\t"
                         "repne	\n\t"
                         "scasb	\n\t"
                         "notl	%0	\n\t"
                         "decl	%0	\n\t"
                         : "=c"(__res)
                         : "D"(str), "a"(0), "0"(0xffffffff)
                         :);
    return __res;
}

static inline uint8_t __attribute__((always_inline)) io_in8(uint16_t port) {
	unsigned char ret = 0;
	__asm__ __volatile__(	"inb	%%dx,	%0	\n\t"
				"mfence			\n\t"
				:"=a"(ret)
				:"d"(port)
				:"memory");
	return ret;
}

static inline void __attribute__((always_inline)) io_out8(uint16_t port, uint8_t value) {
	__asm__ __volatile__(	"outb	%0,	%%dx	\n\t"
				"mfence			\n\t"
				:
				:"a"(value),"d"(port)
				:"memory");
}

static inline void __attribute__((always_inline)) *memset(void *dest, int c, size_t n) {
    void *original_dest = dest; // 保存原始指针用于返回
    unsigned char c8 = (unsigned char)c;
    uint64_t c64 = (uint64_t)c8 * 0x0101010101010101ULL; // 将单字节扩展为8字节重复模式

    __asm__ __volatile__ (
        "movq %[c64], %%rax\n\t"     // 将64位填充值加载到RAX
        "mov %[n], %%rcx\n\t"        // 原始长度存入RCX
        "mov %%rcx, %%rdx\n\t"       // 备份长度到RDX
        "shr $3, %%rcx\n\t"         // 计算64位块数量（RCX = n / 8）
        "jz 1f\n\t"                 // 如果块数为0则跳过STOSQ
        "rep stosq\n\t"             // 以64位为单位填充内存
        "1:\n\t"
        "mov %%rdx, %%rcx\n\t"      // 恢复原始长度到RCX
        "and $7, %%rcx\n\t"         // 计算剩余字节数（RCX = n % 8）
        "jz 2f\n\t"                 // 如果无剩余则跳过STOSB
        "rep stosb\n\t"             // 处理剩余字节
        "2:\n\t"
        : "+D" (dest)               // 输出操作数：RDI寄存器（自动更新）
        : [c64] "r" (c64), [n] "r" (n)
        : "rax", "rcx", "rdx", "memory"
    );

    return original_dest;
}


#ifdef __cplusplus
}
#endif

#endif
