#include <stdio.h>
#include <limits.h>

#define DEVNAME	"/dev/vpb_uart"

int main(void) {
	FILE *uart_dev;

	uart_dev = fopen(DEVNAME, "r+");

	for(int i = 0; i < (INT_MAX>>10); i++) {
		char c = fgetc(uart_dev);
		if (c) {
			fputc(c, uart_dev);
		}
	}
	fclose(uart_dev);
	return 0;
}
