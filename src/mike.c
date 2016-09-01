#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "imago2.h"
#include "demo.h"
#include "screen.h"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void stop(long trans_time);
static void draw(void);

static struct screen scr = {
	"mike",
	init,
	destroy,
	start,
	stop,
	draw
};

struct screen *mike_screen(void)
{
	return &scr;
}


static int init(void)
{
	return 0;
}

static void destroy(void)
{
}

static void start(long trans_time)
{
	if(trans_time) {
		
	}
}

static void stop(long trans_time)
{
	if(trans_time) {

	}
}

static void draw(void)
{	
	unsigned short *pixels = fb_pixels;

	int j, i;
	for (j = 0; j < fb_height; j++) {
		for (i = 0; i < fb_width; i++) {
			*pixels++ = 0xF800;
		}
	}
}


