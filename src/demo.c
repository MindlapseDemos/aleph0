#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "demo.h"
#include "screen.h"

int fb_width = 320;
int fb_height = 240;
int fb_bpp = 16;
void *fb_pixels;
unsigned long time_msec;
int mouse_x, mouse_y;
unsigned int mouse_bmask;

static unsigned long nframes;
static const char *start_scr_name = "tunnel";

int demo_init(int argc, char **argv)
{
	if(argv[1]) {
		start_scr_name = argv[1];
	}

	if(scr_init() == -1) {
		return -1;
	}
	if(scr_change(scr_lookup(start_scr_name), 4000) == -1) {
		fprintf(stderr, "screen %s not found\n", start_scr_name);
		return -1;
	}

	return 0;
}

void demo_cleanup(void)
{
	scr_shutdown();

	if(time_msec) {
		float fps = (float)nframes / ((float)time_msec / 1000.0f);
		printf("average framerate: %.1f\n", fps);
	}
}

void demo_draw(void)
{
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
			break;
		}
	}
}
