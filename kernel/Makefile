# /kernel/Makefile

GCC = gcc
LD = ld
NASM = nasm
OBJCOPY = objcopy 
AS = as
CFLAGS = -mcmodel=large -fno-builtin -m64 -ffreestanding -nostdlib -fno-pic -Wa,--noexecstack

all: system
	$(OBJCOPY) -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system ../kal/KERNEL.KAL

system: head.o main.o printk.o
	$(LD) -b elf64-x86-64 -z muldefs --warn-common -z noexecstack -o system head.o main.o printk.o -T kernel_link.ld

head.o:	head.S
	$(GCC) -E  head.S > head.ss
	$(AS) --64 --noexecstack -o head.o head.ss

main.o:	main.c
	$(GCC) $(CFLAGS) -c main.c

printk.o: printk.c
	$(GCC) $(CFLAGS) -c printk.c

clean:
	rm -rf *.o *.ss *.bin system
