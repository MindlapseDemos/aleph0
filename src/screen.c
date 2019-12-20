#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "screen.h"
#include "demo.h"

#define DBG_SCRCHG \
	do { \
		dbg_curscr_name = cur->name ? cur->name : "<unknown>"; \
		dbg_curscr_name_len = strlen(dbg_curscr_name); \
		dbg_curscr_name_pos = 320 - dbg_curscr_name_len * 9; \
	} while(0)

struct screen *tunnel_screen(void);
struct screen *fract_screen(void);
struct screen *grise_screen(void);
struct screen *polytest_screen(void);
struct screen *plasma_screen(void);
struct screen *bump_screen(void);
struct screen *thunder_screen(void);
struct screen *metaballs_screen(void);
struct screen *greets_screen(void);
struct screen *infcubes_screen(void);
struct screen *hairball_screen(void);

#define NUM_SCR 32
static struct screen *scr[NUM_SCR];
static int num_screens;

static struct screen *cur, *prev, *next;
static long trans_start, trans_dur;

int scr_init(void)
{
	int i, idx = 0;

	if(!(scr[idx++] = tunnel_screen())) {
		return -1;
	}
	if(!(scr[idx++] = fract_screen())) {
		return -1;
	}
	if (!(scr[idx++] = grise_screen())) {
		return -1;
	}
	if(!(scr[idx++] = polytest_screen())) {
		return -1;
	}
	if (!(scr[idx++] = plasma_screen())) {
		return -1;
	}
	if (!(scr[idx++] = bump_screen())) {
		return -1;
	}
	if (!(scr[idx++] = thunder_screen())) {
		return -1;
	}
	if(!(scr[idx++] = metaballs_screen())) {
		return -1;
	}
	if(!(scr[idx++] = greets_screen())) {
		return -1;
	}
	if(!(scr[idx++] = infcubes_screen())) {
		return -1;
	}
	if(!(scr[idx++] = hairball_screen())) {
		return -1;
	}
	num_screens = idx;

	assert(num_screens <= NUM_SCR);

	for(i=0; i<num_screens; i++) {
		if(scr[i]->init() == -1) {
			return -1;
		}
	}
	return 0;
}

void scr_shutdown(void)
{
	int i;
	for(i=0; i<num_screens; i++) {
		scr[i]->shutdown();
	}
}

void scr_update(void)
{
	if(prev) {  /* we're in the middle of a transition */
		long interval = time_msec - trans_start;
		if(interval >= trans_dur) {
			if(next->start) {
				next->start(trans_dur);
			}
			prev = 0;
			cur = next;
			next = 0;

			DBG_SCRCHG;
		}
	}
}


void scr_draw(void)
{
	if(cur) {
		cur->draw();
	}
}

void scr_keypress(int key)
{
	if(cur && cur->keypress) {
		cur->keypress(key);
	}
}

struct screen *scr_lookup(const char *name)
{
	int i;
	for(i=0; i<num_screens; i++) {
		if(strcmp(scr[i]->name, name) == 0) {
			return scr[i];
		}
	}
	return 0;
}

struct screen *scr_screen(int idx)
{
	return scr[idx];
}

int scr_num_screens(void)
{
	return num_screens;
}

int scr_change(struct screen *s, long trans_time)
{
	if(!s) return -1;
	if(s == cur) return 0;

	if(trans_time) {
		trans_dur = trans_time / 2; /* half for each part transition out then in */
		trans_start = time_msec;
	} else {
		trans_dur = 0;
	}

	if(cur && cur->stop) {
		cur->stop(trans_dur);
		prev = cur;
		next = s;
	} else {
		if(s->start) {
			s->start(trans_dur);
		}

		cur = s;
		prev = 0;

		DBG_SCRCHG;
	}
	return 0;
}
