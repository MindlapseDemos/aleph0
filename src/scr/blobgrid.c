/* Blobgrid effect idea */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"

#define NUM_POINTS 128
#define FP_PT 8

static int *xp;
static int *yp;

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"blobgrid",
	init,
	destroy,
	start,
	0,
	draw
};

static unsigned long startingTime;


static unsigned short *blobsPal;


struct screen *blobgrid_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i;

	blobsPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);
	xp = (int*)malloc(sizeof(int) * NUM_POINTS);
	yp = (int*)malloc(sizeof(int) * NUM_POINTS);

	for (i=0; i<128; i++) {
		int r = i >> 2;
		int g = i >> 1;
		int b = i >> 1;
		if (b > 31) b = 31;
		blobsPal[i] = (r<<11) | (g<<5) | b;
	}
	for (i=128; i<256; i++) {
		int r = 31 - ((i-128) >> 4);
		int g = 63 - ((i-128) >> 2);
		int b = 31 - ((i-128) >> 2);
		blobsPal[i] = (r<<11) | (g<<5) | b;
	}
	return 0;
}

static void destroy(void)
{
	free(blobsPal);
	free(xp);
	free(yp);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void drawBlob(int posX, int posY, int size, int color)
{
	int i,j;
	unsigned short *dst = (unsigned short*)fb_pixels + posY * FB_WIDTH + posX;

	for (j=0; j<size; ++j) {
		for (i=0; i<size; ++i) {
			*(dst + i) = blobsPal[color];
		}
		dst += FB_WIDTH;
	}
}

static void drawConnections()
{
	int i,j,k;
	const int breaks = 16;
	const int max = 512;
	const int bp = max / breaks;

	for (j=0; j<NUM_POINTS; ++j) {
		for (i=0; i<NUM_POINTS; ++i) {
			if (i!=j) {
				const int lx = xp[i] - xp[j];
				const int ly = yp[i] - yp[j];
				const int dst = lx*lx + ly*ly;

				if (dst >= bp && dst < max) {
					const int steps = dst / bp;
					int px = xp[i] << FP_PT;
					int py = yp[i] << FP_PT;
					const int dx = ((xp[j] - xp[i]) << FP_PT) / steps;
					const int dy = ((yp[j] - yp[i]) << FP_PT) / steps;

					for (k=0; k<steps-1; ++k) {
						px += dx;
						py += dy;
						drawBlob(px>>FP_PT, py>>FP_PT,2,63);
					}
				}
			}
		}
	}
}

static void drawPoints(int t)
{
	int i;

	for (i=0; i<NUM_POINTS; ++i) {
		xp[i] = FB_WIDTH / 2 + (int)(sin((t + 9768*i)/7924.0) * 148);
		yp[i] = FB_HEIGHT / 2 + (int)(sin((t + 7024*i)/13838.0) * 96);

		drawBlob(xp[i],yp[i],3,191);
	}
}

static void draw(void)
{
	const int t = time_msec - startingTime;

	memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * 2);

	drawPoints(t);
	drawConnections();

	swap_buffers(0);
}
