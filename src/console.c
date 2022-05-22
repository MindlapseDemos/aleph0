#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "console.h"
#include "demo.h"
#include "screen.h"

static int runcmd(void);
static int cmd_list(const char *args);
static int cmd_help(const char *args);

#define CBUF_SIZE	64
#define CBUF_MASK	(CBUF_SIZE - 1)

#define HIST_SIZE	32
#define SBUF_SIZE	4

static char cbuf[CBUF_SIZE];
static char inpbuf[CBUF_SIZE + 1], *dptr;
static int rd, wr;

static char hist[HIST_SIZE][CBUF_SIZE + 1];
static int hist_head, hist_tail;

static char sbuf[SBUF_SIZE][CBUF_SIZE + 1];
static int sbuf_head, sbuf_tail;
static int sbuf_size;


int con_init(void)
{
	wr = rd = 0;
	hist_head = hist_tail = 0;
	sbuf_head = sbuf_tail = 0;
	sbuf_size = 0;
	return 0;
}

void con_start(void)
{
	wr = rd = 0;
}

void con_stop(void)
{
}

void con_draw(uint16_t *fb)
{
	int x, y, sidx, cidx;

	/* print output buffer */
	y = 1;
	sidx = sbuf_head;
	while(sidx != sbuf_tail) {
		cs_cputs(fb, 1, y, sbuf[sidx]);
		sidx = (sidx + 1) & (SBUF_SIZE - 1);
		y += 8;
	}

	memset(fb + y++ * 320, 0xff, 640);

	cs_confont(fb, 1, y, '>' - 32);
	cidx = rd;
	x = 10;
	while(cidx != wr) {
		cs_confont(fb, x, y, cbuf[cidx] - 32);
		x += 6;
		cidx = (cidx + 1) & CBUF_MASK;
	}
	memset(fb + (y + 8) * 320, 0xff, 640);
}

int con_input(int key)
{
	switch(key) {
	case '\b':
		if(wr != rd) {
			wr = (wr + CBUF_SIZE - 1) & CBUF_MASK;
		}
		break;

	case '\n':
	case '\r':
		dptr = inpbuf;
		while(rd != wr) {
			*dptr++ = cbuf[rd];
			rd = (rd + 1) & CBUF_MASK;
		}
		*dptr = 0;
		if(inpbuf[0]) {
			/* add to history */
			memcpy(hist[hist_tail], inpbuf, dptr - inpbuf + 1);
			hist_tail = (hist_tail + 1) & (HIST_SIZE - 1);
			if(hist_tail == hist_head) {	/* ovf */
				hist_head = (hist_head + 1) & (HIST_SIZE - 1);
			}

			return runcmd();
		}
		break;

	case KB_UP:
		if(hist_head == hist_tail) break;
		hist_tail = (hist_tail + HIST_SIZE - 1) & (HIST_SIZE - 1);
		strcpy(inpbuf, hist[hist_tail]);
		break;

	default:
		if(key < 256 && isprint(key)) {
			cbuf[wr] = key;
			wr = (wr + 1) & CBUF_MASK;
			if(wr == rd) { /* overflow */
				rd = (rd + 1) & CBUF_MASK;
			}
		}
		break;
	}

	return 1;
}

void con_printf(const char *fmt, ...)
{
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = vsprintf(sbuf[sbuf_tail], fmt, ap);
	sbuf[sbuf_tail][len] = 0;
	va_end(ap);

	sbuf_tail = (sbuf_tail + 1) & (SBUF_SIZE - 1);
	if(sbuf_tail == sbuf_head) {	/* ovf */
		sbuf_head = (sbuf_head + 1) & (SBUF_SIZE - 1);
	}

	if(sbuf_size < SBUF_SIZE) sbuf_size++;
}

static struct {
	const char *name;
	int (*func)(const char*);
} cmd[] = {
	{"ls", cmd_list},
	{"help", cmd_help},
	{"?", cmd_help},
	{0, 0}
};

static int runcmd(void)
{
	int i, nscr;
	char *endp, *args;

	switch(inpbuf[0]) {
	case '/':
		nscr = scr_num_screens();
		for(i=0; i<nscr; i++) {
			if(strstr(scr_screen(i)->name, inpbuf + 1)) {
				change_screen(i);
				return 0;
			}
		}
		con_printf("no such screen: %s\n", inpbuf + 1);
		break;

	case '#':
		i = strtol(inpbuf + 1, &endp, 10);
		if(endp == inpbuf + 1) {
			con_printf("usage: #<screen number>\n");
			break;
		}
		nscr = scr_num_screens();
		if(i < 0 || i >= nscr) {
			con_printf("no such screen: %d\n", i);
			break;
		}
		change_screen(i);
		return 0;

	default:
		endp = inpbuf;
		while(*endp && isspace(*endp)) endp++;
		while(*endp && !isspace(*endp)) endp++;

		args = *endp ? endp + 1 : 0;
		*endp = 0;

		for(i=0; cmd[i].name; i++) {
			if(strcmp(inpbuf, cmd[i].name) == 0) {
				cmd[i].func(args);
				return 1;
			}
		}

		con_printf("?%s\n", inpbuf);
	}

	return 1;
}

static int cmd_list(const char *args)
{
	int i, nscr, len;
	char buf[512], *ptr = buf;

	nscr = scr_num_screens();
	for(i=0; i<nscr; i++) {
		char *sname = scr_screen(i)->name;
		len = strlen(sname);

		if(ptr - buf + len > 53) {
			*ptr = 0;
			con_printf("%s", buf);
			ptr = buf;
		}

		len = sprintf(ptr, "%s ", sname);
		ptr += len;
	}
	if(ptr > buf) {
		*ptr = 0;
		con_printf("%s", buf);
	}
	return 0;
}

static int cmd_help(const char *args)
{
	con_printf("cmds: /, #, ls, help, ?\n");
	return 0;
}
