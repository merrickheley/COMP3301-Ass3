#
# Makefile for the linux ext3301-filesystem routines.
#

obj-m += ext3301.o

ext3301-y := balloc.o dir.o file.o ialloc.o inode.o \
	  ioctl.o namei.o super.o symlink.o

MOD_DIR=/local/comp3301/linux-3.9.4

all: module	
	
module: 
	make -C $(MOD_DIR) M=$(PWD) ARCH=um modules
	
udbd: module
	rm -f 500K.img
	dd if=/dev/zero of=500K.img bs=500K count=0 seek=1
	mke2fs -F 500K.img
	
uml: udbd
	cd ../uml && ./uml-kernel ubda=rootfs.ext2 mem=128m umid=comp3301 eth0=daemon ubdb=../ext3301/500K.img

clean:
	rm -f 500K.img
	make -C $(MOD_DIR) M=$(PWD) ARCH=um clean
	
.PHONY: all clean module udbd uml