#ifndef __UART_VERSATILEPB_H__
#define __UART_VERSATILEPB_H__

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

#include <asm/io.h>
#include <asm/uaccess.h>
 
#define REFCLK        24000000u   /* 24 MHz */
 
/* Hardware Registers */
#define UART0_BASE    0x101F1000
#define DR      0x0
#define RSRECR  0x4
#define FR	0x18
#define ILPR	0x20
#define IBRD	0x24
#define FBRD    0x28
#define LCRH	0x2C
#define CR	0x30


/* Hardware Data Masks */
#define DR_DATA_MASK    (0xFFu)

#define FR_BUSY         (1 << 3u)
#define FR_RXFE         (1 << 4u)
#define FR_TXFF         (1 << 5u)

#define RSRECR_ERR_MASK (0xFu)

#define LCRH_FEN        (1 << 4u)
#define LCRH_PEN        (1 << 1u)
#define LCRH_EPS        (1 << 2u)
#define LCRH_STP2       (1 << 3u)
#define LCRH_SPS        (1 << 7u)
#define CR_UARTEN       (1 << 0u)

#define LCRH_WLEN_5BITS (0u << 5u)
#define LCRH_WLEN_6BITS (1u << 5u)
#define LCRH_WLEN_7BITS (2u << 5u)
#define LCRH_WLEN_8BITS (3u << 5u)


/* Kernel Module Configurations */
#define BUFFER_SIZE	1024

struct vpb_uart_config {
	uint8_t    data_bits;
	uint8_t    stop_bits;
	uint8_t    parity;
	uint32_t   baudrate;
};

struct vpb_uart_dev {
	struct vpb_uart_config config;
	struct cdev uart_cdev;
	struct mutex hw_mutex;
	void __iomem *iomem;
	char* char_buf;
};

int vpb_uart_configure(struct vpb_uart_dev* dev, struct vpb_uart_config* config);
void vpb_uart_putchar(struct vpb_uart_dev* dev, char c);
void vpb_uart_putstring(struct vpb_uart_dev* dev, const char* data);
int vpb_uart_getchar(struct vpb_uart_dev* dev, char* c);

#endif /* __UART_VERSATILE_H__*/
