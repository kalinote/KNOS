/*
 * kernel/head.S
 * 内核头程序
*/

.section .text            // 定义代码段
.globl _start             // 声明全局符号_start，作为程序入口点

_start:
    cli                  // 在内核完成idt和异常处理前暂时保持中断禁用

    // 加载全局描述符表(GDT)和中断描述符表(IDT)
    lgdt    GDT_POINTER(%rip)  // 使用RIP相对寻址加载GDT地址
    lidt    IDT_POINTER(%rip)  // 加载IDT地址

    // 设置数据段寄存器为0x10（对应GDT第3个条目，内核数据段）
    mov     $0x10,  %ax        // 0x10 = 0b10000（索引2，TI=0，RPL=0）
    mov     %ax,    %ds
    mov     %ax,    %es
    mov     %ax,    %fs
    mov     %ax,    %gs
    mov     %ax,    %ss

    //======= 加载CR3寄存器（设置页表基地址）
    movq    $0x101000,  %rax   // 将页目录物理地址0x101000存入RAX
    movq    %rax,        %cr3  // 写入CR3寄存器

    // 准备长跳转切换到64位代码段
    movq    switch_seg(%rip), %rax  // 获取entry64地址
    pushq   $0x08               // 压入目标代码段选择子（0x08对应GDT第2个条目，内核代码段）
    pushq   %rax                // 压入目标地址
    lretq                       // 长返回，实现段间跳转

//======= 64位模式代码
switch_seg:
    .quad    entry64            // 存储entry64的64位地址

entry64:
    // 重新初始化段寄存器（64位模式下数据段选择子仍为0x10）
    movq    $0x10,  %rax
    movq    %rax,   %ds
    movq    %rax,   %es
    movq    %rax,   %gs
    movq    %rax,   %ss
    movq    $0xffff800000007E00, %rsp  // 设置64位栈指针地址

    // 准备跳转到内核主函数
    movq    go_to_kernel(%rip), %rax  // 获取Start_Kernel地址
    pushq   $0x08               // 压入代码段选择子
    pushq   %rax                // 压入目标地址
    lretq                       // 长返回到Start_Kernel

go_to_kernel:
    .quad    Start_Kernel       // 存储内核入口地址

//======= 页表初始化
.align 8                       // 8字节对齐

.org    0x1000                 // 定位到地址0x1000处
__PML4E:                       // 页映射第4级表(PML4)
    .quad   0x102007           // 第一个条目：指向PDPT，地址0x102000，属性PRESENT+RW+USER
    .fill   255,8,0            // 填充255个8字节0
    .quad   0x102007           // 最后一个条目（用于内核空间），同上
    .fill   255,8,0            // 填充255个8字节0

.org    0x2000                 // 定位到地址0x2000处
__PDPTE:                       // 页目录指针表(PDPT)
    .quad   0x103003           // 第一个条目：指向PD，地址0x103000，属性PRESENT+RW
    .fill   511,8,0            // 填充511个8字节0

.org    0x3000                 // 定位到地址0x3000处
__PDE:                         // 页目录表(PD)
    // 前5个2MB大页：0x0-0xA00000
    .quad   0x000083           // 属性PRESENT+RW+PS（页大小2MB）
    .quad   0x200083
    .quad   0x400083
    .quad   0x600083
    .quad   0x800083
    
    // Bochs VBE显存映射（0xe0000000-0xe0e00000）
    .quad   0xe0000083        // 地址0xe0000000
	.quad	0xe0200083
	.quad	0xe0400083
	.quad	0xe0600083
	.quad	0xe0800083
	.quad	0xe0a00083
	.quad	0xe0c00083
	.quad	0xe0e00083

	// vmware workstation VBE显存映射（0xe8000000-0xe8e00000）
    .quad   0xe8000083        // 地址0xe8000000
	.quad	0xe8200083
	.quad	0xe8400083
	.quad	0xe8600083
	.quad	0xe8800083
	.quad	0xe8a00083
	.quad	0xe8c00083
	.quad	0xe8e00083
    
    .fill   491,8,0           // 填充剩余491个条目

//======= GDT定义
.section .data
.globl GDT_Table
GDT_Table:
    .quad    0x0000000000000000  // 0 空描述符
    .quad    0x0020980000000000  // 1 内核代码段64位：DPL=0，L=1（64位）
    .quad    0x0000920000000000  // 2 内核数据段64位：DPL=0
    .quad    0x0020f80000000000  // 3 用户代码段64位：DPL=3
    .quad    0x0000f20000000000  // 4 用户数据段64位：DPL=3
    .quad    0x00cf9a000000ffff  // 5 内核代码段32位：基址0，界限0xFFFFF，粒度4KB
    .quad    0x00cf92000000ffff  // 6 内核数据段32位
    .fill    10,8,0              // 保留10个条目（TSS等）
GDT_END:

GDT_POINTER:                    // GDTR寄存器格式
GDT_LIMIT:    .word    GDT_END - GDT_Table - 1  // 表长度
GDT_BASE:     .quad    GDT_Table                // 表基地址

//======= IDT定义
.globl IDT_Table
IDT_Table:
    .fill    512,8,0           // 512个中断门，先初始化为0
IDT_END:

IDT_POINTER:                   // IDTR寄存器格式
IDT_LIMIT:    .word    IDT_END - IDT_Table - 1
IDT_BASE:     .quad    IDT_Table

//======= TSS定义
.align 16
.globl    TSS64_Table
TSS64_Table:
    .fill    13,8,0            // TSS结构（104字节）
TSS64_END:

TSS64_POINTER:                 // TR寄存器格式
TSS64_LIMIT:    .word    TSS64_END - TSS64_Table - 1
TSS64_BASE:     .quad    TSS64_Table