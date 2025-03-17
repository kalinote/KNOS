#include "memory.h"
#include "lib.h"
#include "gdt.h"
#include "printk.h"

struct global_memory_manager_struct global_memory_manager_struct;

uint64_t page_init(struct page_frame_struct *page_frame, uint32_t flags) {
    // 确保传入有效页框
    if (!page_frame || !page_frame->zone) {
        fatalk("Invalid page frame or zone");
        while (1)
            __asm__ volatile("hlt");
        ;
    }

    struct memory_zone_struct *zone = page_frame->zone;

    // 场景1：初始化未使用的页框
    if ((page_frame->flags & PAGE_USED) == 0) {
        // 注意：不在这里设置位图，由调用者统一管理位图
        
        // 更新页框属性
        page_frame->flags = flags | PAGE_USED; // 默认标记为已使用
        page_frame->ref_count = 1;

        // 更新区域统计
        zone->nr_free--;
        zone->GMM_struct->huge_page_info.free_2m_pages--;

        return 0;
    }

    // 场景2：处理共享/引用页
    if ((page_frame->flags & PAGE_USER) || (flags & PAGE_USER)) {
        // 叠加属性并增加引用计数
        page_frame->flags |= flags;
        page_frame->ref_count++;
        return 0;
    }

    // 场景3：普通属性更新
    page_frame->flags |= flags;
    return 0;
}

