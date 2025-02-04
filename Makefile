# /Makefile

FAT12IMG=python3 tools/fat12img.py
BIN_DIR = ./bin/
KAL_DIR = ./kal/

# 需要拷贝到硬盘中的文件(暂时)
DISK_FILES = $(KAL_DIR)LOADER.KAL $(KAL_DIR)KERNEL.KAL \

# 默认规则
all: kernel bootloader KNOS.vfd

.PHONY: kernel bootloader KNOS.vfd

# 生成系统和软件的镜像
KNOS.vfd: bootloader
	$(FAT12IMG) $@ format
	$(FAT12IMG) $@ write $(BIN_DIR)boot.bin 0
	for filename in $(DISK_FILES); do \
	  $(FAT12IMG) $@ save $$filename; \
	done

# 一般规则
## 编译bootloader中的代码
bootloader:
	make -C bootloader

kernel:
	make -C kernel

# 仅保留源代码(暂时)
clean:
	rm -f $(BIN_DIR)*.bin
	rm -f *.vfd
	rm -rf kal/*.kal kal/*.KAL
	make -C kernel clean
