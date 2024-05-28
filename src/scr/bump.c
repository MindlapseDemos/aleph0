/* Bump effect (not moving yet of course, I have many ideas on this to commit before it's ready) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"
#include "imago2.h"

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

#define NUM_PARTICLES 64
#define PARTICLE_LIGHT_WIDTH 32
#define PARTICLE_LIGHT_HEIGHT 32


static unsigned long startingTime;

static unsigned char *heightmap;
static unsigned short *lightmap;
static int *bumpOffset;

static unsigned short *bigLight[NUM_BIG_LIGHTS];
static struct point bigLightPoint[NUM_BIG_LIGHTS];

static unsigned short *particleLight;
static struct point particlePoint[NUM_PARTICLES];


struct screen *bump_screen(void)
{
	return &scr;
}

static void generateHeightmapTest()
{
	const int numBlurs = 2;
	const int fb_size = FB_WIDTH * FB_HEIGHT;

	int i,j;

	/* Create random junk */
	for (i = 0; i < fb_size; i++) {
		heightmap[i] = rand() & 255;
	}

	/* Blur to smooth */
	for (j = 0; j < numBlurs; j++) {
		for (i = 0; i < fb_size; i++) {
			heightmap[i] = (heightmap[abs((i - 1) % fb_size)] + heightmap[abs((i + 1) % fb_size)] + heightmap[abs((i - FB_WIDTH) % fb_size)] + heightmap[abs((i + FB_WIDTH) % fb_size)]) >> 2;
		}
	}
}

static int loadHeightMapTest()
{
	static struct img_pixmap bumpPic;

	int x, y, i;
	uint32_t* src;
	int imgWidth, imgHeight;

	img_init(&bumpPic);
	if (img_load(&bumpPic, "data/bump/circ_col.png") == -1) {
		fprintf(stderr, "failed to load bump image\n");
		return -1;
	}

	src = (uint32_t*)bumpPic.pixels;
	imgWidth = bumpPic.width;
	imgHeight = bumpPic.width;

	i = 0;
	for (y = 0; y < FB_HEIGHT; ++y) {
		for (x = 0; x < FB_WIDTH; ++x) {
			uint32_t c32 = src[((y & (imgHeight - 1)) * imgWidth) + (x & (imgWidth - 1))];
			int a = (c32 >> 24) & 255;
			int r = (c32 >> 16) & 255;
			int g = (c32 >> 8) & 255;
			int b = c32 & 255;
			int c = (a + r + g + b) / 4;
			heightmap[i++] = c;
		}
	}

	return 0;
}

