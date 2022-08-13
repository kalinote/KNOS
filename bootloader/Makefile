# /bootloader/Makefile
# bootloader makefile

INCLUDE = -I ../include
BIN_DIR = ../bin/
KAL_DIR = ../kal/

# 默认编译指令
all: boot.bin LOADER.KAL

.PHONY: boot.bin LOADER.KAL

# bootloader
boot.bin : boot.asm
	nasm $< -o $@ $(INCLUDE)
	mv boot.bin $(BIN_DIR)
LOADER.KAL : loader.asm
	nasm $< -o $@ $(INCLUDE)
	mv LOADER.KAL $(KAL_DIR)
