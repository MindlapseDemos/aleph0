/*
colcycle - color cycling image viewer
Copyright (C) 2016  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <i86.h>
#include "pit8254.h"

#define PIT_TIMER_INTR	8
#define DOS_TIMER_INTR	0x1c

/* macro to divide and round to the nearest integer */
#define DIV_ROUND(a, b) \
	((a) / (b) + ((a) % (b)) / ((b) / 2))

static void set_timer_reload(int reload_val);
static void cleanup(void);
static void __interrupt __far timer_irq();
static void __interrupt __far dos_timer_intr();

static void (__interrupt __far *prev_timer_intr)();

static unsigned long ticks;
static unsigned long tick_interval, ticks_per_dos_intr;
static int inum;

void init_timer(int res_hz)
{
	_disable();

	if(res_hz > 0) {
		int reload_val = DIV_ROUND(OSC_FREQ_HZ, res_hz);
		set_timer_reload(reload_val);

		tick_interval = DIV_ROUND(1000, res_hz);
		ticks_per_dos_intr = DIV_ROUND(65535L, reload_val);

		inum = PIT_TIMER_INTR;
		prev_timer_intr = _dos_getvect(inum);
		_dos_setvect(inum, timer_irq);
	} else {
		tick_interval = 55;

		inum = DOS_TIMER_INTR;
		prev_timer_intr = _dos_getvect(inum);
		_dos_setvect(inum, dos_timer_intr);
	}
	_enable();

	atexit(cleanup);
}

static void cleanup(void)
{
	if(!prev_timer_intr) {
		return; /* init hasn't ran, there's nothing to cleanup */
	}

	_disable();
	if(inum == PIT_TIMER_INTR) {
		/* restore the original timer frequency */
		set_timer_reload(65535);
	}

	/* restore the original interrupt handler */
	_dos_setvect(inum, prev_timer_intr);
	_enable();
}

void reset_timer(void)
{
	ticks = 0;
}

unsigned long get_msec(void)
{
	return ticks * tick_interval;
}

static void set_timer_reload(int reload_val)
{
	outp(PORT_CMD, CMD_CHAN0 | CMD_ACCESS_BOTH | CMD_OP_SQWAVE);
	outp(PORT_DATA0, reload_val & 0xff);
	outp(PORT_DATA0, (reload_val >> 8) & 0xff);
}

static void __interrupt __far dos_timer_intr()
{
	ticks++;
	_chain_intr(prev_timer_intr);	/* DOES NOT RETURN */
}

/* first PIC command port */
#define PIC1_CMD	0x20
/* end of interrupt control word */
#define OCW2_EOI	(1 << 5)

static void __interrupt __far timer_irq()
{
	static unsigned long dos_ticks;

	ticks++;

	if(++dos_ticks >= ticks_per_dos_intr) {
		/* I suppose the dos irq handler does the EOI so I shouldn't
		 * do it if I am to call the previous function
		 */
		dos_ticks = 0;
		_chain_intr(prev_timer_intr);	/* XXX DOES NOT RETURN */
		return;	/* just for clarity */
	}

	/* send EOI to the PIC */
	outp(PIC1_CMD, OCW2_EOI);
}
