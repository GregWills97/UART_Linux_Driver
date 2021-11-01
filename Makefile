# Cross compilation Makefile for ARM
# Change KERN_SRC appropriately
SRC_DIR=$(PWD)
KERN_SRC=$(SRC_DIR)/linux
MODULE_SRC=$(SRC_DIR)/src
ROOTFS_DIR=$(SRC_DIR)/rootfs
obj-m = $(MODULE_SRC)/vpb_uart.o

kernel: 
	make -C $(KERN_SRC) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- versatile_defconfig
	make -j8 -C $(KERN_SRC) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- 
 
uart_module: 
	make -C $(KERN_SRC) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- M=$(MODULE_SRC) modules
	arm-linux-gnueabi-gcc -Wall --static -O2 -mcpu=arm926ej-s $(MODULE_SRC)/uart_echo.c -o $(MODULE_SRC)/uart_echo

install: 
	cp $(MODULE_SRC)/vpb_uart.ko   $(ROOTFS_DIR)/lib/modules/
	cp $(MODULE_SRC)/uart_echo     $(ROOTFS_DIR)/usr/bin/
	cp $(MODULE_SRC)/vpb-uart-init $(ROOTFS_DIR)/etc/init.d/
	cd $(ROOTFS_DIR) && find . -print0 | cpio --null -ov --format=newc | gzip -9 > $(SRC_DIR)/rootfs.cpio.gz && cd $(SRC_DIR)

qemu:
	qemu-system-arm -M versatilepb -kernel $(KERN_SRC)/arch/arm/boot/zImage -dtb $(KERN_SRC)/arch/arm/boot/dts/versatile-pb.dtb -initrd $(PWD)/rootfs.cpio.gz -nographic -append "root=/dev/mem serial=ttyAMA0" 2>/dev/null
	
clean:
	make -C $(KERN_SRC) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-  M=$(MODULE_SRC) clean
	make -C $(KERN_SRC) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- clean
	rm $(ROOTFS_DIR)/lib/modules/vpb_uart.ko $(ROOTFS_DIR)/usr/bin/uart_echo $(MODULE_SRC)/uart_echo
	rm rootfs.cpio.gz
