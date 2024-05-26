#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "screen.h"
#include "demo.h"
#include "gfxutil.h"
#include "timer.h"

#define DBG_SCRCHG \
	do { \
		dbg_curscr_name = cur->name ? cur->name : "<unknown>"; \
		dbg_curscr_name_len = strlen(dbg_curscr_name); \
		dbg_curscr_name_pos = 320 - dbg_curscr_name_len * 9; \
	} while(0)

struct screen *tunnel_screen(void);
struct screen *grise_screen(void);
struct screen *polytest_screen(void);
struct screen *plasma_screen(void);
struct screen *bump_screen(void);
struct screen *thunder_screen(void);
struct screen *metaballs_screen(void);
struct screen *infcubes_screen(void);
struct screen *hairball_screen(void);
struct screen *cybersun_screen(void);
struct screen *raytrace_screen(void);
struct screen *minifx_screen(void);
struct screen *voxscape_screen(void);
struct screen *hexfloor_screen(void);
struct screen *juliatunnel_screen(void);
struct screen *blobgrid_screen(void);
struct screen *polka_screen(void);
struct screen *dott_screen(void);
struct screen *water_screen(void);
struct screen *zoom3d_screen(void);

void start_loadscr(void);
void end_loadscr(void);
void loadscr(int n, int count);

#define NUM_SCR 32
static struct screen *scr[NUM_SCR];
static int num_screens;

static struct screen *cur, *prev, *next;
static long trans_start, trans_dur;

const char *dbg_curscr_name;
int dbg_curscr_name_len, dbg_curscr_name_pos;

void populate_screens(void)
{
	int idx = 0;

	scr[idx++] = tunnel_screen();
	scr[idx++] = grise_screen();
	scr[idx++] = polytest_screen();
	scr[idx++] = plasma_screen();
	scr[idx++] = bump_screen();
	scr[idx++] = thunder_screen();
	scr[idx++] = metaballs_screen();
	scr[idx++] = infcubes_screen();
	scr[idx++] = hairball_screen();
	scr[idx++] = cybersun_screen();
	scr[idx++] = raytrace_screen();
	scr[idx++] = minifx_screen();
	scr[idx++] = voxscape_screen();
	scr[idx++] = hexfloor_screen();
	scr[idx++] = juliatunnel_screen();
    scr[idx++] = blobgrid_screen();
	scr[idx++] = polka_screen();
	scr[idx++] = dott_screen();
	scr[idx++] = water_screen();
	scr[idx++] = zoom3d_screen();

	num_screens = idx;
	assert(num_screens <= NUM_SCR);
}

int scr_init(void)
{
	int i, idx;

	start_loadscr();

	populate_screens();

	for(i=0; i<num_screens; i++) {
		loadscr(i, num_screens);
		if(scr[i]->init() == -1) {
			if(opt.dbgmode) {
				fprintf(stderr, "screen \"%s\" failed to initialize, removing.\n", scr[i]->name);
				scr[i] = 0;
			} else {
				return -1;
			}
		}
	}

	/* remove any null pointers (failed init screens) from the array */
	idx = 0;
	while(idx < num_screens) {
		if(!scr[idx]) {
			if(idx < --num_screens) {
				scr[idx] = scr[num_screens];
			}
		} else {
			idx++;
		}
	}

	end_loadscr();
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

#ifndef __EMSCRIPTEN__
/* loading screen */
extern uint16_t loading_pixels[];
static long prev_load_msec;
static long load_delay;

void start_loadscr(void)
{
	char *env;
	void *pixels = loading_pixels;

	if((env = getenv("MLAPSE_LOADDELAY"))) {
		load_delay = atoi(env);
		printf("load delay: %ld ms\n", load_delay);
	}

	swap_buffers(pixels);
	if(load_delay) {
		sleep_msec(load_delay * 2);
	}
	prev_load_msec = get_msec();
}

#define SPLAT_X 288
#define SPLAT_Y 104

#define FING_X	217
#define FING_LAST_X	291
#define FING_Y	151
#define FING_W	7
#define FING_H	8

void end_loadscr(void)
{
	void *pixels = loading_pixels;

	blitfb(loading_pixels + SPLAT_Y * 320 + SPLAT_X, loading_pixels + 320 * 240, 32, 72, 32);
	blit_key(loading_pixels + FING_Y * 320 + FING_LAST_X, 320, loading_pixels + 247 * 320 + 64, FING_W, FING_H, FING_W, 0);
	swap_buffers(pixels);
	if(load_delay) {
		sleep_msec(load_delay * 3);
	}
}

void loadscr(int n, int count)
{
	int xoffs = 75 * n / (count - 1);
	static int prev_xoffs;
	uint16_t *sptr, *dptr;
	long delta;
	void *pixels = loading_pixels;

	sptr = loading_pixels + 247 * 320 + 64;
	dptr = loading_pixels + FING_Y * 320 + FING_X + prev_xoffs;

	while(prev_xoffs < xoffs) {
		blit_key(dptr, 320, sptr, FING_W, FING_H, FING_W, 0);
		dptr++;
		prev_xoffs++;
	}

	swap_buffers(pixels);

	delta = get_msec() - prev_load_msec;
	if(delta < load_delay) {
		sleep_msec(load_delay - delta);
	}
	prev_load_msec = get_msec();
}

#else	/* __EMSCRIPTEN__ */

void start_loadscr(void)
{
}

void end_loadscr(void)
{
}

void loadscr(int n, int count)
{
}

#endif
