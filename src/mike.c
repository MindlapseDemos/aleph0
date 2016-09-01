#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "imago2.h"
#include "demo.h"
#include "screen.h"

#define BG_FILENAME "data/grise.png"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void stop(long trans_time);
static void draw(void);

static void convert32To16(unsigned int *src32, unsigned short *dst16, unsigned int pixelCount);

static unsigned short *background = 0;
static unsigned int backgroundW = 0;
static unsigned int backgroundH = 0;

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
	if (!(background = img_load_pixels(BG_FILENAME, &backgroundW, &backgroundH, IMG_FMT_RGBA32))) {
		fprintf(stderr, "failed to load image " BG_FILENAME "\n");
		return -1;
	}

	/* Convert to 16bpp */
	convert32To16((unsigned int*)background, background, backgroundW * backgroundH);

	return 0;
}

static void destroy(void)
{
	img_free_pixels(background);
}

static void start(long trans_time)
{

}

static void stop(long trans_time)
{
}

static void draw(void)
{	
	unsigned short *pixels = fb_pixels;

	int j, i;
	for (j = 0; j < fb_height; j++) {
		for (i = 0; i < fb_width; i++) {
			*pixels++ = 0x0000;
		}
	}

	memcpy(fb_pixels, background, backgroundW * backgroundH * 2);
}

/* src and dst can be the same */
static void convert32To16(unsigned int *src32, unsigned short *dst16, unsigned int pixelCount) {
	unsigned int p;
	while (pixelCount) {
		p = *src32++;
		*dst16++ =	((p << 8) & 0xF800)		/* R */
			|		((p >> 5) & 0x07E0)		/* G */
			|		((p >> 19) & 0x001F);	/* B */
		pixelCount--;
	}
}


