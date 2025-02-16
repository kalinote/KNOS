# /kernel/Makefile

# 工具链
GCC     := gcc
LD      := ld
NASM    := nasm
OBJCOPY := objcopy
AS      := as

# 构建参数
ASFLAGS      := --64 --noexecstack
CFLAGS       := -mcmodel=large -fno-builtin -m64 -ffreestanding \
                -nostdlib -fno-pic -Wa,--noexecstack
LD_FLAGS     := -b elf64-x86-64 -z muldefs --warn-common -z noexecstack
OBJCOPY_FLAGS:= -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary

# 生成目标
OBJS := head.o main.o printk.o vbe.o
TARGET := system
BINARY := ../kal/KERNEL.KAL

.PHONY: all clean

all: $(TARGET)
	$(OBJCOPY) $(OBJCOPY_FLAGS) $(TARGET) $(BINARY)

$(TARGET): $(OBJS) kernel_link.ld
	$(LD) $(LD_FLAGS) -o $@ $(OBJS) -T kernel_link.ld

# GAS汇编编译规则
%.o: %.S
	$(GCC) -E $< > $*.ss
	$(AS) $(ASFLAGS) -o $@ $*.ss

# C 编译规则
%.o: %.c
	$(GCC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o *.ss $(TARGET)