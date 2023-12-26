# /Makefile

FAT12IMG=sudo ruby tools/fat12img
BIN_DIR = ./bin/
KAL_DIR = ./kal/

# 需要拷贝到硬盘中的文件(暂时)
DISK_FILES = $(KAL_DIR)LOADER.KAL \

# 默认规则
all: bootloader KNOS.img

.PHONY: bootloader KNOS.img

# 生成系统和软件的镜像
KNOS.img: bootloader
	$(FAT12IMG) $@ format
	$(FAT12IMG) $@ write $(BIN_DIR)boot.bin 0
	for filename in $(DISK_FILES); do \
	  $(FAT12IMG) $@ save $$filename; \
	done

# 一般规则
## 编译bootloader中的代码
bootloader:
	make -C bootloader

# 仅保留源代码(暂时)
clean:
	rm -f $(BIN_DIR)*.bin
	rm -f *.img
