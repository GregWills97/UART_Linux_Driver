#include "vpb_uart.h"


/* Device Globals */
struct vpb_uart_dev *vpb_uart;
int vpb_uart_major = 0;
int vpb_uart_minor = 0;

MODULE_AUTHOR("Gregory Williams");
MODULE_LICENSE("GPL v2");


/* Set configuration registers based the configuration struct */
int vpb_uart_configure(struct vpb_uart_dev* dev, struct vpb_uart_config* config) {
	uint32_t reg_data = 0;
	int ret = 0;

	//TODO Validate

	/* Acquire lock for hardware*/
	mutex_lock(&dev->hw_mutex);

	/* Disable UART */
	reg_data = ioread8(dev->iomem + CR) & ~CR_UARTEN;
	iowrite8((uint8_t)reg_data, dev->iomem + CR);

	/* Flush Fifo */
	while(ioread8(dev->iomem + FR) & FR_BUSY);
	reg_data = ioread8(dev->iomem + LCRH) & ~LCRH_FEN;
	iowrite8((uint8_t)reg_data, dev->iomem + LCRH);

	/* Set baudrate */
	//Precalculated for baudrate 9600
	iowrite16((uint16_t)0x9c, dev->iomem + IBRD);
	iowrite8((uint8_t)0x10, dev->iomem + FBRD);

	/* Set word size */
	reg_data = 0;
	switch (config->data_bits) {
		case 5:
			reg_data |= LCRH_WLEN_5BITS;
			break;
		case 6:
			reg_data |= LCRH_WLEN_6BITS;
			break;
		case 7:
			reg_data |= LCRH_WLEN_7BITS;
			break;
		case 8:
			reg_data |= LCRH_WLEN_8BITS;
			break;
		default:
			ret = 1;
			goto unlock;
	}

	/* Set Parity */
	if (config->parity) {
		reg_data |= LCRH_PEN;
		reg_data |= LCRH_EPS;
		reg_data |= LCRH_SPS;
	} else {
		reg_data &= ~LCRH_PEN;
		reg_data &= ~LCRH_EPS;
		reg_data &= ~LCRH_SPS;
	}

	/* Setup Stop bits */
	if (config->stop_bits == 1u) {
		reg_data &= ~LCRH_STP2;
	} else if (config->stop_bits == 2u) {
		reg_data |= LCRH_STP2;
	}

	/* Enable FIFOs */
	reg_data |= LCRH_FEN;
	iowrite8(reg_data, dev->iomem + LCRH);

	/* Enable uart device */
	reg_data = ioread8(dev->iomem + CR) | CR_UARTEN;
	iowrite8(reg_data, dev->iomem + CR);

unlock:
	mutex_unlock(&dev->hw_mutex);
	return ret;
}
int vpb_uart_getchar(struct vpb_uart_dev* dev, char* c) {
	uint8_t err;
	/* Lock hardware */
	mutex_lock(&dev->hw_mutex);

	if (ioread8(dev->iomem + FR) & FR_RXFE)
		return 1;
	*c = ioread8(dev->iomem + DR) & DR_DATA_MASK;
	/* Check for error */
	if (ioread8(dev->iomem + RSRECR) & RSRECR_ERR_MASK) {
		err = ioread8(dev->iomem + RSRECR);
		iowrite8(err & ~RSRECR_ERR_MASK, dev->iomem + RSRECR);
		return 1;
	}
	/* Unlock hardware */
	mutex_unlock(&dev->hw_mutex);
	return 0;
}

void vpb_uart_putchar(struct vpb_uart_dev* dev, char c) {
	/* Lock hardware */
	mutex_lock(&dev->hw_mutex);
	
	/* Write Character */
	while (ioread8(dev->iomem + FR) & FR_TXFF);
	iowrite8(c, dev->iomem + DR);

	/* Unlock hardware */
	mutex_unlock(&dev->hw_mutex);
}

void vpb_uart_putstring(struct vpb_uart_dev* dev, const char* data) {
	while (*data) {
		vpb_uart_putchar(dev, *data++);
	}
}

int vpb_uart_open(struct inode *inode, struct file *filp) {
	struct vpb_uart_dev* dev;

	/* Get parent struct vp_uart_dev from cdev struct in inode */
	dev = container_of(inode->i_cdev, struct vpb_uart_dev, uart_cdev);

	/* Assign the files private data to our struct so we can access in other methods */
	filp->private_data = dev;

	/* Success */
	return 0;
}

