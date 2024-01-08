# /kernel/Makefile

GCC = gcc
LD = ld
AS = as
GCC_PARAM = -mcmodel=large -fno-builtin -m64 -Iheaders

PIC := APIC

OBJECTS = head.o entry.o main.o printk.o trap.o memory.o interrupt.o task.o syscall.o cpu.o graphical.o PIC.o
TARGET = system

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) -b elf64-x86-64 -o $(TARGET) $(OBJECTS) -T Kernel.lds
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary $(TARGET) ../kal/KERNEL.KAL

%.o: %.c
	$(GCC) $(GCC_PARAM) -c $< -o $@ -D$(PIC)

%.o: %.S
	$(GCC) -E $< > $*.s
	$(AS) --64 -o $@ $*.s

ifeq ($(PIC),APIC)
PIC.o: APIC.c
	$(GCC) $(GCC_PARAM) -c APIC.c -o PIC.o
else
PIC.o: 8259A.c
	$(GCC) $(GCC_PARAM) -c 8259A.c -o PIC.o
endif

clean:
	rm -rf *.o *.s *.bin $(TARGET)
