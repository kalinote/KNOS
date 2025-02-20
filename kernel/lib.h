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

#ifdef __cplusplus
}
#endif

#endif
