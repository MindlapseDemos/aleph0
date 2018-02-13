#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "demo.h"
#include "screen.h"
#include "3dgfx.h"
#include "music.h"
#include "cfgopt.h"
#include "tinyfps.h"

int fb_width = 320;
int fb_height = 240;
int fb_bpp = 16;
uint16_t *fb_pixels, *vmem_back, *vmem_front;
unsigned long time_msec;
int mouse_x, mouse_y;
unsigned int mouse_bmask;

float sball_matrix[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

static unsigned long nframes;

int demo_init(int argc, char **argv)
{
	struct screen *scr;
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

	initFpsFonts();

	if(g3d_init() == -1) {
		return -1;
	}
	g3d_framebuffer(fb_width, fb_height, fb_pixels);

	if(opt.music) {
		if(music_open("data/test.mod") == -1) {
			return -1;
		}
	}

	if(scr_init() == -1) {
		return -1;
	}
	if(opt.start_scr) {
		scr = scr_lookup(opt.start_scr);
	} else {
		scr = scr_screen(0);
	}

	if(!scr || scr_change(scr, 4000) == -1) {
		fprintf(stderr, "screen %s not found\n", opt.start_scr ? opt.start_scr : "0");
		return -1;
	}

	/* clear the framebuffer at least once */
	memset(fb_pixels, 0, fb_width * fb_height * fb_bpp / CHAR_BIT);

	if(opt.music) {
		music_play();
	}
	return 0;
}

void demo_cleanup(void)
{
	if(opt.music) {
		music_close();
	}
	scr_shutdown();
	g3d_destroy();

	if(time_msec) {
		float fps = (float)nframes / ((float)time_msec / 1000.0f);
		printf("average framerate: %.1f\n", fps);
	}
}

void demo_draw(void)
{
	if(opt.music) {
		music_update();
	}
	scr_update();
	scr_draw();

	++nframes;
}

static void change_screen(int idx)
{
	printf("change screen %d\n", idx);
	scr_change(scr_screen(idx), 4000);
}

#define CBUF_SIZE	64
#define CBUF_MASK	(CBUF_SIZE - 1)
void demo_keyboard(int key, int press)
{
	static char cbuf[CBUF_SIZE];
	static int rd, wr;
	char inp[CBUF_SIZE + 1], *dptr;
	int i, nscr;

	if(press) {
		switch(key) {
		case 27:
			demo_quit();
			break;

		case '\n':
		case '\r':
			dptr = inp;
			while(rd != wr) {
				*dptr++ = cbuf[rd];
				rd = (rd + 1) & CBUF_MASK;
			}
			*dptr = 0;
			if(inp[0]) {
				printf("trying to match: %s\n", inp);
				nscr = scr_num_screens();
				for(i=0; i<nscr; i++) {
					if(strstr(scr_screen(i)->name, inp)) {
						change_screen(i);
						break;
					}
				}
			}
			break;

		default:
			if(key >= '1' && key <= '9' && key <= '1' + scr_num_screens()) {
				change_screen(key - '1');
			} else if(key == '0' && scr_num_screens() >= 10) {
				change_screen(9);
			}

			if(key < 256 && isprint(key)) {
				cbuf[wr] = key;
				wr = (wr + 1) & CBUF_MASK;
				if(wr == rd) { /* overflow */
					rd = (rd + 1) & CBUF_MASK;
				}
			}
			break;
		}
	}
}


void mouse_orbit_update(float *theta, float *phi, float *dist)
{
	static int prev_mx, prev_my;
	static unsigned int prev_bmask;

	if(mouse_bmask) {
		if((mouse_bmask ^ prev_bmask) == 0) {
			int dx = mouse_x - prev_mx;
			int dy = mouse_y - prev_my;

			if(dx || dy) {
				if(mouse_bmask & 1) {
					float p = *phi;
					*theta += dx * 1.0;
					p += dy * 1.0;

					if(p < -90) p = -90;
					if(p > 90) p = 90;
					*phi = p;
				}
				if(mouse_bmask & 4) {
					*dist += dy * 0.5;

					if(*dist < 0) *dist = 0;
				}
			}
		}
	}
	prev_mx = mouse_x;
	prev_my = mouse_y;
	prev_bmask = mouse_bmask;
}
