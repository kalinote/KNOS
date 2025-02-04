# /kernel/Makefile

GCC = gcc
LD = ld
NASM = nasm
OBJCOPY = objcopy

all: system
	$(OBJCOPY) -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system ../kal/KERNEL.KAL

system: head.o main.o
	$(LD) -b elf64-x86-64 -o system head.o main.o -T kernel_link.ld

head.o:	head.asm
	$(NASM) -f elf64 head.asm -o head.o

main.o:	main.c
	$(GCC) -mcmodel=large -fno-builtin -m64 -c main.c

clean:
	rm -rf *.o *.ss *.bin system
