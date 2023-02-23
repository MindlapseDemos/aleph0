#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "logger.h"

static int logfd = -1, orig_fd1 = -1;

int init_logger(const char *fname)
{
	int sdev;

	if(logfd != -1) return -1;

	if((logfd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
		fprintf(stderr, "init_logger: failed to open %s: %s\n", fname, strerror(errno));
		return -1;
	}

	if(sscanf(fname, "COM%d", &sdev) == 1) {
		setup_serial(sdev - 1);
	}

	orig_fd1 = dup(1);
	close(1);
	close(2);
	dup(logfd);
	dup(logfd);
	return 0;
}

void stop_logger(void)
{
	if(logfd >= 0) {
		close(logfd);
		logfd = -1;
	}
	if(orig_fd1 >= 0) {
		close(1);
		close(2);
		dup(orig_fd1);
		dup(orig_fd1);
		orig_fd1 = -1;
	}
}

int print_tail(const char *fname)
{
	FILE *fp;
	char buf[64];
	long lineoffs[16];
	int wr, rd, c;

	if(!(fp = fopen(fname, "r"))) {
		return -1;
	}
	wr = rd = 0;
	lineoffs[wr++] = 0;
	while(fgets(buf, sizeof buf, fp)) {
		lineoffs[wr] = ftell(fp);
		wr = (wr + 1) & 0xf;
		if(wr == rd) {
			rd = (rd + 1) & 0xf;
		}
	}

	fseek(fp, lineoffs[rd], SEEK_SET);
	while((c = fgetc(fp)) != -1) {
		fputc(c, stdout);
	}
	fclose(fp);
	return 0;
}

#define UART1_BASE	0x3f8
#define UART2_BASE	0x2f8

#define UART_DATA	0
#define UART_DIVLO	0
#define UART_DIVHI	1
#define UART_FIFO	2
#define UART_LCTL	3
#define UART_MCTL	4
#define UART_LSTAT	5

#define DIV_9600			(115200 / 9600)
#define DIV_38400			(115200 / 38400)
#define LCTL_8N1			0x03
#define LCTL_DLAB			0x80
#define FIFO_ENABLE_CLEAR	0x07
#define MCTL_DTR_RTS_OUT2	0x0b
#define LST_TRIG_EMPTY		0x20

static unsigned int iobase;

static int setup_serial(int sdev)
{
	if(sdev < 0 || sdev > 1) {
		return -1;
	}
	iobase = sdev == 0 ? UART1_BASE : UART2_BASE;

	/* set clock divisor */
	outp(iobase | UART_LCTL, LCTL_DLAB);
	outp(iobase | UART_DIVLO, DIV_9600 & 0xff);
	outp(iobase | UART_DIVHI, DIV_9600 >> 8);
	/* set format 8n1 */
	outp(iobase | UART_LCTL, LCTL_8N1);
	/* assert RTS and DTR */
	outp(iobase | UART_MCTL, MCTL_DTR_RTS_OUT2);
	return 0;
}

void ser_putchar(int c)
{
	if(c == '\n') {
		ser_putchar('\r');
	}

	while((inp(iobase | UART_LSTAT) & LST_TRIG_EMPTY) == 0);
	outp(iobase | UART_DATA, c);
}

void ser_puts(const char *s)
{
	while(*s) {
		ser_putchar(*s++);
	}
}
