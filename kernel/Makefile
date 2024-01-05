# /kernel/Makefile

GCC = gcc
LD = ld
AS = as
GCC_PARAM = -mcmodel=large -fno-builtin -m64 -Iheaders

OBJECTS = head.o entry.o main.o printk.o trap.o memory.o interrupt.o task.o syscall.o cpu.o graphical.o
TARGET = system

all: $(TARGET)
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary $(TARGET) ../kal/KERNEL.KAL

$(TARGET): $(OBJECTS)
	$(LD) -b elf64-x86-64 -o $(TARGET) $(OBJECTS) -T Kernel.lds

%.o: %.c
	$(GCC) $(GCC_PARAM) -c $< -o $@

%.o: %.S
	$(GCC) -E $< > $*.s
	$(AS) --64 -o $@ $*.s

clean:
	rm -rf *.o *.s *.bin $(TARGET)
