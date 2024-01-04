#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "printk.h"
#include "ptrace.h"

#define MAX_SYSTEM_CALL_NR 128
typedef unsigned long (* system_call_t)(struct pt_regs * regs);

static unsigned long no_system_call(struct pt_regs *regs) {
    color_printk(RED, BLACK, "no_system_call is calling,NR:%#04x\n", regs->rax);
    return -1;
}

static unsigned long sys_printf(struct pt_regs *regs) {
    color_printk(BLACK, WHITE, (char *)regs->rdi);
    return 1;
}

extern system_call_t system_call_table[];

#endif