void init_memory(void) {
    logk("Start init memory...\n");

    uint32_t *entry_count_ptr = (uint32_t *)MEMORY_STRUCT_BUFFER_ADDR;
    struct e820_entry_struct *entries = (struct e820_entry_struct *)(MEMORY_STRUCT_BUFFER_ADDR + 4);

    // 1. 保存原始E820数据到内存管理器
    global_memory_manager_struct.e820_entry_count = *entry_count_ptr;
    if (global_memory_manager_struct.e820_entry_count > E820_MAX_ENTRIES) {
        warnk("E820 entry count overflow! Truncated to %d\n", E820_MAX_ENTRIES);
        global_memory_manager_struct.e820_entry_count = E820_MAX_ENTRIES;
    }

    // 拷贝原始E820条目（逐个字段拷贝确保packed结构正确）
    for (uint32_t i = 0; i < global_memory_manager_struct.e820_entry_count; ++i) {
        memcpy(&global_memory_manager_struct.e820_entry[i], &entries[i], sizeof(struct e820_entry_struct));
    }

    // 2. 处理对齐并统计可用页
    uint64_t total_memory = 0;
    global_memory_manager_struct.huge_page_info.total_2m_pages = 0;

    uint64_t max_pfn = 0;       // 最大页框号
    for (uint32_t i = 0; i < global_memory_manager_struct.e820_entry_count; ++i) {
        struct e820_entry_struct *entry = &global_memory_manager_struct.e820_entry[i];
        uint64_t base = ((uint64_t)entry->base_addr_high << 32) | entry->base_addr_low;
        uint64_t length = ((uint64_t)entry->length_high << 32) | entry->length_low;
        uint64_t end = base + length;

        // 计算该区域的最大PFN（无论是否可用）
        uint64_t region_max_pfn = end >> PAGE_2M_SHIFT;
        if (region_max_pfn > max_pfn) {
            max_pfn = region_max_pfn;
        }

        // 打印原始内存信息
        logk("E820 entry[%02d] base:%#018lx length:%#018lx type:%s\n", i, base, length,
             memory_type_array[entry->type]);

        // 仅处理可用内存区域
        if (entry->type != 1 || length == 0)
            continue;

        // 计算对齐后的可用区域
        uint64_t aligned_base = (base + PAGE_2M_SIZE - 1) & PAGE_2M_MASK;
        uint64_t aligned_end = end & PAGE_2M_MASK; 

        if (aligned_base >= aligned_end) {
            logk("Skip non-alignable areas: base=%#lx end=%#lx\n", base, end);
            continue;
        }

        // 统计可用页
        uint64_t aligned_length = aligned_end - aligned_base;
        uint64_t pages = aligned_length / PAGE_2M_SIZE;
        global_memory_manager_struct.huge_page_info.total_2m_pages += pages;

        // 调试信息
        logk("Alignment area: base=%#018lx end=%#018lx pages=%lu (%#lx)\n", aligned_base, aligned_end, pages,
             aligned_length);

        total_memory += length; // 保持原始总内存统计
    }

    // 输出统计结果
    logk("Total memory: %#018lx bytes (%#lu MB)\n", total_memory, total_memory >> 20);
    logk("Total available 2M pages: %#lu (%#018lx bytes)\n", global_memory_manager_struct.huge_page_info.total_2m_pages,
         global_memory_manager_struct.huge_page_info.total_2m_pages * PAGE_2M_SIZE);

    logk("System total memory: %#018lx\n", total_memory);

    // 获取内核代码、数据、结束地址
    global_memory_manager_struct.kernel_addr_info.start_code = (uint64_t)&_text;
    global_memory_manager_struct.kernel_addr_info.end_code = (uint64_t)&_etext;
    global_memory_manager_struct.kernel_addr_info.end_data = (uint64_t)&_edata;
    global_memory_manager_struct.kernel_addr_info.end_krnl = (uint64_t)&_end;
    logk("start_code: %#018lx; end_code: %#018lx; end_data: %#018lx; end_krnl: %#018lx\n",
         global_memory_manager_struct.kernel_addr_info.start_code, global_memory_manager_struct.kernel_addr_info.end_code,
         global_memory_manager_struct.kernel_addr_info.end_data, global_memory_manager_struct.kernel_addr_info.end_krnl);
    
    // 初始化内存页位图
    global_memory_manager_struct.bitmap.addr = (uint64_t *)((global_memory_manager_struct.kernel_addr_info.end_krnl + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    global_memory_manager_struct.bitmap.count = max_pfn + 1;
    global_memory_manager_struct.bitmap.size = ((max_pfn + 1 + 63) / 64) * sizeof(uint64_t);
    memset((void *)global_memory_manager_struct.bitmap.addr, 0x00, global_memory_manager_struct.bitmap.size);
    
    logk("Bitmap initialized at %#018lx, size: %#018lx bytes\n", 
        (uint64_t)global_memory_manager_struct.bitmap.addr, 
        global_memory_manager_struct.bitmap.size);

    // 非可用区域标记
    for (uint32_t i = 0; i < global_memory_manager_struct.e820_entry_count; ++i) {
        struct e820_entry_struct *entry = &global_memory_manager_struct.e820_entry[i];
        if (entry->type == 1) continue; // 跳过可用区域
        uint64_t base = ((uint64_t)entry->base_addr_high << 32) | entry->base_addr_low;
        uint64_t length = ((uint64_t)entry->length_high << 32) | entry->length_low;
        uint64_t start_pfn = PAGE_2M_ALIGN(base) >> PAGE_2M_SHIFT;
        uint64_t end_pfn = (base + length) >> PAGE_2M_SHIFT;
        for (uint64_t pfn = start_pfn; pfn < end_pfn; ++pfn) {
            uint64_t word = pfn / 64;
            uint64_t bit = pfn % 64;
            global_memory_manager_struct.bitmap.addr[word] |= (1UL << bit); // 标记保留区域[^1]
        }
    }

    // 初始化页框结构体数组
    global_memory_manager_struct.page.addr = (struct page_frame_struct *)(((uint64_t)global_memory_manager_struct.bitmap.addr + global_memory_manager_struct.bitmap.size + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    global_memory_manager_struct.page.count = max_pfn + 1;
    global_memory_manager_struct.page.size = (global_memory_manager_struct.page.count * sizeof(struct page_frame_struct) + 63) & ~63;
    memset(global_memory_manager_struct.page.addr, 0x00, global_memory_manager_struct.page.size);
    for (uint64_t pfn = 0; pfn <= max_pfn; ++pfn) {
        struct page_frame_struct *page = &global_memory_manager_struct.page.addr[pfn];
        page->zone = NULL;
        page->pfn = pfn;
        page->ref_count = 0;
        page->flags = PAGE_RESERVED;  // 默认标记为保留
        page->page_size = PAGE_2M;
    }

    // 初始化内存区域结构体数组
    global_memory_manager_struct.zone.addr = (struct memory_zone_struct *)(((uint64_t)global_memory_manager_struct.page.addr + global_memory_manager_struct.page.size + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    global_memory_manager_struct.zone.count = 0;
    global_memory_manager_struct.zone.size = (global_memory_manager_struct.e820_entry_count * sizeof(struct memory_zone_struct) + 63) & ~63;        // 这里直接使用e820读出的内存区域数量，虽然这个数量中包含type不是1的内存区域
    memset(global_memory_manager_struct.zone.addr, 0x00, global_memory_manager_struct.zone.size);
    
    // 初始化zone
    for (uint32_t i = 0; i < global_memory_manager_struct.e820_entry_count; ++i) {
        uint64_t start, end;
        
        // 过滤掉非可用内存区域
        if (global_memory_manager_struct.e820_entry[i].type != 1)
            continue;

        // 处理对齐
        start = PAGE_2M_ALIGN((uint64_t)global_memory_manager_struct.e820_entry[i].base_addr_high << 32 | 
                            (uint64_t)global_memory_manager_struct.e820_entry[i].base_addr_low);
        end = (((uint64_t)global_memory_manager_struct.e820_entry[i].base_addr_high << 32 | 
            (uint64_t)global_memory_manager_struct.e820_entry[i].base_addr_low) + 
            ((uint64_t)global_memory_manager_struct.e820_entry[i].length_high << 32 | 
            (uint64_t)global_memory_manager_struct.e820_entry[i].length_low)) & PAGE_2M_MASK;

        if (end <= start)
            continue;

        // 检查是否需要分割区域
        uint64_t dma_boundary = 0x1000000; // 16MB
        
        // 如果区域跨越DMA边界，需要分割
        if (start < dma_boundary && end > dma_boundary) {
            // 创建DMA区域 (start到16MB)
            struct memory_zone_struct *dma_zone;
            dma_zone = global_memory_manager_struct.zone.addr + global_memory_manager_struct.zone.count;
            global_memory_manager_struct.zone.count++;
            
            dma_zone->start_pfn = start >> PAGE_2M_SHIFT;
            dma_zone->end_pfn = dma_boundary >> PAGE_2M_SHIFT;
            dma_zone->total_pages = dma_zone->end_pfn - dma_zone->start_pfn;
            dma_zone->nr_free = dma_zone->total_pages;
            dma_zone->attr = 0;
            dma_zone->GMM_struct = &global_memory_manager_struct;
            dma_zone->type = ZONE_DMA;
            dma_zone->page_array = &global_memory_manager_struct.page.addr[dma_zone->start_pfn];
            
            // 初始化DMA区域的页框
            struct page_frame_struct *page_frame = dma_zone->page_array;
            for(int j = 0; j < dma_zone->total_pages; j++, page_frame++) {
                page_frame->zone = dma_zone;
                page_frame->pfn = dma_zone->start_pfn + j;
                page_frame->ref_count = 0;
                page_frame->flags = 0;
                page_frame->page_size = PAGE_2M;
            }
            
            logk("Created ZONE_DMA: start_pfn=%#lx, end_pfn=%#lx, pages=%#lx\n", 
                dma_zone->start_pfn, dma_zone->end_pfn, dma_zone->total_pages);
                
            // 创建Normal区域 (16MB到end)
            struct memory_zone_struct *normal_zone;
            normal_zone = global_memory_manager_struct.zone.addr + global_memory_manager_struct.zone.count;
            global_memory_manager_struct.zone.count++;
            
            normal_zone->start_pfn = dma_boundary >> PAGE_2M_SHIFT;
            normal_zone->end_pfn = end >> PAGE_2M_SHIFT;
            normal_zone->total_pages = normal_zone->end_pfn - normal_zone->start_pfn;
            normal_zone->nr_free = normal_zone->total_pages;
            normal_zone->attr = 0;
            normal_zone->GMM_struct = &global_memory_manager_struct;
            normal_zone->type = ZONE_NORMAL;
            normal_zone->page_array = &global_memory_manager_struct.page.addr[normal_zone->start_pfn];
            
            // 初始化Normal区域的页框
            page_frame = normal_zone->page_array;
            for(int j = 0; j < normal_zone->total_pages; j++, page_frame++) {
                page_frame->zone = normal_zone;
                page_frame->pfn = normal_zone->start_pfn + j;
                page_frame->ref_count = 0;
                page_frame->flags = 0;
                page_frame->page_size = PAGE_2M;
            }
            
            logk("Created ZONE_NORMAL: start_pfn=%#lx, end_pfn=%#lx, pages=%#lx\n", 
                normal_zone->start_pfn, normal_zone->end_pfn, normal_zone->total_pages);
        } 
        else {
            // 处理不需要分割的区域，保持原有逻辑
            struct memory_zone_struct *zone;
            zone = global_memory_manager_struct.zone.addr + global_memory_manager_struct.zone.count;
            global_memory_manager_struct.zone.count++;
            
            zone->total_pages = (end - start) >> PAGE_2M_SHIFT;
            zone->start_pfn = start >> PAGE_2M_SHIFT;
            zone->end_pfn = end >> PAGE_2M_SHIFT;
            zone->nr_free = zone->total_pages;
            zone->attr = 0;
            zone->GMM_struct = &global_memory_manager_struct;
            zone->type = (start < 0x1000000) ? ZONE_DMA : ZONE_NORMAL;
            zone->page_array = &global_memory_manager_struct.page.addr[zone->start_pfn];

            struct page_frame_struct *page_frame = zone->page_array;
            for(int j = 0; j < zone->total_pages; j++, page_frame++) {
                if (zone->end_pfn > max_pfn) {
                    fatalk("Zone %d end_pfn %lu exceeds max_pfn %lu",
                        i, zone->end_pfn, max_pfn);
                    while (1)
                        __asm__ volatile("hlt");
                    ;
                }

                page_frame->zone = zone;
                page_frame->pfn = zone->start_pfn + j;
                page_frame->ref_count = 0;
                page_frame->flags = 0;
                page_frame->page_size = PAGE_2M;
            }
            
            logk("Created %s: start_pfn=%#lx, end_pfn=%#lx, pages=%#lx\n", 
                zone->type == ZONE_DMA ? "ZONE_DMA" : "ZONE_NORMAL",
                zone->start_pfn, zone->end_pfn, zone->total_pages);
        }
    }

    // 单独处理第0页
    global_memory_manager_struct.page.addr[0].zone = global_memory_manager_struct.zone.addr;
    global_memory_manager_struct.page.addr[0].pfn = 0;
    global_memory_manager_struct.page.addr[0].ref_count = 0;
    global_memory_manager_struct.page.addr[0].flags = 0;
    global_memory_manager_struct.page.addr[0].page_size = PAGE_2M;

    global_memory_manager_struct.zone.size = (global_memory_manager_struct.zone.count * sizeof(struct memory_zone_struct) + 63) & ~63;

    // 复位bitmap
    global_memory_manager_struct.bitmap.addr[0] &= ~(1 << 0);

    // 内存布局验证
    logk("Memory layout verification:\n");
    logk("Bitmap range: %#018lx - %#018lx\n",
        (uint64_t)global_memory_manager_struct.bitmap.addr,
        (uint64_t)global_memory_manager_struct.bitmap.addr + global_memory_manager_struct.bitmap.size);
    logk("Page array range: %#018lx - %#018lx\n",
        (uint64_t)global_memory_manager_struct.page.addr,
        (uint64_t)global_memory_manager_struct.page.addr + global_memory_manager_struct.page.size);
    logk("Zone array range: %#018lx - %#018lx\n",
        (uint64_t)global_memory_manager_struct.zone.addr,
        (uint64_t)global_memory_manager_struct.zone.addr + global_memory_manager_struct.zone.size);
    // 输出所有已初始化的区域结构体信息
    logk("Zone info:\n");
    for(int i = 0;i < global_memory_manager_struct.zone.count; i++) {
        struct memory_zone_struct *zone = global_memory_manager_struct.zone.addr + i;
        logk("Zone%d: total_pages: %#018lx; start_pfn: %#018lx; end_pfn: %#018lx; nr_free: %#018lx\n", i,
             zone->total_pages, zone->start_pfn, zone->end_pfn, zone->nr_free);
    }
    
    // 添加位图位置计算测试，验证首页和末页的位图位置
    uint64_t first_pfn = global_memory_manager_struct.zone.addr[0].start_pfn;
    uint64_t last_pfn = global_memory_manager_struct.zone.addr[global_memory_manager_struct.zone.count-1].end_pfn - 1;
    
    logk("First PFN: %lu maps to bitmap[%lu] bit %lu\n", 
         first_pfn, first_pfn/64, first_pfn%64);
    logk("Last PFN: %lu maps to bitmap[%lu] bit %lu\n", 
         last_pfn, last_pfn/64, last_pfn%64);
    
    // 测试位图读写操作
    uint64_t test_pfn = first_pfn + 10;  // 选择一个测试PFN
    uint64_t test_word = test_pfn / 64;
    uint64_t test_bit = test_pfn % 64;
    
    logk("Test PFN: %lu, bitmap[%lu] before: %#018lx\n", 
         test_pfn, test_word, global_memory_manager_struct.bitmap.addr[test_word]);
    
    // 设置测试位
    global_memory_manager_struct.bitmap.addr[test_word] |= (1UL << test_bit);
    
    logk("After setting bit %lu: %#018lx\n", 
         test_bit, global_memory_manager_struct.bitmap.addr[test_word]);
    
    // 清除测试位（恢复原状）
    global_memory_manager_struct.bitmap.addr[test_word] &= ~(1UL << test_bit);
    
    logk("After clearing bit: %#018lx\n", 
         global_memory_manager_struct.bitmap.addr[test_word]);

    global_memory_manager_struct.struct_end = (((uint64_t)global_memory_manager_struct.zone.addr + global_memory_manager_struct.zone.size) + 63) & ~63;

    uint64_t kernel_struct_start = VIRT_TO_PHYS((uint64_t)global_memory_manager_struct.bitmap.addr);
    uint64_t kernel_struct_end = VIRT_TO_PHYS(global_memory_manager_struct.struct_end);
    uint64_t start_pfn = PAGE_2M_ALIGN(kernel_struct_start) >> PAGE_2M_SHIFT;
    uint64_t end_pfn = (PAGE_2M_ALIGN(kernel_struct_end + PAGE_2M_SIZE - 1)) >> PAGE_2M_SHIFT;
    for (uint64_t pfn = start_pfn; pfn < end_pfn; ++pfn) {
        uint64_t word = pfn / 64;
        uint64_t bit = pfn % 64;
        global_memory_manager_struct.bitmap.addr[word] |= (1UL << bit); // 直接标记位图
        struct page_frame_struct *page = &global_memory_manager_struct.page.addr[pfn];
        page->flags = PAGE_KERNEL | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USED;
        page->ref_count = 1;
        // 更新zone统计
        for (int z = 0; z < global_memory_manager_struct.zone.count; z++) {
            struct memory_zone_struct *zone = &global_memory_manager_struct.zone.addr[z];
            if (pfn >= zone->start_pfn && pfn <= zone->end_pfn) {
                zone->nr_free--;
                global_memory_manager_struct.huge_page_info.free_2m_pages--;
                break;
            }
        }
    }

    Global_CR3 = Get_gdt();

    color_printk(GREEN, BLACK,"Global_CR3\t:%#018lx\n",Global_CR3);
    color_printk(GREEN, BLACK,"*Global_CR3\t:%#018lx\n", *(uint64_t*)PHYS_TO_VIRT(Global_CR3) & (~0xff));
    color_printk(GREEN, BLACK,"**Global_CR3\t:%#018lx\n",*(uint64_t*)PHYS_TO_VIRT(*(uint64_t*)PHYS_TO_VIRT(Global_CR3) & (~0xff)) & (~0xff));

    // PML4页表中的前10个页表项清零
	for(int i = 0;i < 10;i++)
		*((uint64_t *)PHYS_TO_VIRT(Global_CR3) + i) = 0UL;
	
	flush_tlb_all();

    logk("Memory initialized!\n");
}

struct page_frame_struct *alloc_pages(enum memory_zone_type zone_type, 
                                     uint32_t nr_pages, 
                                     uint32_t flags) {
    // TODO 如果有必要，添加降级分配
    struct memory_zone_struct *target_zone = NULL;
    
    // 1. 区域选择逻辑
    for (uint32_t i = 0; i < global_memory_manager_struct.zone.count; ++i) {
        struct memory_zone_struct *zone = &global_memory_manager_struct.zone.addr[i];
        
        if (zone->type != zone_type) 
            continue;
        if (zone->nr_free >= nr_pages) {
            target_zone = zone;
            break;
        }
    }
    
    if (!target_zone) {
        warnk("No enough pages in zone %d (request %u pages)\n", zone_type, nr_pages);
        return NULL;
    }

    // 2. 在位图中查找连续空闲页
    const uint64_t zone_start_bit = target_zone->start_pfn;
    const uint64_t zone_end_bit = target_zone->end_pfn;
    uint64_t found_start = 0;
    
    // 添加调试输出，帮助诊断问题
    logk("Searching for %#u pages in zone type %d (start_pfn=%#lu, end_pfn=%#lu)\n", 
         nr_pages, zone_type, zone_start_bit, zone_end_bit);
    
    // 按64位块遍历位图
    for (uint64_t bit = zone_start_bit; bit <= zone_end_bit - nr_pages; ) {
        uint64_t word_idx = bit / 64;
        uint64_t inword_offset = bit % 64;
        
        // 调试输出
        if(bit == zone_start_bit) {
            logk("Bitmap word at start: %#018lx\n", global_memory_manager_struct.bitmap.addr[word_idx]);
        }
        
        // 新增大块分配优化（处理64页请求）
        if (nr_pages >= 64 && inword_offset == 0) {
            uint64_t needed_words = nr_pages / 64;
            uint8_t found = 1;
            for (uint64_t j = 0; j < needed_words; j++) {
                if (global_memory_manager_struct.bitmap.addr[word_idx + j] != 0) {
                    found = 0;
                    break;
                }
            }
            if (found) {
                found_start = bit;
                break;
            }
            bit += needed_words * 64;
            continue;
        }
        
        uint64_t remaining = 64 - inword_offset;
        // 处理单个位图项中的查找
        if (remaining >= nr_pages) {
            uint64_t mask = ((1UL << nr_pages) - 1) << inword_offset;
            if (!(global_memory_manager_struct.bitmap.addr[word_idx] & mask)) {
                found_start = bit;
                break;
            }
            bit += nr_pages;
        } 
        // 处理跨位图项的查找
        else {
            uint64_t mask_low = (1UL << remaining) - 1;
            uint64_t mask_high = (1UL << (nr_pages - remaining)) - 1;
            
            if (!(global_memory_manager_struct.bitmap.addr[word_idx] & (mask_low << inword_offset)) &&
                !(global_memory_manager_struct.bitmap.addr[word_idx + 1] & mask_high)) 
            {
                found_start = bit;
                break;
            }
            bit += remaining;
        }
    }

    if (!found_start) {
        warnk("No contiguous %u pages in zone %d\n", nr_pages, zone_type);
        return NULL;
    }

    // 3. 标记已分配页 - 确保位图正确设置
    logk("Found start PFN: %lu\n", found_start);
    
    for (uint32_t i = 0; i < nr_pages; ++i) {
        uint64_t pfn = found_start + i;
        struct page_frame_struct *page = &global_memory_manager_struct.page.addr[pfn];
        
        // 先检查该页是否已被分配
        uint64_t word = pfn / 64;
        uint64_t bit = pfn % 64;
        
        // 添加调试信息查看位图更新
        if (i == 0) {
            logk("Setting bitmap - word: %lu, bit: %lu, before: %#018lx\n", 
                 word, bit, global_memory_manager_struct.bitmap.addr[word]);
        }
        
        // 设置位图标记页已分配
        global_memory_manager_struct.bitmap.addr[word] |= (1UL << bit);
        
        // 验证位图已被更新
        if (i == 0) {
            logk("After setting bit: %#018lx\n", global_memory_manager_struct.bitmap.addr[word]);
        }
        
        // 设置页属性
        page_init(page, flags);
    }

    // 4. 更新区域统计
    target_zone->nr_free -= nr_pages;
    global_memory_manager_struct.huge_page_info.free_2m_pages -= nr_pages;
    
    return &global_memory_manager_struct.page.addr[found_start];
}
