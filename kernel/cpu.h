/**
 * 说明：这里面的代码都是针对gcc编译环境、Intel芯片平台，对MSVC和AMD芯片平台不一定适用
 */
#ifndef __CPU_H__
#define __CPU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lib.h"

/**
 * @brief CPUID指令封装
 */
void cpuid(int32_t code, int32_t *eax, int32_t *ebx, int32_t *ecx, int32_t *edx) {
    __asm__ __volatile__ ("cpuid"
                          : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                          : "a"(code));
}

/**
 * @brief 支持 leaf 和 subleaf 的 CPUID 封装
 */
void cpuid_count(int32_t leaf, int32_t subleaf, int32_t *eax, int32_t *ebx, int32_t *ecx, int32_t *edx) {
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
    );
}

/**
 * @brief 获取CPU名称
 */
void cpu_name(char *cpu_name) {
    int32_t cpu_info[4];

    cpuid(0x80000002, &cpu_info[0], &cpu_info[1], &cpu_info[2], &cpu_info[3]);
    memcpy(cpu_name, cpu_info, sizeof(cpu_info));
    cpuid(0x80000003, &cpu_info[0], &cpu_info[1], &cpu_info[2], &cpu_info[3]);
    memcpy(cpu_name + 16, cpu_info, sizeof(cpu_info));
    cpuid(0x80000004, &cpu_info[0], &cpu_info[1], &cpu_info[2], &cpu_info[3]);
    memcpy(cpu_name + 32, cpu_info, sizeof(cpu_info));

    cpu_name[48] = '\0';
}

/**
 * @brief 获取CPU物理核心数量
 */
uint32_t cpu_physical_cores() {
    int32_t eax, ebx, ecx, edx;

    // 检查是否支持 CPUID 0x0B
    cpuid(0, &eax, &ebx, &ecx, &edx);
    if (eax < 0x0B) {
        // 不支持 leaf 0x0B
        return -1;
    }

    uint32_t smt_threads = 0;        // SMT 层级返回的逻辑处理器数（每个物理核心的线程数）
    uint32_t package_logical = 0;      // Core 层级返回的整个 Package 内的逻辑处理器总数

    // 遍历 CPUID 0x0B 的各个子层级
    for (uint32_t subleaf = 0; ; subleaf++) {
        cpuid_count(0x0B, subleaf, &eax, &ebx, &ecx, &edx);
        uint32_t level_type = (ecx >> 8) & 0xFF;
        if (level_type == 0) {
            // 已经没有更多有效层级
            break;
        }
        switch (level_type) {
            case 1: // SMT 层级
                smt_threads = ebx;
                break;
            case 2: // Core 层级
                package_logical = ebx;
                break;
            default:
                break;
        }
    }

    // 如果未检测到 SMT 信息，则默认每个核心只有 1 个线程
    if (smt_threads == 0) {
        smt_threads = 1;
    }
    // 如果没有 Core 层级信息，则尝试使用 CPUID 1 的信息（EBX[23:16]）获取整个 Package 的逻辑处理器数
    if (package_logical == 0) {
        cpuid(1, &eax, &ebx, &ecx, &edx);
        package_logical = (ebx >> 16) & 0xFF;
    }
    // 物理核心数 = Package 内逻辑处理器数 / 每个核心的 SMT 线程数
    return package_logical / smt_threads;
}

#ifdef __cplusplus
}
#endif

#endif