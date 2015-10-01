# 编译器使用gcc
CC=gcc
# -m32 是编译32位程序
CFLAGS=-m32 -DHOST -DANDROID -D__APPLE__ -D__MACH__ -I./include -I./libselinux/include 
# 链接静态库
LDFLAGS= make_ext4fs/lib/libselinux.a make_ext4fs/lib/liblz4-host.a make_ext4fs/lib/libsparse_host.a make_ext4fs/lib/libext4_utils_host.a
# 调用系统库
LIBS=-lm -lz -lsqlite3
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
