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
static void draw(void);

static struct screen scr = {
	"bump",
	init,
	destroy,
	start,
	0,
	draw
};

struct point {
	int x, y;
};

#define NUM_BIG_LIGHTS 3
#define BIG_LIGHT_WIDTH 256
#define BIG_LIGHT_HEIGHT BIG_LIGHT_WIDTH

static unsigned long startingTime;

static unsigned char *heightmap;
static unsigned short *lightmap;
static int *bumpOffset;

static unsigned short *bigLight[NUM_BIG_LIGHTS];
static struct point bigLightPoint[NUM_BIG_LIGHTS];

struct screen *bump_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j, x, y, c, r, g, b;

	const int numBlurs = 3;
	const int lightRadius = BIG_LIGHT_WIDTH / 2;

	const int bigLightSize = BIG_LIGHT_WIDTH * BIG_LIGHT_HEIGHT;
	const int fb_size = fb_width * fb_height;

	// Just some parameters to temporary test the colors of 3 lights
	// if every light uses it's own channel bits, it's better
	const float rgbMul[9] = { 1.0f, 0.0f, 0.0f, 
								  0.0f, 1.0f, 0.0f,
								  0.0f, 0.0f, 1.0f};
	initFpsFonts();

	heightmap = malloc(sizeof(*heightmap) * fb_size);
	lightmap = malloc(sizeof(*lightmap) * fb_size);
	bumpOffset = malloc(sizeof(*bumpOffset) * fb_size);

	for (i = 0; i < NUM_BIG_LIGHTS; i++)
		bigLight[i] = malloc(sizeof(*bigLight[i]) * bigLightSize);

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

			dx = i < fb_size - 1 ? (int)((heightmap[i] - heightmap[i + 1]) * offsetPower) : 0;
			dy = i < fb_size - fb_width ? (int)((heightmap[i] - heightmap[i + fb_width]) * offsetPower) : 0;

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
	for (y = 0; y < BIG_LIGHT_HEIGHT; y++)
	{
		int yc = y - (BIG_LIGHT_HEIGHT / 2);
		for (x = 0; x < BIG_LIGHT_WIDTH; x++)
		{
			int xc = x - (BIG_LIGHT_WIDTH / 2);
			float d = (float)sqrt(xc * xc + yc * yc);
			float invDist = ((float)lightRadius - (float)sqrt(xc * xc + yc * yc)) / (float)lightRadius;
			if (invDist < 0.0f) invDist = 0.0f;

			c = (int)(invDist * 63);
			r = c >> 1;
			g = c;
			b = c >> 1;

			for (j = 0; j < NUM_BIG_LIGHTS; j++)
			{
				bigLight[j][i] = ((int)(r * rgbMul[j * 3]) << 11) | ((int)(g * rgbMul[j * 3 + 1]) << 5) | (int)(b * rgbMul[j * 3 + 2]);
			}
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

static void eraseArea(struct point *p, int width, int height)
{
	int x, y, dx;
	unsigned short *dst;

	int x0 = p->x;
	int y0 = p->y;
	int x1 = p->x + width;
	int y1 = p->y + height;

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > fb_width) x1 = fb_width;
	if (y1 > fb_height) y1 = fb_height;

	dx = x1 - x0;

	dst = lightmap + y0 * fb_width + x0;

	for (y = y0; y < y1; y++)
	{
		for (x = x0; x < x1; x++)
		{
			*dst++ = 0;
		}
		dst += fb_width - dx;
	}
}


static void renderLight(struct point *p, int width, int height, unsigned short *light)
{
	int x, y, dx;
	unsigned short *src, *dst;

	int x0 = p->x;
	int y0 = p->y;
	int x1 = p->x + width;
	int y1 = p->y + height;

	int xl = 0;
	int yl = 0;

	if (x0 < 0)
	{
		xl = -x0;
		x0 = 0;
	}

	if (y0 < 0)
	{
		yl = -y0;
		y0 = 0;
	}

	if (x1 > fb_width) x1 = fb_width;
	if (y1 > fb_height) y1 = fb_height;

	dx = x1 - x0;

	dst = lightmap + y0 * fb_width + x0;
	src = light + yl * width + xl;

	for (y = y0; y < y1; y++)
	{
		for (x = x0; x < x1; x++)
		{
			*dst++ |= *src++;
		}
		dst += fb_width - dx;
		src += width - dx;
	}
}

static void eraseLights()
{
	int i;
	for (i = 0; i < NUM_BIG_LIGHTS; i++)
		eraseArea(&bigLightPoint[i], BIG_LIGHT_WIDTH, BIG_LIGHT_HEIGHT);
}

static void renderLights()
{
	int i;
	for (i = 0; i < NUM_BIG_LIGHTS; i++)
		renderLight(&bigLightPoint[i], BIG_LIGHT_WIDTH, BIG_LIGHT_HEIGHT, bigLight[i]);
}

static void animateLights()
{
	struct point center;
	float dt = (float)(time_msec - startingTime) / 1000.0f;

	center.x = (fb_width >> 1) - (BIG_LIGHT_WIDTH / 2);
	center.y = (fb_height >> 1) - (BIG_LIGHT_HEIGHT / 2);

	bigLightPoint[0].x = center.x + sin(1.2f * dt) * 144.0f;
	bigLightPoint[0].y = center.y + sin(0.8f * dt) * 148.0f;

	bigLightPoint[1].x = center.x + sin(1.5f * dt) * 156.0f;
	bigLightPoint[1].y = center.y + sin(1.1f * dt) * 92.0f;

	bigLightPoint[2].x = center.x + sin(2.0f * dt) * 112.0f;
	bigLightPoint[2].y = center.y + sin(1.3f * dt) * 136.0f;
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
	eraseLights();
	animateLights();
	renderLights();
	renderBump((unsigned short*)vmem_back);

	drawFps((unsigned short*)vmem_back);

	swap_buffers(0);
}
