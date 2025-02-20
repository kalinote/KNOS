#ifndef __GATE_H__
#define __GATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

static inline void __attribute__((always_inline)) load_TR(uint16_t sel) {
    __asm__ volatile("ltr %0" : : "r"(sel));
}



#ifdef __cplusplus
}
#endif

#endif