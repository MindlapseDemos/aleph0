#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>

#ifdef __WATCOMC__
#include <i86.h>
#endif

#ifdef __DJGPP__
#include <stdint.h>
#include <dpmi.h>
#include <go32.h>
#include <pc.h>
#endif

#include "sball.h"

struct motion {
	int x, y, z;
	int rx, ry, rz;
};

#define UART1_BASE	0x3f8
#define UART2_BASE	0x2f8
#define UART1_IRQ	4
#define UART2_IRQ	3

#define UART_DATA	0
#define UART_INTR	1
#define UART_DIVLO	0
#define UART_DIVHI	1
#define UART_FIFO	2
#define UART_IID	2
#define UART_LCTL	3
#define UART_MCTL	4
#define UART_LSTAT	5
#define UART_MSTAT	6

/* interrupt enable register bits */
#define INTR_RECV	1
#define INTR_SEND	2
#define INTR_LSTAT	4
#define INTR_DELTA	8

/* fifo control register bits */
#define FIFO_ENABLE		0x01
#define FIFO_RECV_CLEAR	0x02
#define FIFO_SEND_CLEAR	0x04
#define FIFO_DMA		0x08
#define FIFO_TRIG_4		0x40
#define FIFO_TRIG_8		0x80
#define FIFO_TRIG_14	0xc0

/* interrupt id register bits */
#define IID_PENDING		0x01
#define IID_ID0			0x02
#define IID_ID1			0x04
#define IID_ID2			0x08
#define IID_FIFO_EN		0xc0

#define IID_SOURCE		0xe

#define IID_DELTA		0
#define IID_SEND		0x2
#define IID_RECV		0x4
#define IID_FIFO		0xc
#define IID_STATUS		0x6

/* line control register bits */
#define LCTL_BITS_8	0x03
#define LCTL_STOP_2	0x04
#define LCTL_DLAB	0x80
#define LCTL_8N1	LCTL_BITS_8
#define LCTL_8N2	(LCTL_BITS_8 | LCTL_STOP_2)

/* modem control register bits */
#define MCTL_DTR	0x01
#define MCTL_RTS	0x02
#define MCTL_OUT1	0x04
#define MCTL_OUT2	0x08
#define MCTL_LOOP	0x10

/* line status register bits */
#define LST_DRDY		0x01
#define LST_ERR_OVER	0x02
#define LST_ERR_PARITY	0x04
#define LST_ERR_FRAME	0x08
#define LST_ERR_BRK		0x10
#define LST_TREG_EMPTY	0x20
#define LST_TIDLE		0x40
#define LST_ERROR		0x80

/* modem status register bits */
#define MST_DELTA_CTS	0x01
#define MST_DELTA_DSR	0x02
#define MST_TERI		0x04
#define MST_DELTA_DCD	0x08
#define MST_CTS			0x10
#define MST_DSR			0x20
#define MST_RING		0x40
#define MST_DCD			0x80

/* interrupt controller stuff */
#define PIC1_CMD_PORT	0x20
#define PIC1_DATA_PORT	0x21
#define PIC2_CMD_PORT	0xa0
#define PIC2_DATA_PORT	0xa1
#define OCW2_EOI		0x20

struct packet {
	int id;
	char data[80];
};

static int init_smouse(void);
static void read_motion(int *m, const char *s);
static void read_keystate(unsigned int *stptr, const char *s);
static void procpkt(struct packet *p);
static void enqueue_event(sball_event *ev);

#define COM_FMT_8N1		LCTL_8N1
#define COM_FMT_8N2		LCTL_8N2
static void com_setup(int port, int baud, unsigned int fmt);
static void com_close(void);

static void com_putc(char c);
static void com_puts(const char *s);
static int com_getc(void);
static char *com_gets(char *buf, int sz);

static int com_have_recv(void);
static int com_can_send(void);

#ifdef __WATCOMC__
#define INTERRUPT	__interrupt __far
static void (INTERRUPT *prev_recv_intr)(void);
#endif

#ifdef __DJGPP__
#define INTERRUPT

static _go32_dpmi_seginfo intr, prev_intr;

#define outp(port, val)	outportb(port, val)
#define inp(port) inportb(port)
#endif

static void INTERRUPT recv_intr(void);

