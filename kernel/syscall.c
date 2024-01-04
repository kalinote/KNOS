#include "syscall.h"

system_call_t system_call_table[MAX_SYSTEM_CALL_NR] = {
    [0] = no_system_call,
    [1] = sys_printf,
    [2 ... MAX_SYSTEM_CALL_NR - 1] = no_system_call
};
