#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include "demo.h"
#include "screen.h"

int fb_width = 320;
int fb_height = 240;
int fb_bpp = 16;
void *fb_pixels;
unsigned long time_msec;

static unsigned long nframes;

int demo_init(int argc, char **argv)
{
	if(scr_init() == -1) {
		return -1;
	}
	scr_change(scr_lookup("tunnel"), 4000);

	/* clear the framebuffer at least once */
	memset(fb_pixels, 0, fb_width * fb_height * fb_bpp / CHAR_BIT);
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
