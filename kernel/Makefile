# /kernel/Makefile

GCC = gcc
LD = ld
AS = as

all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system ../kal/KERNEL.KAL

system: head.o main.o
	$(LD) -b elf64-x86-64 -o system head.o main.o -T Kernel.lds

main.o: main.c
	$(GCC) -mcmodel=large -fno-builtin -m64 -c main.c

head.o: head.S
	$(GCC) -E head.S > head.s
	$(AS) --64 -o head.o head.s

clean:
	rm -rf *.o *.s *.bin system
