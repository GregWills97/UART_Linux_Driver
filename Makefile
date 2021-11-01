# Cross compilation Makefile for ARM
# Change KERN_SRC appropriately
SRC_DIR=$(PWD)
KERN_SRC=$(SRC_DIR)/linux
BUSY_BOX=$(SRC_DIR)/busybox
MODULE_SRC=$(SRC_DIR)/src
ROOTFS_DIR=$(SRC_DIR)/rootfs
INIT_DIR=$(SRC_DIR)/init
obj-m = $(MODULE_SRC)/vpb_uart.o

.PHONY: rootfs clean

kernel: 
	make -C $(KERN_SRC) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- versatile_defconfig
	make -j8 -C $(KERN_SRC) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- 

rootfs:
	make -j8 -C $(BUSY_BOX) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-
	make -C $(BUSY_BOX) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- install
	cp -av $(BUSY_BOX)/_install/* $(ROOTFS_DIR)/
	mkdir -p $(ROOTFS_DIR)/etc
	mkdir -p $(ROOTFS_DIR)/lib
	mkdir -p $(ROOTFS_DIR)/proc
	mkdir -p $(ROOTFS_DIR)/sys
	mkdir -p $(ROOTFS_DIR)/etc/init.d
	mkdir -p $(ROOTFS_DIR)/lib/modules
 
uart_module: 
	make -C $(KERN_SRC) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- M=$(MODULE_SRC) modules
	arm-linux-gnueabi-gcc -Wall --static -O2 -mcpu=arm926ej-s $(MODULE_SRC)/uart_echo.c -o $(MODULE_SRC)/uart_echo

install: 
	cp $(INIT_DIR)/init            $(ROOTFS_DIR)
	cp $(INIT_DIR)/vpb-uart-init   $(ROOTFS_DIR)/etc/init.d/
	cp $(MODULE_SRC)/vpb_uart.ko   $(ROOTFS_DIR)/lib/modules/
	cp $(MODULE_SRC)/uart_echo     $(ROOTFS_DIR)/usr/bin/
	cd $(ROOTFS_DIR) && find . -print0 | cpio --null -ov --format=newc | gzip -9 > $(SRC_DIR)/rootfs.cpio.gz && cd $(SRC_DIR)

qemu:
	qemu-system-arm -M versatilepb -kernel $(KERN_SRC)/arch/arm/boot/zImage -dtb $(KERN_SRC)/arch/arm/boot/dts/versatile-pb.dtb -initrd $(SRC_DIR)/rootfs.cpio.gz -nographic -append "root=/dev/mem serial=ttyAMA0" 2>/dev/null
	
clean:
	make -C $(KERN_SRC) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-  M=$(MODULE_SRC) clean
	make -C $(KERN_SRC) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- clean
	make -C $(BUSY_BOX) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- clean
	rm $(ROOTFS_DIR)/lib/modules/vpb_uart.ko $(ROOTFS_DIR)/usr/bin/uart_echo $(MODULE_SRC)/uart_echo
	rm -rf $(SRC_DIR)/rootfs.cpio.gz $(ROOTFS_DIR)/*
