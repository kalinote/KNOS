#ifndef __GDT_H__
#define __GDT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* GDT表声明 */
extern uint64_t GDT_Table[];  // 汇编中初始化为.quad序列

/* TSS 表声明 */
extern char TSS64_Table[];
/* TSS 结构体 */
struct TSS64 {
    uint32_t reserved0;
    uint64_t rsp[3];         // RSP0-RSP2
    uint64_t reserved1;
    uint64_t ist[7];         // IST1-IST7
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

void setup_tss64(void);

#define GDT_TSS_INDEX 8  // 对应GDT的索引8和9，每个TSS描述符占两个GDT条目

static inline uint64_t * __attribute__((always_inline)) Get_gdt() {
	uint64_t * tmp;
	__asm__ __volatile__	(
					"movq	%%cr3,	%0	\n\t"
					:"=r"(tmp)
					:
					:"memory"
				);
	return tmp;
}

#ifdef __cplusplus
}
#endif

#endif