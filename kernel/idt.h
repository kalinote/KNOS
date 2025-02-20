#ifndef __IDT_H__
#define __IDT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "gdt.h"

// IDT条目结构（兼容x86_64架构）
#define IDT_ENTRIES 256
typedef struct __attribute__((packed)) {
    uint16_t offset_low;   // 偏移量低16位
    uint16_t selector;     // 段选择子
    uint8_t  ist;          // IST（低3位），高5位保留
    uint8_t  type_attr;    // 类型和属性（P | DPL | Type）
    uint16_t offset_mid;   // 偏移量中16位
    uint32_t offset_high;  // 偏移量高32位
    uint32_t reserved;     // 保留域
} IDTEntry;
/* IDT表声明 */
extern IDTEntry IDT_Table[IDT_ENTRIES];

/* 门类型和属性宏 */
#define GATE_TYPE_INTERRUPT 0x8E // P=1, DPL=0, 32/64位中断门
#define GATE_TYPE_TRAP      0x8F // P=1, DPL=0, 陷阱门
#define GATE_TYPE_TASK      0x85 // 任务门
/* 特权级 */
#define DPL_KERNEL 0
#define DPL_USER   3

void setup_idt(void);
void set_gate(uint8_t vector, void *handler, uint16_t selector, uint8_t type_attr, uint8_t ist);

#ifdef __cplusplus
}
#endif

#endif