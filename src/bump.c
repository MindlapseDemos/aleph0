// Bump effect (not moving yet of course, I have many ideas on this to commit before it's ready)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"
#include "tinyfps.h"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void stop(long trans_time);
static void draw(void);

static struct screen scr = {
	"bump",
	init,
	destroy,
	start,
	stop,
	draw
};

static unsigned long startingTime;

static unsigned char *heightmap;
static unsigned short *lightmap;
static int *bumpOffset;

struct screen *bump_screen(void)
{
	return &scr;
}

static int init(void)
{
	const int numBlurs = 3;
	int i, j, x, y;
	int fb_size = fb_width * fb_height;

	initFpsFonts();

	heightmap = malloc(sizeof(*heightmap) * fb_size);
	lightmap = malloc(sizeof(*lightmap) * fb_size);
	bumpOffset = malloc(sizeof(*bumpOffset) * fb_size);

	memset(lightmap, 0, fb_size);
	memset(bumpOffset, 0, fb_size);

	// Create random junk
	for (i = 0; i < fb_size; i++)
		heightmap[i] = rand() & 255;

	// Blur to smooth
	for (j = 0; j < numBlurs; j++)
		for (i = 0; i < fb_size; i++)
			heightmap[i] = (heightmap[abs((i - 1) % fb_size)] + heightmap[abs((i + 1) % fb_size)] + heightmap[abs((i - fb_width) % fb_size)] + heightmap[abs((i + fb_width) % fb_size)]) >> 2;

	// Inclination precalculation
	i = 0;
	for (y = 0; y < fb_height; y++)
	{
		for (x = 0; x < fb_width; x++)
		{
			const float offsetPower = 2.0f;
			int dx, dy, xp, yp;

			dx = (int)((heightmap[i] - heightmap[i + 1]) * offsetPower);
			dy = (int)((heightmap[i] - heightmap[i + fb_width]) * offsetPower);

			xp = x + dx;
			if (xp < 0) xp = 0;
			if (xp > fb_width - 1) xp = fb_width - 1;

			yp = y + dy;
			if (yp < 0) yp = 0;
			if (yp > fb_height - 1) yp = fb_height - 1;

			bumpOffset[i++] = yp * fb_width + xp;
		}
	}

	// Lightmap test (this will be replaced by code in the main loop later on, the idea is you can render moving lights and other light geometry or sprites in light buffer and the bumpOffset will catch them)
	i = 0;
	for (y = 0; y < fb_height; y++)
	{
		int yc = y - (fb_height >> 1);
		for (x = 0; x < fb_width; x++)
		{
			int xc = x - (fb_width >> 1);
			int c = (int)sqrt(xc * xc + yc * yc) << 1;
			int r;
			if (c > 255) c = 255;
			r = 255 - c;
			lightmap[i++] = ((r >> 4) << 11) | ((r >> 3) << 5) | (r >> 3);
		}
	}

	return 0;
}

static void destroy(void)
{
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void stop(long trans_time)
{
}

static void draw(void)
{
	int i;
	unsigned short *vram = (unsigned short*)fb_pixels;
	for (i = 0; i < fb_width * fb_height; i++)
	{
		unsigned short c = lightmap[bumpOffset[i]];
		*vram++ = c;
	}

	drawFps((unsigned short*)fb_pixels);

	swap_buffers(0);
}