static int uart_base, uart_intr_num;

static struct packet pktbuf[16];
static int pktbuf_ridx, pktbuf_widx;
#define BNEXT(x)	(((x) + 1) & 0xf)
#define BEMPTY(b)	(b##_ridx == b##_widx)

static sball_event evbuf[16];
static int evbuf_ridx, evbuf_widx;


int sball_init(void)
{
	com_setup(0, 9600, COM_FMT_8N2);
	init_smouse();
	return 0;
}

void sball_shutdown(void)
{
	com_close();
}

int sball_getdev(void)
{
	return 0;
}

int sball_pending(void)
{
	_disable();
	while(!BEMPTY(pktbuf)) {
		procpkt(pktbuf + pktbuf_ridx);
		pktbuf_ridx = BNEXT(pktbuf_ridx);
	}
	_enable();
	return !BEMPTY(evbuf);
}

int sball_getevent(sball_event *ev)
{
	_disable();
	while(!BEMPTY(pktbuf)) {
		procpkt(pktbuf + pktbuf_ridx);
		pktbuf_ridx = BNEXT(pktbuf_ridx);
	}
	_enable();

	if(BEMPTY(evbuf)) {
		return 0;
	}
	*ev = evbuf[evbuf_ridx];
	evbuf_ridx = BNEXT(evbuf_ridx);
	return 1;
}

static int init_smouse(void)
{
	/* try repeatedly zeroing the device until we get a response */
	do {
		delay(500);
		com_puts("z\r");
	} while(BEMPTY(pktbuf));

	/* then ask for id string and request motion updates */
	com_puts("vQ\r");
	com_puts("m3\r");
	return 0;
}

static void procpkt(struct packet *p)
{
	static unsigned int bnstate;
	int i;
	unsigned int st, delta, prev;
	sball_event *ev;

	switch(p->id) {
	case 'd':
		ev = evbuf + evbuf_widx;
		read_motion(ev->motion.motion, p->data);
		ev->type = SBALL_EV_MOTION;
		enqueue_event(ev);
		break;

	case 'k':
		read_keystate(&st, p->data);

		delta = st ^ bnstate;
		prev = bnstate;
		bnstate = st;

		for(i=0; i<32; i++) {
			if(delta & 1) {
				ev = evbuf + evbuf_widx;
				ev->type = SBALL_EV_BUTTON;
				ev->button.id = i;
				ev->button.pressed = st & 1;
				ev->button.state = prev ^ (1 << i);
				enqueue_event(ev);
			}
			st >>= 1;
			delta >>= 1;
		}
		break;

	case 'v':
		printf("Device: %s\n", p->data);
		break;
	/*
	default:
		printf("DBG %c -> %s\n", (char)p->id, p->data);
	*/
	}
}

static void enqueue_event(sball_event *ev)
{
	if(ev != evbuf + evbuf_widx) {
		evbuf[evbuf_widx] = *ev;
	}

	evbuf_widx = BNEXT(evbuf_widx);
	if(evbuf_widx == evbuf_ridx) {
		fprintf(stderr, "enqueue_event: overflow, dropping oldest\n");
		evbuf_ridx = BNEXT(evbuf_ridx);
	}
}

static void com_setup(int port, int baud, unsigned int fmt)
{
	unsigned char ctl;
	unsigned short div = 115200 / baud;
	static int base[] = {UART1_BASE, UART2_BASE};
	static int irq[] = {UART1_IRQ, UART2_IRQ};

	uart_base = base[port];
	uart_intr_num = irq[port] | 8;

	_disable();
#ifdef __WATCOMC__
	prev_recv_intr = _dos_getvect(uart_intr_num);
	_dos_setvect(uart_intr_num, recv_intr);
#endif
#ifdef __DJGPP__
	_go32_dpmi_get_protected_mode_interrupt_vector(uart_intr_num, &prev_intr);
	intr.pm_offset = (intptr_t)recv_intr;
	intr.pm_selector = _go32_my_cs();
	_go32_dpmi_allocate_iret_wrapper(&intr);
	_go32_dpmi_set_protected_mode_interrupt_vector(uart_intr_num, &intr);
#endif
	/* unmask the appropriate interrupt */
	outp(PIC1_DATA_PORT, inp(PIC1_DATA_PORT) & ~(1 << irq[port]));

	outp(uart_base + UART_LCTL, LCTL_DLAB);
	outp(uart_base + UART_DIVLO, div & 0xff);
	outp(uart_base + UART_DIVHI, (div >> 8) & 0xff);
	outp(uart_base + UART_LCTL, fmt);	/* fmt should be LCTL_8N1, LCTL_8N2 etc */
	outp(uart_base + UART_FIFO, FIFO_ENABLE | FIFO_SEND_CLEAR | FIFO_RECV_CLEAR);
	outp(uart_base + UART_MCTL, MCTL_DTR | MCTL_RTS | MCTL_OUT2);
	outp(uart_base + UART_INTR, INTR_RECV);

	_enable();
}

