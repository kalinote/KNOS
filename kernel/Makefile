# /kernel/Makefile

GCC = gcc
LD = ld
AS = as
GCC_PARAM = -mcmodel=large -fno-builtin -m64 -Iheaders

all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system ../kal/KERNEL.KAL

system: head.o entry.o main.o printk.o trap.o memory.o
	$(LD) -b elf64-x86-64 -o system head.o entry.o main.o printk.o trap.o memory.o -T Kernel.lds

main.o: main.c
	$(GCC) $(GCC_PARAM) -c main.c

printk.o: printk.c
	$(GCC) $(GCC_PARAM) -c printk.c

trap.o: trap.c
	$(GCC) $(GCC_PARAM) -c trap.c

memory.o: memory.c
	$(GCC) $(GCC_PARAM) -c memory.c

head.o: head.S
	$(GCC) -E head.S > head.s
	$(AS) --64 -o head.o head.s

entry.o: entry.S
	$(GCC) -E entry.S > entry.s
	$(AS) --64 -o entry.o entry.s

clean:
	rm -rf *.o *.s *.bin system