static int init(void)
{
	int i, j, x, y, c, r, g, b;

	const int fb_size = FB_WIDTH * FB_HEIGHT;

	const int lightRadius = BIG_LIGHT_WIDTH / 2;
	const int particleRadius = PARTICLE_LIGHT_WIDTH / 2;

	const int bigLightSize = BIG_LIGHT_WIDTH * BIG_LIGHT_HEIGHT;
	const int particleLightSize = PARTICLE_LIGHT_WIDTH * PARTICLE_LIGHT_HEIGHT;

	/* Just some parameters to temporary test the colors of 3 lights
	 * if every light uses it's own channel bits, it's better
	 */
	const float rgbMul[9] = { 0.75, 0,    0,
							  0,    0.75, 0,
							  0,    0,    0.75 };

	heightmap = malloc(sizeof(*heightmap) * fb_size);
	lightmap = malloc(sizeof(*lightmap) * fb_size);
	bumpOffset = malloc(sizeof(*bumpOffset) * fb_size);

	for (i = 0; i < NUM_BIG_LIGHTS; i++)
		bigLight[i] = malloc(sizeof(*bigLight[i]) * bigLightSize);

	particleLight = malloc(sizeof(*particleLight) * particleLightSize);

	memset(lightmap, 0, sizeof(*lightmap) * fb_size);
	memset(bumpOffset, 0, sizeof(*bumpOffset) * fb_size);
	memset(particlePoint, 0, sizeof(*particlePoint) * NUM_PARTICLES);

	/* generateHeightmapTest(); */
	if (loadHeightMapTest() == -1) return - 1;


	/* Inclination precalculation */
	i = 0;
	for (y = 0; y < FB_HEIGHT; y++) {
		for (x = 0; x < FB_WIDTH; x++) {
			const float offsetPower = 0.75f;
			int dx, dy, xp, yp;

			dx = i < fb_size - 1 ? (int)((heightmap[i] - heightmap[i + 1]) * offsetPower) : 0;
			dy = i < fb_size - FB_WIDTH ? (int)((heightmap[i] - heightmap[i + FB_WIDTH]) * offsetPower) : 0;

			xp = x + dx;
			if (xp < 0) xp = 0;
			if (xp > FB_WIDTH - 1) xp = FB_WIDTH - 1;

			yp = y + dy;
			if (yp < 0) yp = 0;
			if (yp > FB_HEIGHT - 1) yp = FB_HEIGHT - 1;

			bumpOffset[i++] = yp * FB_WIDTH + xp;
		}
	}

	/* Generate three lights */
	i = 0;
	for (y = 0; y < BIG_LIGHT_HEIGHT; y++) {
		int yc = y - (BIG_LIGHT_HEIGHT / 2);
		for (x = 0; x < BIG_LIGHT_WIDTH; x++) {
			int xc = x - (BIG_LIGHT_WIDTH / 2);
			float d = (float)sqrt(xc * xc + yc * yc);
			float invDist = ((float)lightRadius - (float)sqrt(xc * xc + yc * yc)) / (float)lightRadius;
			if (invDist < 0.0f) invDist = 0.0f;

			c = (int)(invDist * 63);
			r = c >> 1;
			g = c;
			b = c >> 1;

			for (j = 0; j < NUM_BIG_LIGHTS; j++) {
				bigLight[j][i] = ((int)(r * rgbMul[j * 3]) << 11) | ((int)(g * rgbMul[j * 3 + 1]) << 5) | (int)(b * rgbMul[j * 3 + 2]);
			}
			i++;
		}
	}

	i = 0;
	for (y = 0; y < PARTICLE_LIGHT_HEIGHT; y++) {
		int yc = y - (PARTICLE_LIGHT_HEIGHT / 2);
		for (x = 0; x < PARTICLE_LIGHT_WIDTH; x++) {
			int xc = x - (PARTICLE_LIGHT_WIDTH / 2);

			float invDist = ((float)particleRadius - (float)sqrt(xc * xc + yc * yc)) / (float)particleRadius;
			if (invDist < 0.0f) invDist = 0.0f;

			c = (int)(pow(invDist, 0.75f) * 31);
			particleLight[i++] = ((c >> 1) << 11) | (c << 5) | (c >> 1);
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
	int y, dx;
	unsigned short *dst;

	int x0 = p->x;
	int y0 = p->y;
	int x1 = p->x + width;
	int y1 = p->y + height;

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > FB_WIDTH) x1 = FB_WIDTH;
	if (y1 > FB_HEIGHT) y1 = FB_HEIGHT;

	dx = x1 - x0;

	dst = lightmap + y0 * FB_WIDTH + x0;

	for (y = y0; y < y1; y++) {
		memset(dst, 0, 2*dx);
		dst += FB_WIDTH;
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

	if (x0 < 0) {
		xl = -x0;
		x0 = 0;
	}

	if (y0 < 0) {
		yl = -y0;
		y0 = 0;
	}

	if (x1 > FB_WIDTH) x1 = FB_WIDTH;
	if (y1 > FB_HEIGHT) y1 = FB_HEIGHT;

	dx = x1 - x0;

	dst = lightmap + y0 * FB_WIDTH + x0;
	src = light + yl * width + xl;

	for (y = y0; y < y1; y++) {
		for (x = x0; x < x1; x++) {
			*dst++ |= *src++;
		}
		dst += FB_WIDTH - dx;
		src += width - dx;
	}
}

static void eraseLights()
{
	int i;
	for (i = 0; i < NUM_BIG_LIGHTS; i++) {
		eraseArea(&bigLightPoint[i], BIG_LIGHT_WIDTH, BIG_LIGHT_HEIGHT);
	}
}

static void renderLights()
{
	int i;
	for (i = 0; i < NUM_BIG_LIGHTS; i++) {
		renderLight(&bigLightPoint[i], BIG_LIGHT_WIDTH, BIG_LIGHT_HEIGHT, bigLight[i]);
	}
}

static void renderParticles()
{
	int i;
	for (i = 0; i < NUM_PARTICLES; i++) {
		renderLight(&particlePoint[i], PARTICLE_LIGHT_WIDTH, PARTICLE_LIGHT_HEIGHT, particleLight);
	}
}

static void animateLights()
{
	struct point center;
	float dt = (float)(time_msec - startingTime) / 1000.0f;
	float tt = 1.0f - sin(dt);
	float disperse = tt * 16.0f;

	center.x = (FB_WIDTH >> 1) - (BIG_LIGHT_WIDTH / 2);
	center.y = (FB_HEIGHT >> 1) - (BIG_LIGHT_HEIGHT / 2);

	bigLightPoint[0].x = center.x + sin(1.2f * dt) * (96.0f + disperse);
	bigLightPoint[0].y = center.y + sin(0.8f * dt) * (64.0f + disperse);

	bigLightPoint[1].x = center.x + sin(1.5f * dt) * (80.0f + disperse);
	bigLightPoint[1].y = center.y + sin(1.1f * dt) * (48.0f + disperse);

	bigLightPoint[2].x = center.x + sin(2.0f * dt) * (72.0f + disperse);
	bigLightPoint[2].y = center.y + sin(1.3f * dt) * (56.0f + disperse);
}

static void renderBump(unsigned short *vram)
{
	int i;
	for (i = 0; i < FB_WIDTH * FB_HEIGHT; i++) {
		unsigned short c = lightmap[bumpOffset[i]];
		*vram++ = c;
	}
}

static void animateParticles()
{
	int i;
	struct point center;
	float dt = (float)(time_msec - startingTime) / 2000.0f;
	float tt = sin(dt);

	center.x = (FB_WIDTH >> 1) - (PARTICLE_LIGHT_WIDTH / 2);
	center.y = (FB_HEIGHT >> 1) - (PARTICLE_LIGHT_HEIGHT / 2);

	for (i = 0; i < NUM_PARTICLES; i++) {
		particlePoint[i].x = center.x + (sin(1.2f * (i*i*i + dt)) * 74.0f + sin(0.6f * (i + dt)) * 144.0f) * tt;
		particlePoint[i].y = center.y + (sin(1.8f * (i + dt)) * 68.0f + sin(0.5f * (i*i + dt)) * 132.0f) * tt;
	}
}

static void draw(void)
{
	memset(lightmap, 0, FB_WIDTH * FB_HEIGHT * sizeof(*lightmap));

	animateLights();
	renderLights();

	/* animateParticles();
	renderParticles(); */

	renderBump((unsigned short*)fb_pixels);

	swap_buffers(0);
}
