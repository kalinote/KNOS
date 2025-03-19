#ifndef __MEMORY_H__
#define __MEMORY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define MEMORY_STRUCT_BUFFER_ADDR 0xffff800000007e00        // 内存结构体缓冲区线性地址

#define PAGE_1G_SHIFT	30
#define PAGE_2M_SHIFT	21
#define PAGE_4K_SHIFT	12

#define PAGE_1G_SIZE	(1UL << PAGE_1G_SHIFT)
#define PAGE_2M_SIZE	(1UL << PAGE_2M_SHIFT)
#define PAGE_4K_SIZE	(1UL << PAGE_4K_SHIFT)

#define PAGE_1G_MASK	(~ (PAGE_1G_SIZE - 1))
#define PAGE_2M_MASK	(~ (PAGE_2M_SIZE - 1))
#define PAGE_4K_MASK	(~ (PAGE_4K_SIZE - 1))

#define PAGE_1G_ALIGN(addr)	(((uint64_t)(addr) + PAGE_1G_SIZE - 1) & PAGE_1G_MASK)
#define PAGE_2M_ALIGN(addr)	(((uint64_t)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN(addr)	(((uint64_t)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

#define E820_MAX_ENTRIES 64            // 最大e820内存区域结构数量(Linux中设置的是128，实际应该很难用得完)

#define PHYS_TO_VIRT(pa) ((void*)((uintptr_t)(pa) + 0xFFFF800000000000))
#define VIRT_TO_PHYS(va) ((uintptr_t)(va) - 0xFFFF800000000000)

struct e820_entry_struct {
    uint32_t base_addr_low;    // 基地址低32位
    uint32_t base_addr_high;   // 基地址高32位
    uint32_t length_low;      // 长度低32位
    uint32_t length_high;     // 长度高32位
    uint32_t type;           // 内存类型 (1=可用内存)
} __attribute__((packed));

// 内存类型数组
const char *memory_type_array[] = {
    [0] = "Unknown",
    [1] = "RAM",
    [2] = "ROM or Reserved",
    [3] = "ACPI Reclaim Memory",
    [4] = "ACPI NVS Memory",
};

// 页框属性标志位定义（低8位用于通用状态）
#define PAGE_FREE        0x0001  // 页空闲
#define PAGE_USED        0x0002  // 页已分配
#define PAGE_RESERVED    0x0004  // 页被保留
#define PAGE_KERNEL      0x0008  // 内核专用页
// 页表映射属性（高56位继承自页表项）
#define PAGE_PRESENT     0x0100  // Present位（继承自PTE）
#define PAGE_WRITABLE    0x0200  // 可写属性（R/W位）
#define PAGE_USER        0x0400  // 用户空间可访问（U/S位）
#define PAGE_NOCACHE     0x0800  // 禁用缓存（PCD位）
#define PAGE_WRITETHROUGH 0x1000 // 写透模式（PWT位）
#define PAGE_NX         0x2000   // 禁止执行（XD位，需要IA32_EFER.NXE=1）

enum page_size {
    PAGE_4K,
    PAGE_2M,
    PAGE_1G
};

// 页框结构体
struct page_frame_struct {
    struct memory_zone_struct *zone;    // 指向页所在内存区域结构
    uint64_t pfn;                       // 页框号
    uint16_t ref_count;                 // 引用计数
    uint32_t flags;                     // 状态标志 [31:12]保留 | [11:0]标志位
    enum page_size page_size;           // 页大小：0=4K,1=2M,2=1G(不过本系统暂时都固定2M页，不一定能用上)
};

// 内存区域类型
enum memory_zone_type {
    ZONE_DMA,       // < 16MB区域
    ZONE_DMA32,     // 64位系统不适用
    ZONE_NORMAL     // 内核直接映射区域	16MB-物理内存上限
};

// 内存区域结构
struct memory_zone_struct {
    struct page_frame_struct *page_array;   // 指向页框元信息数组
    uint64_t total_pages;                   // 总页数
    uint64_t start_pfn;                     // 起始页框号
    uint64_t end_pfn;                       // 结束页框号
    uint64_t attr;                          // 内存区域属性
    enum memory_zone_type type;             // 内存区域类型
    uint64_t nr_free;                       // 总空闲页数
    struct global_memory_manager_struct *GMM_struct;    // 指向全局内存管理结构

    // spinlock_t lock;                     // 区域自旋锁(预留)
};

// 内存管理结构
struct global_memory_manager_struct {
    struct e820_entry_struct e820_entry[E820_MAX_ENTRIES];      // e820内存区域信息
    uint32_t e820_entry_count;                                  // 这里记录了实际长度

    struct {
        uint64_t *addr;                 // 内存页位图起始地址
        uint64_t count;                 // 总页数(逻辑数量)
        uint64_t size;                // 内存页位图长度(物理内存占用长度)
    } bitmap;
    
    struct {
        struct page_frame_struct *addr;                 // 页框结构体数组起始地址
        uint64_t count;                                 // 总页框结构体数
        uint64_t size;                                // 内存页位图长度
    } page;
 
    struct {
        struct memory_zone_struct *addr;                // 内存区域起始地址
        uint64_t count;                                 // 内存区域总数
        uint64_t size;                                // 内存区域长度
    } zone;

    struct {
        uint64_t total_2m_pages;
        uint64_t free_2m_pages;
        uint64_t total_1g_pages;
    } huge_page_info;                    // 大页信息

    struct {
        uint64_t start_code;                // 内核代码段起始地址
        uint64_t end_code;                  // 内核代码段结束地址
        uint64_t end_data;                  // 内核数据段结束地址
        uint64_t end_krnl;                  // 内核程序结束地址
    } kernel_addr_info;

    uint64_t struct_end;                // 所有内存结构体结束地址
};

uint64_t *Global_CR3 = NULL;

void init_memory(void);
struct page_frame_struct *alloc_pages(enum memory_zone_type zone_type, 
                                     uint32_t nr_pages, 
                                     uint32_t flags);
void free_pages(struct page_frame_struct *page, uint32_t nr_pages);

extern struct global_memory_manager_struct global_memory_manager_struct;
extern char _text; 
extern char _etext; 
extern char _edata; 
extern char _end;

// 刷新单个TLB条目
#define flush_tlb(addr) \
    __asm__ __volatile__("invlpg (%0)" : : "r"(addr) : "memory")

// 刷新所有非全局TLB条目
#define flush_tlb_all() do { \
    unsigned long cr3_val; \
    __asm__ __volatile__("mov %%cr3, %0\n\t" \
                         "mov %0, %%cr3" \
                         : "=r"(cr3_val) : : "memory"); \
} while (0)

#ifdef __cplusplus
}
#endif

#endif