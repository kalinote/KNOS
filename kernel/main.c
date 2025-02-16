#include "printk.h"
#include "cpu.h"
#include "vbe.h"

void Test_Printk_Function(void) {
    // 1. 基础字符串与换行
    printk("Hello, world!\n");
    printk("Welcome KNOS!\n");

    // 2. 打印整型（十进制）
    int32_t value = 2023;
    printk("Year: %d\n", value);

    // 3. 打印整型（十六进制）
    int32_t hexValue = 0x1A3F;
    printk("Hex value: 0x%x\n", hexValue);

    // 4. 多参数测试
    printk("Mix test: %d, %x, %s, %c\n", 12345, 0xABCD, "HelloString", 'A');

    // 5. 测试不带换行的连续打印
    printk("First part. ");
    printk("Second part.");
    printk("\n");

    // 6. 长数字与边界测试
    int64_t bigNum = 12345678901234567LL;
    printk("Big number: %lld\n", bigNum);

    // 7. 字符串包含特殊字符
    printk("Special chars: tab\\t, quote\\\", backslash\\\\\n");

    // 8. 测试格式化对齐（若支持）
    printk("%5d | %-8s | 0x%04x\n", 42, "Test", 0xAB);

    // 9. 循环多行输出
    int count = 10;
    printk("Counting start:\n");
    for (int i = 0; i < count; i++) {
        printk(" - Step %d\n", i);
    }

    // 10. 连续打印不同类型数据，顺便测试CPUID，获取一些CPU基本信息
	char cpu_name_str[49];
	cpu_name(cpu_name_str);
    printk("CPU name: %s, Cores: %d\n", cpu_name_str, cpu_physical_cores());

	// 11. logs
	logk("log message: %d\n", 1);
	warnk("warning message: %d\n", 2);
	errk("error message: %d\n", 3);
	fatalk("system down: %d\n", 4);
}

void Start_Kernel(void) {
    init_vbe_info();

    int32_t *frame_buffer = (int32_t *)(0xffff800000a00000);		// bochs VBE帧缓存地址，对应物理地址 0xe0000000

    // int32_t *frame_buffer = (int32_t *)(0xffff800001a00000); // VMware Workstation VBE帧缓存地址
    int32_t total_width = 1024;                              // 假设屏幕宽度为1024像素
    int32_t bar_height = 20;                                 // 每个色带20行高度

    // 彩虹色参数（按BGR顺序）
    for (int32_t bar = 0; bar < 4; bar++) {
        for (int32_t y = 0; y < bar_height; y++) {
            for (int32_t x = 0; x < total_width; x++) {
                uint8_t r = 0, g = 0, b = 0;
                uint8_t color_value = (x * 0xFF) / total_width;
                if (bar == 0) { // 蓝->青渐变
                    b = 0xFF;
                    g = color_value;
                } else if (bar == 1) { // 青->绿渐变
                    g = 0xFF;
                    b = 0xFF - color_value;
                } else if (bar == 2) { // 绿->黄渐变
                    g = 0xFF;
                    r = color_value;
                } else if (bar == 3) { // 黄->红渐变
                    r = 0xFF;
                    g = 0xFF - color_value;
                }
                *((uint8_t *)frame_buffer + 0) = b;
                *((uint8_t *)frame_buffer + 1) = g;
                *((uint8_t *)frame_buffer + 2) = r;
                *((uint8_t *)frame_buffer + 3) = 0x00;
                frame_buffer++;
            }
        }
    }

    Test_Printk_Function();

	// 打印vbe_info结构体信息
	printk("vbe_info:\n");
	printk("ModeAttributes: %#x\n", vbe_info->ModeAttributes);
	printk("XResolution: %d\n", vbe_info->XResolution);
	printk("YResolution: %d\n", vbe_info->YResolution);
	printk("BitsPerPixel: %d\n", vbe_info->BitsPerPixel);
	printk("PhysBasePtr: %#X\n", vbe_info->PhysBasePtr);
	printk("MaxPixelClock(VBE3.0, Not necessarily supported): %d\n", vbe_info->MaxPixelClock);

    while (1)
        ;
}
