# /bootloader/Makefile
# bootloader makefile

INCLUDE = -I include
BIN_DIR = ../bin/

# 默认编译指令
all: boot.bin

.PHONY: boot.bin

# bootloader
boot.bin : boot.asm
	nasm $< -o $@ $(INCLUDE)
	mv boot.bin $(BIN_DIR)
