#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "demo.h"
#include "screen.h"
#include "dseq.h"
#include "3dgfx.h"
#include "audio.h"
#include "cfgopt.h"
#include "console.h"
#include "tinyfps.h"
#include "gfxutil.h"
#include "util.h"
#include "cpuid.h"

#ifdef __GLIBC__
#include <malloc.h>
/* for malloc_stats */
#endif


#define MOUSE_TIMEOUT	1200

#define GUARD_XPAD	0
#define GUARD_YPAD	64

static void screen_evtrig(dseq_event *ev, enum dseq_trig_mask trig, void *cls);
static void end_evtrig(dseq_event *ev, enum dseq_trig_mask trig, void *cls);
static void init_mmx_routines(void);

int fb_width, fb_height, fb_bpp, fb_scan_size;
float fb_aspect;
long fb_size, fb_buf_size;
uint16_t *fb_pixels, *vmem;
uint16_t *fb_buf;

struct image fbimg;

unsigned long time_msec;
int mouse_x, mouse_y;
unsigned int mouse_bmask;

struct font demofont;

static struct au_module *mod;

float sball_matrix[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
float sball_view_matrix[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

static unsigned long last_mouse_move;
static int prev_mx, prev_my, mouse_dx, mouse_dy;
static unsigned int bmask_diff, prev_bmask;

static unsigned long nframes;
static int con_active;
static int running, show_dseq_dbg;
static struct screen *runonlypart;

extern uint16_t loading_pixels[];	/* data.asm */

static struct screen *scr;

static int showfps, showpat;


int demo_init_cfgopt(int argc, char **argv)
{
	char *env;

	if(load_config("demo.cfg") == -1) {
		return -1;
	}
	if((env = getenv("START_SCR"))) {
		opt.start_scr = env;
	}
	if(parse_args(argc, argv) == -1) {
		return -1;
	}
	return 0;
}

int demo_init(void)
{
	if(opt.dbgmode) {
		/*enable_fpexcept();*/
	}

	con_init();
	initFpsFonts();

	init_gfxutil();
	if(CPU_HAVE_MMX) {
		init_mmx_routines();
	}

	if(g3d_init() == -1) {
		return -1;
	}
	g3d_framebuffer(FB_WIDTH, FB_HEIGHT, fb_pixels, FB_WIDTH);

	if(opt.music) {
		if(!(mod = au_load_module("data/music.xm"))) {
			return -1;
		}
	}

	if(load_font(&demofont, "data/aleph0.gmp", PACK_RGB16(255, 220, 192)) == -1) {
		return -1;
	}

	if(opt.seqfile) {
		if(dseq_open(opt.seqfile) == -1) {
			return -1;
		}
	} else {
		if(dseq_open("demo.seq") == -1) {
			if(dseq_open("data/demo.seq") == -1) {
				if(!opt.dbgmode) {
					return -1;
				}
			}
		}
	}

	if(scr_init() == -1) {
		return -1;
	}

	if(dseq_isopen()) {
		/* find all the events matching screen names and assign trigger callbacks */
		int i, num_scr;
		dseq_event *ev;

		num_scr = scr_num_screens();
		for(i=0; i<num_scr; i++) {
			if(!(scr = scr_screen(i)) || !scr->name) {
				continue;
			}
			if((ev = dseq_lookup(scr->name))) {
				dseq_set_trigger(ev, DSEQ_TRIG_START, screen_evtrig, (void*)i);

				dseq_transtime(ev, &scr->trans_in, &scr->trans_out);
			}
		}

		if(!opt.dbgmode && (ev = dseq_lookup("quit"))) {
			dseq_set_trigger(ev, DSEQ_TRIG_START, end_evtrig, 0);
		}
	}

	/* clear the framebuffer at least once */
	memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * FB_BPP / CHAR_BIT);

	if(opt.start_scr) {
		scr = scr_lookup(opt.start_scr);
	} else {
		scr = scr_screen(0);
	}

	if(!scr || scr_change(scr) == -1) {
		fprintf(stderr, "screen %s not found\n", opt.start_scr ? opt.start_scr : "0");
		return -1;
	}

	if(!opt.dbgmode) {
		dseq_start();
	}

	if(opt.music) {
		au_play_module(mod);
	}
	return 0;
}

void demo_cleanup(void)
{
#ifdef __GLIBC__
	if(opt.dbgmode) {
		printf("malloc statistics:\n");
		malloc_stats();
	}
#endif

	destroy_font(&demofont);

	if(opt.music) {
		au_free_module(mod);
	}
	scr_shutdown();
	g3d_destroy();

	if(time_msec) {
		float sec = (float)time_msec / 1000.0f;
		float fps = (float)nframes / sec;

		if(sec > 60.0f) {
			int min = (int)(sec / 60.0f);
			sec = fmod(sec, 60.0f);
			printf("runtime: %d min, %.3f sec\n", min, sec);
		} else {
			printf("runtime: %.3f sec\n", sec);
		}
		printf("average framerate: %.1f\n", fps);
	}
}

int demo_resizefb(int width, int height, int bpp)
{
	int newsz, new_scansz;

	if(!width || !height || !bpp) {
		free(fb_buf);
		fb_buf = fb_pixels = 0;
		fb_size = fb_buf_size = fb_scan_size = 0;
		fb_width = fb_height = fb_bpp = 0;
		return 0;
	}

	new_scansz = ((width + GUARD_XPAD * 2) * bpp + 7) / 8;
	newsz = (height + GUARD_YPAD * 2) * new_scansz;

	if(!fb_buf || newsz > fb_buf_size) {
		void *tmp = malloc(newsz);
		if(!tmp) return -1;

		free(fb_buf);
		fb_buf = tmp;
		fb_buf_size = newsz;
	}

	fb_scan_size = new_scansz;
	fb_pixels = (uint16_t*)((char*)fb_buf + GUARD_YPAD * fb_scan_size + (GUARD_XPAD * bpp + 7) / 8);
	fb_width = width;
	fb_height = height;
	fb_bpp = bpp;
	fb_size = fb_scan_size * fb_height;

	fb_aspect = (float)fb_width / (float)fb_height;

	/* initialize fbimg structure */
	fbimg.pixels = fb_pixels;
	fbimg.width = width;
	fbimg.height = height;
	fbimg.scanlen = width;

	return 0;
}


void demo_draw(void)
{
	if(opt.mouse) {
		mouse_dx = mouse_x - prev_mx;
		mouse_dy = mouse_y - prev_my;
		prev_mx = mouse_x;
		prev_my = mouse_y;
		bmask_diff = mouse_bmask ^ prev_bmask;
		prev_bmask = mouse_bmask;
		if(mouse_dx | mouse_dy) {
			last_mouse_move = time_msec;
		}
	}

	if(opt.music) {
		au_update();
	}
	dseq_update();
	scr_update();
	scr_draw();

	++nframes;
}

/* called by swap_buffers just before the actual swap */
void demo_post_draw(void *pixels)
{
	if(showpat && opt.music && au_module_state(0) == AU_PLAYING) {
		char buf[128];
		sprintf(buf, "modpos: %04d\n", au_player_pos());
		cs_cputs(pixels, 240, 10, buf);
	}

	/* no consoles, cursors, fps counters and part names in non-debug mode */
	if(!opt.dbgmode) {
		if(showfps) {
			drawFps(pixels);
		}
		return;
	}

	drawFps(pixels);
	if(curscr_name) {
		cs_dputs(pixels, curscr_name_pos, 240 - 16, curscr_name);
	}

	if(show_dseq_dbg) {
		dseq_dbg_draw();
	}

	if(con_active) {
		con_draw(pixels);
	}

	if(opt.mouse && time_msec - last_mouse_move < MOUSE_TIMEOUT) {
		cs_mouseptr(pixels, mouse_x, mouse_y);
	}
}

void cs_puts_font(cs_font_func csfont, int sz, void *fb, int x, int y, const char *str)
{
	while(*str) {
		int c = *str++;

		if(c > ' ' && c < 128) {
			csfont(fb, x, y, c - ' ');
		}
		x += sz;
	}
}

void change_screen(int idx)
{
	struct screen *scr = scr_screen(idx);
	scr_change(scr);
}

void demo_keyboard(int key, int press)
{
#ifdef NDEBUG
	if(!press) return;
	switch(key) {
	case 27:
		demo_quit();
		break;

	case KB_F9:
		showfps ^= 1;
		break;

	default:
		break;
	}

#else	/* debug build */

	if(press) {
		switch(key) {
		case 27:
			if(con_active) {
				con_stop();
				con_active = 0;
			} else {
				demo_quit();
			}
			return;

#if !defined(NDEBUG) && !defined(NO_ASM)
		case 127:
			debug_break();
			return;
#endif

		case '`':
			con_active = !con_active;
			if(con_active) {
				con_start();
			} else {
				con_stop();
			}
			return;

		case KB_F1:
			if(curscr_name) {
				demo_runpart(curscr_name, 1);
			}
			break;

		case KB_F2:
			demo_run(0);
			break;

		case KB_F5:
			demo_runpart(curscr_name, 0);
			break;

		case KB_F3:
			show_dseq_dbg ^= 1;
			break;

		case KB_F9:
			showfps ^= 1;
			break;

		case KB_F10:
			showpat ^= 1;
			break;

		case '/':
			if(!con_active) {
				con_start();
				con_active = con_input('/');
				return;
			}

		case ' ':
			if(running) {
				if(dseq_started()) {
					dseq_pause();
				} else {
					dseq_resume();
				}
			}
			break;

		default:
			if(con_active) {
				con_active = con_input(key);
				return;
			}

			if(key >= '1' && key <= '9' && key <= '1' + scr_num_screens()) {
				change_screen(key - '1');
				return;
			} else if(key == '0' && scr_num_screens() >= 10) {
				change_screen(9);
				return;
			}
		}

		scr_keypress(key);
	}
#endif	/* !def NDEBUG */
}

void demo_run(long start_time)
{
	if(curscr) {
		if(curscr->stop) {
			curscr->stop(0);
		}
		curscr = 0;
	}

	runonlypart = 0;
	reset_timer(start_time);
	dseq_start();
	dseq_ffwd(start_time);

	running = 1;
}

void demo_runpart(const char *name, int single)
{
	int i, nscr;
	long evstart;
	dseq_event *ev;

	if(!(ev = dseq_lookup(name))) {
		return;
	}

	evstart = dseq_start_time(ev);
	reset_timer(evstart);

	if(curscr) {
		if(curscr->stop) {
			curscr->stop(0);
		}
		curscr = 0;
	}

	if(single) {
		runonlypart = 0;
		nscr = scr_num_screens();
		for(i=0; i<nscr; i++) {
			if(strcmp(scr_screen(i)->name, name) == 0) {
				runonlypart = scr_screen(i);
				change_screen(i);
				break;
			}
		}
		if(!runonlypart) return;
	}

	reset_timer(evstart);
	dseq_start();
	dseq_ffwd(evstart);

	running = 1;
}


void mouse_orbit_update(float *theta, float *phi, float *dist)
{
	if(!opt.dbgmode) {
		return;	/* no interactivity in non-debug mode */
	}

	if(mouse_bmask) {
		if(bmask_diff == 0) {

			if(mouse_dx | mouse_dy) {
				if(mouse_bmask & MOUSE_BN_LEFT) {
					float p = *phi;
					*theta += mouse_dx * 1.0;
					p += mouse_dy * 1.0;

					if(p < -90) p = -90;
					if(p > 90) p = 90;
					*phi = p;
				}
				if(mouse_bmask & MOUSE_BN_RIGHT) {
					*dist += mouse_dy * 0.5;

					if(*dist < 0) *dist = 0;
				}
			}
		}
	}
}

/* process demo sequence triggers for screen changes */
static void screen_evtrig(dseq_event *ev, enum dseq_trig_mask trig, void *cls)
{
	int scrno = (intptr_t)cls;
	if(runonlypart && scr_screen(scrno) != runonlypart) {
		dseq_stop();
		return;
	}
	change_screen(scrno);
}

static void end_evtrig(dseq_event *ev, enum dseq_trig_mask trig, void *cls)
{
	demo_quit();
}

/* initialize pointers to various routines which have MMX versions if they're
 * not handled elsewhere
 */
static void init_mmx_routines(void)
{
	fputs("set up MMX routines: ", stdout);

	fputs("memcpy64", stdout);
	memcpy64 = memcpy64_mmx;		/* util_s.asm */

	putchar('\n');
	fflush(stdout);
}
