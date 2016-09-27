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

static struct point {
	int x, y;
};

#define LIGHT_WIDTH 128
#define LIGHT_HEIGHT LIGHT_WIDTH

static unsigned long startingTime;

static unsigned char *heightmap;
static unsigned short *lightmap;
static int *bumpOffset;

static unsigned short *lightR;
static unsigned short *lightG;
static unsigned short *lightB;
static struct point pointR, pointG, pointB;

//#define FUNKY_COLORS


struct screen *bump_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j, x, y, c, r, g, b;

	const int numBlurs = 3;
	const int lightRadius = LIGHT_WIDTH / 2;

	const int lightSize = LIGHT_WIDTH * LIGHT_HEIGHT;
	const int fb_size = fb_width * fb_height;

	// Just some parameters to temporary test the colors of 3 lights
	#ifdef FUNKY_COLORS
		// I see some artifacts if I mix channels, not sure if ORing is fine
		const float r0 = 1.0f, g0 = 0.6f, b0 = 0.0f;
		const float r1 = 0.5f, g1 = 1.0f, b1 = 0.2f;
		const float r2 = 0.6f, g2 = 0.4f, b2 = 1.0f;
	#else
		// if every light uses it's own channel bits, it's better
		const float r0 = 1.0f, g0 = 0.0f, b0 = 0.0f;
		const float r1 = 0.0f, g1 = 1.0f, b1 = 0.0f;
		const float r2 = 0.0f, g2 = 0.0f, b2 = 1.0f;
	#endif

	initFpsFonts();

	heightmap = malloc(sizeof(*heightmap) * fb_size);
	lightmap = malloc(sizeof(*lightmap) * fb_size);
	bumpOffset = malloc(sizeof(*bumpOffset) * fb_size);

	lightR = malloc(sizeof(*lightR) * lightSize);
	lightG = malloc(sizeof(*lightG) * lightSize);
	lightB = malloc(sizeof(*lightB) * lightSize);

	memset(lightmap, 0, sizeof(*lightmap) * fb_size);
	memset(bumpOffset, 0, sizeof(*bumpOffset) * fb_size);

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

	// Generate three lights
	i = 0;
	for (y = 0; y < LIGHT_HEIGHT; y++)
	{
		int yc = y - (LIGHT_HEIGHT / 2);
		for (x = 0; x < LIGHT_WIDTH; x++)
		{
			int xc = x - (LIGHT_WIDTH / 2);
			float d = (float)sqrt(xc * xc + yc * yc);
			float invDist = ((float)lightRadius - (float)sqrt(xc * xc + yc * yc)) / (float)lightRadius;
			if (invDist < 0.0f) invDist = 0.0f;

			c = (int)(invDist * 63);
			r = c >> 1;
			g = c;
			b = c >> 1;

			lightR[i] = ((int)(r * r0) << 11) | ((int)(g * g0) << 5) | (int)(b * b0);
			lightG[i] = ((int)(r * r1) << 11) | ((int)(g * g1) << 5) | (int)(b * b1);
			lightB[i] = ((int)(r * r2) << 11) | ((int)(g * g2) << 5) | (int)(b * b2);
			i++;
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


static void renderLight(struct point *p, unsigned short *light)
{
	// Check for boundaries is missing atm, will add soon
	int x, y;
	unsigned short *dst = (unsigned short*)lightmap + p->y * fb_width + p->x;
	for (y = 0; y < LIGHT_HEIGHT; y++)
	{
		for (x = 0; x < LIGHT_WIDTH; x++)
		{
			*dst++ |= *light++;
		}
		dst += fb_width - LIGHT_WIDTH;
	}
}


static void renderLights()
{
	memset(lightmap, 0, fb_width * fb_height * sizeof(*lightmap));
	// I will later delete only the regions of lights to speed up

	renderLight(&pointR, lightR);
	renderLight(&pointG, lightG);
	renderLight(&pointB, lightB);
}

static void animateLights()
{
	struct point center;
	float dt = (float)(time_msec - startingTime) / 1000.0f;

	center.x = (fb_width >> 1) - (LIGHT_WIDTH / 2);
	center.y = (fb_height >> 1) - (LIGHT_HEIGHT / 2);

	pointR.x = center.x + sin(1.2f * dt) * 64.0f;
	pointR.y = center.y + sin(0.8f * dt) * 48.0f;

	pointG.x = center.x + sin(1.5f * dt) * 56.0f;
	pointG.y = center.y + sin(1.1f * dt) * 42.0f;

	pointB.x = center.x + sin(2.0f * dt) * 80.0f;
	pointB.y = center.y + sin(1.3f * dt) * 46.0f;
}

static void renderBump(unsigned short *vram)
{
	int i;
	for (i = 0; i < fb_width * fb_height; i++)
	{
		unsigned short c = lightmap[bumpOffset[i]];
		*vram++ = c;
	}
}

static void draw(void)
{
	animateLights();
	renderLights();
	renderBump((unsigned short*)vmem_back);

	drawFps((unsigned short*)vmem_back);

	swap_buffers(0);
}
