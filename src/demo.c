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

static void screen_evtrig(int evid, int val, void *cls);
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

static struct au_module *mod;

float sball_matrix[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

static unsigned long last_mouse_move;
static int prev_mx, prev_my, mouse_dx, mouse_dy;
static unsigned int bmask_diff, prev_bmask;

static unsigned long nframes;
static int con_active;
static int demo_running;

extern uint16_t loading_pixels[];	/* data.asm */

static struct screen *scr;

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
	g3d_framebuffer(FB_WIDTH, FB_HEIGHT, fb_pixels);

	if(opt.music) {
		if(!(mod = au_load_module("data/test.mod"))) {
			return -1;
		}
	}

	if(dseq_open("demo.seq") == -1) {
		if(dseq_open("data/demo.seq") == -1) {
			if(!opt.dbgmode) {
				return -1;
			}
		}
	}

	if(scr_init() == -1) {
		return -1;
	}

	if(dseq_isopen()) {
		/* find all the events matching screen names and assign trigger callbacks */
		int i, num_scr, evid;

		num_scr = scr_num_screens();
		for(i=0; i<num_scr; i++) {
			scr = scr_screen(i);
			if((evid = dseq_lookup(scr->name)) >= 0) {
				dseq_trig_callback(evid, DSEQ_TRIG_ALL, screen_evtrig, (void*)i);
			}
		}
	}

	/* clear the framebuffer at least once */
	memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * FB_BPP / CHAR_BIT);

	if(opt.start_scr) {
		scr = scr_lookup(opt.start_scr);
	} else {
		scr = scr_screen(0);
	}

	if(!scr || scr_change(scr, 2000) == -1) {
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
	printf("malloc statistics:\n");
	malloc_stats();
#endif

	if(opt.music) {
		au_free_module(mod);
	}
	scr_shutdown();
	g3d_destroy();

	if(time_msec) {
		float fps = (float)nframes / ((float)time_msec / 1000.0f);
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
	if(opt.dbgmode) {
		drawFps(pixels);
		if(curscr_name) {
			cs_dputs(pixels, curscr_name_pos, 240 - 16, curscr_name);
		}
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
	printf("change screen %d\n", idx);
	scr_change(scr_screen(idx), 2000);
}

void demo_keyboard(int key, int press)
{
	int evid;

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

#ifndef __EMSCRIPTEN__
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
			if(curscr_name && (evid = dseq_lookup(curscr_name)) >= 0) {
				reset_timer(dseq_evstart(evid));
			}
			dseq_start();
			break;

		case '/':
			if(!con_active) {
				con_start();
				con_active = con_input('/');
				return;
			}

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
}


void mouse_orbit_update(float *theta, float *phi, float *dist)
{
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
static void screen_evtrig(int evid, int val, void *cls)
{
	if(val <= 0) return;

	change_screen((intptr_t)cls);
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
