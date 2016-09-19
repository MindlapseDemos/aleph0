#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include "demo.h"
#include "screen.h"
#include "3dgfx.h"
#include "music.h"

int fb_width = 320;
int fb_height = 240;
int fb_bpp = 16;
uint16_t *fb_pixels, *vmem_back, *vmem_front;
unsigned long time_msec;
int mouse_x, mouse_y;
unsigned int mouse_bmask;

static unsigned long nframes;
static const char *start_scr_name;

int demo_init(int argc, char **argv)
{
	struct screen *scr;

	start_scr_name = getenv("START_SCR");
	if(argv[1]) {
		start_scr_name = argv[1];
	}

	if(g3d_init() == -1) {
		return -1;
	}
	g3d_framebuffer(fb_width, fb_height, fb_pixels);

	if(music_open("data/test.mod") == -1) {
		return -1;
	}

	if(scr_init() == -1) {
		return -1;
	}
	if(start_scr_name) {
		scr = scr_lookup(start_scr_name);
	} else {
		scr = scr_screen(0);
	}

	if(!scr || scr_change(scr, 4000) == -1) {
		fprintf(stderr, "screen %s not found\n", start_scr_name ? start_scr_name : "0");
		return -1;
	}

	/* clear the framebuffer at least once */
	memset(fb_pixels, 0, fb_width * fb_height * fb_bpp / CHAR_BIT);

	music_play();
	return 0;
}

void demo_cleanup(void)
{
	music_close();
	scr_shutdown();
	g3d_destroy();

	if(time_msec) {
		float fps = (float)nframes / ((float)time_msec / 1000.0f);
		printf("average framerate: %.1f\n", fps);
	}
}

void demo_draw(void)
{
	music_update();
	scr_update();
	scr_draw();

	++nframes;
}

void demo_keyboard(int key, int state)
{
	if(state) {
		switch(key) {
		case 27:
			demo_quit();
			break;

		default:
			if(key >= '1' && key <= '1' + scr_num_screens()) {
				int idx = key - '1';
				printf("change screen %d\n", idx);
				scr_change(scr_screen(idx), 4000);
			}
			break;
		}
	}
}
