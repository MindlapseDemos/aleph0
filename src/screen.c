#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "screen.h"
#include "demo.h"
#include "gfxutil.h"
#include "timer.h"

#define SCRCHG \
	do { \
		curscr_name = curscr->name ? curscr->name : "<unknown>"; \
		curscr_name_len = strlen(curscr_name); \
		curscr_name_pos = 320 - curscr_name_len * 9; \
	} while(0)

struct screen *tunnel_screen(void);
struct screen *grise_screen(void);
struct screen *bump_screen(void);
struct screen *thunder_screen(void);
struct screen *metaballs_screen(void);
struct screen *infcubes_screen(void);
struct screen *hairball_screen(void);
struct screen *minifx_screen(void);
struct screen *voxscape_screen(void);
struct screen *hexfloor_screen(void);
struct screen *juliatunnel_screen(void);
struct screen *blobgrid_screen(void);
struct screen *polka_screen(void);
struct screen *water_screen(void);
struct screen *zoom3d_screen(void);
struct screen *space_screen(void);
struct screen *molten_screen(void);
struct screen *credits_screen(void);

void start_loadscr(void);
void end_loadscr(void);
void loadscr(int n, int count);

#define NUM_SCR 32
static struct screen *scr[NUM_SCR];
static int num_screens;

static struct screen *prev, *next;
static long trans_start, trans_dur;

struct screen *curscr;
const char *curscr_name;
int curscr_name_len, curscr_name_pos;

void populate_screens(void)
{
	int idx = 0;

	scr[idx++] = tunnel_screen();
	scr[idx++] = grise_screen();
	scr[idx++] = bump_screen();
	scr[idx++] = thunder_screen();
	scr[idx++] = metaballs_screen();
	scr[idx++] = infcubes_screen();
	scr[idx++] = hairball_screen();
	scr[idx++] = minifx_screen();
	scr[idx++] = voxscape_screen();
	scr[idx++] = hexfloor_screen();
	scr[idx++] = juliatunnel_screen();
    scr[idx++] = blobgrid_screen();
	scr[idx++] = polka_screen();
	scr[idx++] = water_screen();
	scr[idx++] = zoom3d_screen();
	scr[idx++] = space_screen();
	scr[idx++] = molten_screen();
	scr[idx++] = credits_screen();

	num_screens = idx;
	assert(num_screens <= NUM_SCR);
}

struct scrtime {
	const char *name;
	long time;
};

static int scrtime_cmp(const void *a, const void *b)
{
	return ((struct scrtime*)b)->time - ((struct scrtime*)a)->time;
}

int scr_init(void)
{
	int i, idx = 0;
	unsigned long t0;
	struct scrtime itime[NUM_SCR];

	start_loadscr();

	populate_screens();

	for(i=0; i<num_screens; i++) {
		loadscr(i, num_screens);
		if(opt.dbgsingle && strcmp(scr[i]->name, opt.start_scr) != 0) {
			scr[i] = 0;
			continue;
		}
		printf("initializing %s ...\n", scr[i]->name);
		t0 = get_msec();
		if(scr[i]->init() == -1) {
			if(opt.dbgmode) {
				fprintf(stderr, "screen \"%s\" failed to initialize, removing.\n", scr[i]->name);
				scr[i] = 0;
			} else {
				return -1;
			}
		} else {
			itime[idx].name = scr[i]->name;
			itime[idx++].time = get_msec() - t0;
		}

		if(!opt.dbgmode) {
			/* for non-debug mode, remove all interactivity */
			scr[i]->keypress = 0;
		}
	}

	qsort(itime, idx, sizeof *itime, scrtime_cmp);
	printf("\ninit times (msec):\n");
	for(i=0; i<idx; i++) {
		printf("  %s: %ld\n", itime[i].name, itime[i].time);
	}
	putchar('\n');

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
			if(prev->stop) {
				prev->stop(0);
			}
			if(next->start) {
				next->start(trans_dur);
			}
			prev = 0;
			curscr = next;
			next = 0;

			SCRCHG;
		}
	}
}


void scr_draw(void)
{
	if(curscr) {
		curscr->draw();
	}
}

void scr_keypress(int key)
{
	if(curscr && curscr->keypress) {
		curscr->keypress(key);
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

int scr_change(struct screen *s)
{
	if(!s) return -1;
	if(s == curscr) return 0;

	trans_start = time_msec;

	if(curscr && curscr->stop) {
		trans_dur = curscr->trans_out;
		curscr->stop(trans_dur);
		prev = curscr;
		next = s;
	} else {
		if(s->start) {
			s->start(s->trans_in);
		}

		curscr = s;
		prev = 0;

		SCRCHG;
	}
	return 0;
}

#ifndef NO_ASM
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
	} else {
		if(!opt.dbgmode) {
			sleep_msec(300);
		}
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

#else	/* NO_ASM */

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
