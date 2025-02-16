#ifndef __VBE_H__
#define __VBE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define VBE_INFO_VIRT_ADDR 0x00008400

#pragma pack(push, 1) // 设置结构体按1字节对齐
typedef struct {
    uint16_t ModeAttributes;        // 模式属性
    uint8_t  WinAAttributes;        // 窗口A属性
    uint8_t  WinBAttributes;        // 窗口B属性
    uint16_t WinGranularity;        // 窗口粒度
    uint16_t WinSize;               // 窗口大小
    uint16_t WinASegment;           // 窗口A段地址
    uint16_t WinBSegment;           // 窗口B段地址
    uint32_t WinFuncPtr;            // 窗口功能指针
    uint16_t BytesPerScanLine;      // 每扫描行字节数

    // VBE 1.2 及以上版本
    uint16_t XResolution;           // 水平分辨率
    uint16_t YResolution;           // 垂直分辨率
    uint8_t  XCharSize;             // 字符单元宽度
    uint8_t  YCharSize;             // 字符单元高度
    uint8_t  NumberOfPlanes;        // 颜色平面数
    uint8_t  BitsPerPixel;          // 每像素位数
    uint8_t  NumberOfBanks;         // 银行数
    uint8_t  MemoryModel;           // 内存模型类型
    uint8_t  BankSize;              // 每银行大小
    uint8_t  NumberOfImagePages;    // 可用页面数
    uint8_t  Reserved1;             // 保留

    // Direct Color 相关字段（VBE 1.2 及以上版本）
    uint8_t  RedMaskSize;           // 红色掩码大小
    uint8_t  RedFieldPosition;      // 红色场位置
    uint8_t  GreenMaskSize;         // 绿色掩码大小
    uint8_t  GreenFieldPosition;    // 绿色场位置
    uint8_t  BlueMaskSize;          // 蓝色掩码大小
    uint8_t  BlueFieldPosition;     // 蓝色场位置
    uint8_t  RsvdMaskSize;          // 保留掩码大小
    uint8_t  RsvdFieldPosition;     // 保留场位置
    uint8_t  DirectColorModeInfo;   // 直接颜色模式信息

    // VBE 2.0 及以上版本
    uint32_t PhysBasePtr;           // 物理帧缓冲区地址
    uint32_t OffScreenMemOffset;    // 屏幕外内存偏移
    uint16_t OffScreenMemSize;      // 屏幕外内存大小（以KB为单位）

    // VBE 3.0 及以上版本
    uint16_t LinBytesPerScanLine;   // 线性模式下每扫描行字节数
    uint8_t  BnkNumberOfImagePages; // 银行模式下的图像页面数
    uint8_t  LinNumberOfImagePages; // 线性模式下的图像页面数
    uint8_t  LinRedMaskSize;        // 线性模式下红色掩码大小
    uint8_t  LinRedFieldPosition;   // 线性模式下红色场位置
    uint8_t  LinGreenMaskSize;      // 线性模式下绿色掩码大小
    uint8_t  LinGreenFieldPosition; // 线性模式下绿色场位置
    uint8_t  LinBlueMaskSize;       // 线性模式下蓝色掩码大小
    uint8_t  LinBlueFieldPosition;  // 线性模式下蓝色场位置
    uint8_t  LinRsvdMaskSize;       // 线性模式下保留掩码大小
    uint8_t  LinRsvdFieldPosition;  // 线性模式下保留场位置
    uint32_t MaxPixelClock;         // 最大像素时钟（以Hz为单位）
    uint8_t  Reserved2[189];        // 保留字段
} VBEModeInfoBlock;
#pragma pack(pop) // 恢复默认对齐
VBEModeInfoBlock* vbe_info = (VBEModeInfoBlock*)VBE_INFO_VIRT_ADDR;

void init_vbe_info(void);


#ifdef __cplusplus
}
#endif

#endif