static void com_close(void)
{
	_disable();
	outp(uart_base + UART_INTR, 0);
	outp(uart_base + UART_MCTL, 0);
#ifdef __WATCOMC__
	_dos_setvect(uart_intr_num, prev_recv_intr);
#endif
#ifdef __DJGPP__
	_go32_dpmi_set_protected_mode_interrupt_vector(uart_intr_num, &prev_intr);
	_go32_dpmi_free_iret_wrapper(&intr);
#endif
	_enable();
}

static void com_putc(char c)
{
	while(!com_can_send());
	while((inp(uart_base + UART_MSTAT) & MST_CTS) == 0);
	outp(uart_base + UART_DATA, c);
}

static void com_puts(const char *s)
{
	while(*s) {
		com_putc(*s++);
	}
}

static int com_getc(void)
{
	int have;
	while(!(have = com_have_recv()));
	return inp(uart_base + UART_DATA);
}

static char *com_gets(char *buf, int sz)
{
	int c;
	char *ptr = buf;

	while(sz-- > 1 && (c = com_getc()) != -1) {
		if(c == '\r') {
			*ptr++ = '\n';
			break;
		}
		*ptr++ = c;
	}
	if(c == -1) {
		return 0;
	}
	*ptr = 0;
	return buf;
}

static int com_have_recv(void)
{
	unsigned short stat = inp(uart_base + UART_LSTAT);
	if(stat & LST_ERROR) {
		fprintf(stderr, "receive error\n");
		abort();
	}
	return stat & LST_DRDY;
}

static int com_can_send(void)
{
	return inp(uart_base + UART_LSTAT) & LST_TREG_EMPTY;
}

static void INTERRUPT recv_intr()
{
	static char buf[128];
	static char *bptr = buf;
	struct packet *pkt;
	int idreg, c, datasz;

	while(((idreg = inp(uart_base + UART_IID)) & IID_PENDING) == 0) {
		while(com_have_recv()) {
			if((c = inp(uart_base + UART_DATA)) == '\r') {
				*bptr = 0;
				datasz = bptr - buf;
				bptr = buf;

				pkt = pktbuf + pktbuf_widx;
				pktbuf_widx = BNEXT(pktbuf_widx);

				if(pktbuf_widx == pktbuf_ridx) {
					/* we overflowed, drop the oldest packet */
					pktbuf_ridx = BNEXT(pktbuf_ridx);
				}

				if(datasz > sizeof pkt->data) {
					datasz = sizeof pkt->data;	/* truncate */
				}
				pkt->id = buf[0];
				memcpy(pkt->data, buf + 1, datasz);

			} else if(bptr - buf < sizeof buf - 1) {
				*bptr++ = c;
			}
		}
	}

	outp(PIC1_CMD_PORT, OCW2_EOI);
}

static void read_motion(int *m, const char *s)
{
	int i;

	for(i=0; i<6; i++) {
		long val = ((((long)s[0] & 0xf) << 12) |
			(((long)s[1] & 0xf) << 8) |
			(((long)s[2] & 0xf) << 4) |
			((long)s[3] & 0xf)) - 32768;
		s += 4;
		*m++ = (int)val;
	}
}

static void read_keystate(unsigned int *stptr, const char *s)
{
	int i, bit = 0;
	unsigned int st = 0;

	for(i=0; i<3; i++) {
		st |= ((unsigned int)*s++ & 0xf) << bit;
		bit += 4;
	}
	*stptr = st;
}
