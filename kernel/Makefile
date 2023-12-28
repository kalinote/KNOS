# /kernel/Makefile

GCC = gcc
LD = ld
AS = as
GCC_PARAM = -mcmodel=large -fno-builtin -m64 -Iheaders

all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system ../kal/KERNEL.KAL

system: head.o main.o printk.o
	$(LD) -b elf64-x86-64 -o system head.o main.o printk.o -T Kernel.lds

main.o: main.c
	$(GCC) $(GCC_PARAM) -c main.c

printk.o: printk.c
	$(GCC) $(GCC_PARAM) -c printk.c

head.o: head.S
	$(GCC) -E head.S > head.s
	$(AS) --64 -o head.o head.s

clean:
	rm -rf *.o *.s *.bin system
