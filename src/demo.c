#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "demo.h"

int fbwidth = 320;
int fbheight = 240;
int fbbpp = 8;
unsigned char *fbpixels;
unsigned long time_msec;

static unsigned long nframes;

int demo_init(int argc, char **argv)
{
	return 0;
}

void demo_cleanup(void)
{
	if(time_msec) {
		float fps = (float)nframes / ((float)time_msec / 1000.0f);
		printf("average framerate: %.1f\n", fps);
	}
}

void demo_draw(void)
{
	int i, j;
	unsigned char *fbptr = fbpixels;

	for(i=0; i<fbheight; i++) {
		for(j=0; j<fbwidth; j++) {
			int val = i^j;

			*fbptr++ = val;
		}
	}

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
