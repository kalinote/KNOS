.section .text

# 定义异常处理宏(无错误码)
.macro EXCEPTION_ENTRY_NOERRCODE vector, name
.global exception_entry_\name
exception_entry_\name:
    pushq $0            # 填充错误码占位符
    pushq $\vector      # 压入向量号
    jmp common_exception_stub
.endm

# 有错误码
.macro EXCEPTION_ENTRY_ERRCODE vector, name
.global exception_entry_\name
exception_entry_\name:
    pushq $\vector      # 向量号（错误码已由CPU压入）
    jmp common_exception_stub
.endm

# 异常入口表
EXCEPTION_ENTRY_NOERRCODE 0,  divide_error         # 除零错误
EXCEPTION_ENTRY_NOERRCODE 1,  debug                # 调试异常
EXCEPTION_ENTRY_NOERRCODE 2,  nmi                  # 不可屏蔽中断
EXCEPTION_ENTRY_NOERRCODE 3,  breakpoint           # 断点
EXCEPTION_ENTRY_NOERRCODE 4,  overflow             # 溢出
EXCEPTION_ENTRY_NOERRCODE 5,  bound_range          # 边界检查
EXCEPTION_ENTRY_NOERRCODE 6,  invalid_opcode       # 无效指令
EXCEPTION_ENTRY_NOERRCODE 7,  device_not_available # FPU不可用
EXCEPTION_ENTRY_ERRCODE   8,  double_fault         # 双重错误
EXCEPTION_ENTRY_ERRCODE   10, invalid_tss          # 无效TSS
EXCEPTION_ENTRY_ERRCODE   11, segment_not_present  # 段不存在
EXCEPTION_ENTRY_ERRCODE   12, stack_segment        # 栈错误
EXCEPTION_ENTRY_ERRCODE   13, general_protection   # 通用保护
EXCEPTION_ENTRY_ERRCODE   14, page_fault           # 页错误
EXCEPTION_ENTRY_NOERRCODE 16, x87_fpu_error        # x87浮点异常
EXCEPTION_ENTRY_ERRCODE   17, alignment_check      # 对齐检查
EXCEPTION_ENTRY_NOERRCODE 18, machine_check        # 机器检查
EXCEPTION_ENTRY_NOERRCODE 19, simd_exception       # SIMD浮点异常
EXCEPTION_ENTRY_NOERRCODE 20, virtualization       # 虚拟化异常


common_exception_stub:
    # 保存所有通用寄存器
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # 保存段寄存器
    movw %ds, %ax
    pushq %rax
    movw %es, %ax
    pushq %rax

    # 设置内核数据段
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es

    # 调用C处理函数
    movq %rsp, %rdi          # 传递寄存器结构体指针
    call dispatch_exception

    # 恢复段寄存器
    popq %rax
    movw %ax, %es
    popq %rax
    movw %ax, %ds

    # 恢复通用寄存器
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    # 清理错误码和向量号
    addq $16, %rsp
    iretq
