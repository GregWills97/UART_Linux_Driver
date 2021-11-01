# Cross compilation Makefile for ARM
# Change KERN_SRC appropriately
SRC_DIR=$(PWD)
KERN_SRC=$(SRC_DIR)/linux
BUSY_BOX=$(SRC_DIR)/busybox
MODULE_SRC=$(SRC_DIR)/src
ROOTFS_DIR=$(SRC_DIR)/rootfs
INIT_DIR=$(SRC_DIR)/init


ARCH=arm
CROSS_COMPILE=arm-linux-gnueabi-

CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall --static -O2 -mcpu=arm926ej-s 


ENV=ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)

.PHONY: busybox clean realclean

all: install

kernel: 
	make -C $(KERN_SRC) $(ENV) versatile_defconfig
	make -j8 -C $(KERN_SRC) $(ENV)

busybox:
	make -j8 -C $(BUSY_BOX) $(ENV)
	make -C $(BUSY_BOX) $(ENV) install
 
uart_module: kernel
	make -C $(KERN_SRC) $(ENV) M=$(MODULE_SRC) modules
	$(CC) $(CFLAGS) $(MODULE_SRC)/uart_echo.c -o $(MODULE_SRC)/uart_echo

install: busybox uart_module
	mkdir $(ROOTFS_DIR)
	cp -av $(BUSY_BOX)/_install/* $(ROOTFS_DIR)/
	mkdir -p $(ROOTFS_DIR)/etc/init.d $(ROOTFS_DIR)/lib/modules $(ROOTFS_DIR)/proc $(ROOTFS_DIR)/sys
	cp $(INIT_DIR)/init            $(ROOTFS_DIR)
	cp $(INIT_DIR)/vpb-uart-init   $(ROOTFS_DIR)/etc/init.d/
	cp $(MODULE_SRC)/vpb_uart.ko   $(ROOTFS_DIR)/lib/modules/
	cp $(MODULE_SRC)/uart_echo     $(ROOTFS_DIR)/usr/bin/
	cd $(ROOTFS_DIR) && find . -print0 | cpio --null -ov --format=newc | gzip -9 > $(SRC_DIR)/rootfs.cpio.gz && cd $(SRC_DIR)

qemu:
	qemu-system-arm -M versatilepb -kernel $(KERN_SRC)/arch/arm/boot/zImage -dtb $(KERN_SRC)/arch/arm/boot/dts/versatile-pb.dtb -initrd $(SRC_DIR)/rootfs.cpio.gz -nographic -append "root=/dev/mem serial=ttyAMA0" 2>/dev/null
	

clean:
	rm -rf $(SRC_DIR)/rootfs.cpio.gz $(ROOTFS_DIR)
	rm $(MODULE_SRC)/uart_echo
	make -C $(KERN_SRC) $(ENV)  M=$(MODULE_SRC) clean

realclean: 
	make -C $(KERN_SRC) $(ENV) clean
	make -C $(BUSY_BOX) $(ENV) clean
	rm -rf $(SRC_DIR)/rootfs.cpio.gz $(ROOTFS_DIR)
	rm $(MODULE_SRC)/uart_echo
	make -C $(KERN_SRC) $(ENV)  M=$(MODULE_SRC) clean