int vpb_uart_release(struct inode *inode, struct file *filp) {
	/* Success */
	return 0; 
}

ssize_t vpb_uart_write(struct file *filp, const char __user *buf, size_t count, 
		loff_t *f_pos)
{
	int bytes_written = 0;
	char to_write;

	/* Get correct device struct */
	struct vpb_uart_dev *dev = filp->private_data;

	/* Send all characters from userspace buffer over the UART hardware */
	while (count > 0) {
		if (copy_from_user(&to_write, &buf[bytes_written], 1))
			return -EFAULT;
		vpb_uart_putchar(dev, to_write);
		++bytes_written;
		--count;
	}
	return bytes_written;
}

ssize_t vpb_uart_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos)
{
	int bytes_read = 0;
	char to_read = 0;

	struct vpb_uart_dev *dev = filp->private_data;

	/* Read all of the characters asked for by userspace from UART hardware, then send them to userspace */
	while (count > 0) {
		if (vpb_uart_getchar(dev, &to_read)) {
			printk(KERN_WARNING "Error receiving character");
			return -EFAULT;
		}
		if (copy_to_user(&buf[bytes_read], &to_read, 1))
			return -EFAULT;
		++bytes_read;
		--count;
	}
	return bytes_read;
}

/* File operations for character device */
struct file_operations vpb_uart_fops = {
	.owner   = THIS_MODULE,
	.read    = vpb_uart_read,
	.write   = vpb_uart_write,
	.open    = vpb_uart_open,
	.release = vpb_uart_release,
};

void vpb_uart_exit(void) {
	dev_t devno = MKDEV(vpb_uart_major, vpb_uart_minor);

	/* Deallocate char device and memory */
	if (vpb_uart) {
		cdev_del(&vpb_uart->uart_cdev);
		iounmap(vpb_uart->iomem);
		kfree(vpb_uart->char_buf);
		kfree(vpb_uart);
	}
	unregister_chrdev_region(devno, 1);
}

/* Initialize the character device */
static void uart_setup_cdev(struct vpb_uart_dev *dev) {
	int err, devno = MKDEV(vpb_uart_major, vpb_uart_minor);

	cdev_init(&dev->uart_cdev, &vpb_uart_fops);
	dev->uart_cdev.owner = THIS_MODULE;
	dev->uart_cdev.ops = &vpb_uart_fops;
	err = cdev_add(&dev->uart_cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "vpb-uart: error %d adding uart char device", err);
}

int vpb_uart_init(void) {
	int result;
	dev_t dev = 0;

	/* Allocate for character device and get back dynamically assigned major number */
	result = alloc_chrdev_region(&dev, vpb_uart_minor, 1, "vpb_uart");
	vpb_uart_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "vpb-uart: can't get major %d\n", vpb_uart_major);
		return result;
	}

	/* Allocate memory for our device struct*/
	vpb_uart = kmalloc(sizeof(struct vpb_uart_dev), GFP_KERNEL);
	if (!vpb_uart) {
		result = -ENOMEM;
		printk(KERN_WARNING "vpb-uart: can't get memory error:%d\n", result);
		goto fail;
	}
	memset(vpb_uart, 0, sizeof(struct vpb_uart_dev));
	
	/* Now allocate for our character buffer */
	vpb_uart->char_buf = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if (!vpb_uart->char_buf) {
		result = -ENOMEM;
		printk(KERN_WARNING "vpb-uart: can't get memory for character buffer, error:%d\n", result);
		goto fail;
	}
	memset(vpb_uart, 0, BUFFER_SIZE);

	/* Use ioremap to map our virtual address to physical memory region */
	vpb_uart->iomem = ioremap(UART0_BASE, 0x1000);

	/* Assign Config */
	vpb_uart->config.data_bits = 8;
	vpb_uart->config.stop_bits = 1;
	vpb_uart->config.parity = 0;
	vpb_uart->config.baudrate = 9600;

	/* Initialize devices and hardware*/
	uart_setup_cdev(vpb_uart);
	mutex_init(&vpb_uart->hw_mutex);
	vpb_uart_configure(vpb_uart, &vpb_uart->config);
	return 0;

fail:
	vpb_uart_exit();
	return result;
}

module_init(vpb_uart_init);
module_exit(vpb_uart_exit);
