; kernel/head.asm

[BITS 64]

global _start
extern Start_Kernel

_start:
    ;======= 加载 GDTR 和 IDTR
    lgdt [rel GDT_POINTER]
    lidt [rel IDT_POINTER]

    ;======= 设置数据段寄存器（加载段选择子 0x10，对应 GDT_Table 中的数据段）
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ;======= 设置初始栈（临时栈地址 0x7E00）
    mov rsp, 0x7E00

    ;======= 运行 setup_pagetables 例程，将页表数据写入物理内存中
    call setup_pagetables

    ;======= 将 CR3 设置为页表的物理基地址（即 PML4 表位于 0x101000）
    mov rax, 0x101000
    mov cr3, rax

    ;======= 用远返回跳转至 switch_seg（用以更新 CS 为 64-bit 代码段）
    mov rax, qword [rel switch_seg]
    push qword 0x08       ; 0x08 对应 GDT_Table 中的 64 位代码段选择子
    push rax
    retfq                 ; 64 位远返回

;======= 64 位模式下的代码入口
align 8
switch_seg:
    dq entry64          ; 此处保存将要跳转到的 64 位入口地址

entry64:
    ; 更新段寄存器（加载数据段选择子 0x10）
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov ss, ax
    ; 设置新的栈（映射在内核高端虚拟地址 0xffff800000007E00）
    mov rsp, 0xffff800000007E00
    ; 加载跳转目标地址（go_to_kernel 指向 Start_Kernel 的地址）
    mov rax, qword [rel go_to_kernel]
    push qword 0x08      ; 同样使用 0x08 作为代码段选择子
    push rax
    retfq

go_to_kernel:
    dq Start_Kernel     ; 由 C 语言实现的内核入口函数

;======= 以下例程将页表数据写入预定的物理地址
; 页表位置及要求：
;   – PML4E 表：从 0x101000 开始，共 512 项，每项 8 字节，其中
;         项 0 和项 256 分别写入 0x102007，其余填 0
;   – PDPTE 表：从 0x102000 开始，共 512 项，项 0 写入 0x103003，其余为 0
;   – PDE 表：从 0x103000 开始，共 512 项，其中前 13 项分别为：
;         0x000083, 0x200083, 0x400083, 0x600083, 0x800083,
;         0xe0000083, 0xe0200083, 0xe0400083, 0xe0600083,
;         0xe0800083, 0xe0a00083, 0xe0c00083, 0xe0e00083，
;         其余项填 0
setup_pagetables:
    cld

    ; ----- 初始化 PML4E 表（共 512 项） -----
    ; PML4E 基地址 0x101000
    mov rdi, 0x101000
    ; 项 0
    mov rax, 0x102007
    mov [rdi], rax
    add rdi, 8
    ; 填充项 1～255 为 0
    mov ecx, 255
    xor rax, rax
    rep stosq
    ; 项 256
    mov rax, 0x102007
    mov [rdi], rax
    add rdi, 8
    ; 填充项 257～511 为 0
    mov ecx, 255
    xor rax, rax
    rep stosq

    ; ----- 初始化 PDPTE 表（共 512 项） -----
    ; PDPTE 基地址 0x102000
    mov rdi, 0x102000
    mov rax, 0x103003
    mov [rdi], rax       ; 项 0
    add rdi, 8
    mov ecx, 511
    xor rax, rax
    rep stosq

    ; ----- 初始化 PDE 表（共 512 项） -----
    ; PDE 基地址 0x103000
    mov rdi, 0x103000
    ; 写入前 13 项（各项均为 8 字节）
    mov rax, 0x000083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0x200083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0x400083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0x600083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0x800083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe0000083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe0200083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe0400083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe0600083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe0800083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe0a00083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe0c00083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe0e00083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe8000083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe8200083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe8400083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe8600083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe8800083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe8a00083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe8c00083
    mov [rdi], rax
    add rdi, 8
    mov rax, 0xe8e00083
    mov [rdi], rax
    add rdi, 8
    ; 填充剩余 499 项为 0
    mov ecx, 491
    xor rax, rax
    rep stosq

    ret

;======= 以下为数据段（.data）定义

section .data

global GDT_Table
GDT_Table:
    dq 0x0000000000000000            ; 0: NULL descriptor
    dq 0x0020980000000000            ; 1: KERNEL Code 64-bit Segment
    dq 0x0000920000000000            ; 2: KERNEL Data 64-bit Segment
    dq 0x0020f80000000000            ; 3: USER Code 64-bit Segment
    dq 0x0000f20000000000            ; 4: USER Data 64-bit Segment
    dq 0x00cf9a000000ffff            ; 5: KERNEL Code 32-bit Segment
    dq 0x00cf92000000ffff            ; 6: KERNEL Data 32-bit Segment
    times 10 dq 0                    ; 7: (TSS area reserved,占 10 项，共 80 字节)
GDT_END:

; GDT 指针结构：16 位的限长 + 64 位基地址
GDT_POINTER:
    dw GDT_END - GDT_Table - 1
    dq GDT_Table

global IDT_Table
IDT_Table:
    times 512 dq 0                  ; 512 个 8 字节条目
IDT_END:

IDT_POINTER:
    dw IDT_END - IDT_Table - 1
    dq IDT_Table

global TSS64_Table
TSS64_Table:
    times 13 dq 0                   ; 13 个 8 字节条目
TSS64_END:

TSS64_POINTER:
    dw TSS64_END - TSS64_Table - 1
    dq TSS64_Table

section .note.GNU-stack         ; 创建一个空的 .note.GNU-stack 节，告诉链接器该目标文件不需要可执行的栈
