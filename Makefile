# 编译器使用gcc
CC=gcc
# -m32 是编译32位程序
CFLAGS=-m32 -DHOST -DANDROID -I./include -I./libselinux/include 
# 链接静态库
ifeq ($(shell uname), Linux)
	CFLAGS+=-D__linux__
	LDFLAGS=-L./make_ext4fs/lib/linux
else
	CFLAGS+=-D__APPLE__ -D__MACH__
	LDFLAGS=-L./make_ext4fs/lib/osx
endif
# 调用系统库
LIBS=-lm -lz -lsqlite3 -lext4_utils_host -llz4-host -lselinux -lsparse_host
# 源码文件
SRCS= amrom.c \
	simg2img/simg2img.c \
	simg2img/sparse_crc32.c \
	make_ext4fs/make_ext4fs_main.c

.PHONY: amrom

amrom:
	@echo "Building amrom..."
	@echo "==========================================="
	@echo "CFLAGS = "$(CFLAGS)
	@echo "LDFLAGS = "$(LDFLAGS)
	@echo "LIBS = "$(LIBS)
	@echo "==========================================="
	@$(CC) -o $@ $(SRCS) $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	@echo "Cleaning..."
	@rm -rf *.o
	@rm -rf amrom
