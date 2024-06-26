#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"
#include "imago2.h"
#include "opt_rend.h"

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

typedef struct Point2D {
	int x, y;
} Point2D;

typedef struct Edge {
	short xIn, xOut;
} Edge;

#define NUM_BIG_LIGHTS 3
#define BIG_LIGHT_WIDTH 256
#define BIG_LIGHT_HEIGHT BIG_LIGHT_WIDTH

#define NUM_PARTICLES 64
#define PARTICLE_LIGHT_WIDTH 16
#define PARTICLE_LIGHT_HEIGHT 16

#define HMAP_WIDTH 256
#define HMAP_HEIGHT 256

#define LMAP_WIDTH 512
#define LMAP_HEIGHT 512
#define LMAP_OFFSET_X ((LMAP_WIDTH - FB_WIDTH) / 2)
#define LMAP_OFFSET_Y ((LMAP_HEIGHT - FB_HEIGHT) / 2)


static unsigned long startingTime;

static unsigned char *heightmap;
static unsigned short *lightmap;
static int *bumpOffset;
static int *bumpOffsetScreen;

static unsigned short *bigLight[NUM_BIG_LIGHTS];
static Point2D bigLightPoint[NUM_BIG_LIGHTS];
static Edge *bigLightEdges;

static unsigned short *particleLight;
static Point2D particlePoint[NUM_PARTICLES];

static int lightMinX = 0;
static int lightMaxX = LMAP_WIDTH-1;
static int lightMinY = 0;
static int lightMaxY = LMAP_HEIGHT-1;



struct screen *bump_screen(void)
{
	return &scr;
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
	for (y = 0; y < HMAP_HEIGHT; ++y) {
		for (x = 0; x < HMAP_WIDTH; ++x) {
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

	const int hm_size = HMAP_WIDTH * HMAP_HEIGHT;
	const int lm_size = LMAP_WIDTH * LMAP_HEIGHT;

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

	heightmap = malloc(sizeof(*heightmap) * hm_size);
	lightmap = malloc(sizeof(*lightmap) * lm_size);
	bumpOffset = malloc(sizeof(*bumpOffset) * hm_size);
	bumpOffsetScreen = malloc(sizeof(*bumpOffset) * FB_WIDTH * FB_HEIGHT);

	bigLightEdges = (Edge*)malloc(BIG_LIGHT_HEIGHT * sizeof(*bigLightEdges));

	for (i = 0; i < NUM_BIG_LIGHTS; i++) {
		bigLight[i] = malloc(sizeof(*bigLight[i]) * bigLightSize);
	}
	particleLight = malloc(sizeof(*particleLight) * particleLightSize);

	memset(lightmap, 0, sizeof(*lightmap) * lm_size);
	memset(bumpOffset, 0, sizeof(*bumpOffset) * hm_size);
	memset(particlePoint, 0, sizeof(*particlePoint) * NUM_PARTICLES);

	if (loadHeightMapTest() == -1) return - 1;


	/* Inclination precalculation */
	i = 0;
	for (y = 0; y < HMAP_HEIGHT; y++) {
		const int yp0 = y * HMAP_WIDTH;
		const int yp1 = ((y+1) & (HMAP_HEIGHT-1)) * HMAP_WIDTH;
		for (x = 0; x < HMAP_WIDTH; x++) {
			const int ii = yp0 + x;
			const int iRight = yp0 + ((x + 1) & (HMAP_WIDTH - 1));
			const int iDown = yp1 +x;
			const float offsetPower = 0.75f;
			int dx, dy, di;

			dx = (int)((heightmap[ii] - heightmap[iRight]) * offsetPower);
			dy = (int)((heightmap[ii] - heightmap[iDown]) * offsetPower);
			di = dy * LMAP_WIDTH + dx;

			bumpOffset[i++] = di;
		}
	}

	/* Generate three lights */
	i = 0;
	for (y = 0; y < BIG_LIGHT_HEIGHT; y++) {
		int yc = y - (BIG_LIGHT_HEIGHT / 2);
		int xIn = -1;
		int xOut = -1;
		int lastC = 0;
		for (x = 0; x < BIG_LIGHT_WIDTH; x++) {
			int xc = x - (BIG_LIGHT_WIDTH / 2);
			float d = (float)sqrt(xc * xc + yc * yc);
			float invDist = ((float)lightRadius - (float)sqrt(xc * xc + yc * yc)) / (float)lightRadius;
			if (invDist < 0.0f) invDist = 0.0f;

			c = (int)(invDist * 63);
			r = c >> 1;
			g = c;
			b = c >> 1;

			if (lastC == 0 && c > 0) {
				xIn = x;
			}
			if (lastC != 0 && c == 0) {
				xOut = x;
			}
			lastC = c;

			for (j = 0; j < NUM_BIG_LIGHTS; j++) {
				bigLight[j][i] = ((int)(r * rgbMul[j * 3]) << 11) | ((int)(g * rgbMul[j * 3 + 1]) << 5) | (int)(b * rgbMul[j * 3 + 2]);
			}
			i++;
		}
		bigLightEdges[y].xIn = xIn;
		bigLightEdges[y].xOut = xOut;
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
	int i;

	free(heightmap);
	free(lightmap);
	free(bumpOffset);
	free(bumpOffsetScreen);
	free(particleLight);
	free(bigLightEdges);

	for (i=0; i<NUM_BIG_LIGHTS; ++i) {
		free(bigLight[i]);
	}
}

static void start(long trans_time)
{
	startingTime = time_msec;
}


static void renderParticleLight(Point2D *p, int width, int height, unsigned short *light)
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

	dst = lightmap + (y0 + LMAP_OFFSET_Y) * LMAP_WIDTH + x0 + LMAP_OFFSET_X;
	src = light + yl * width + xl;

	for (y = y0; y < y1; y++) {
		for (x = x0; x < x1; x++) {
			*dst++ |= *src++;
		}
		dst += LMAP_WIDTH - dx;
		src += width - dx;
	}
}

static void renderParticles()
{
	int i;
	for (i = 0; i < NUM_PARTICLES; i++) {
		renderParticleLight(&particlePoint[i], PARTICLE_LIGHT_WIDTH, PARTICLE_LIGHT_HEIGHT, particleLight);
	}
}

static void renderBigLights()
{
	int i;
	for (i = 0; i < NUM_BIG_LIGHTS; i++) {
		int y;
		unsigned short* src;
		unsigned short* dst;

		Point2D* p = &bigLightPoint[i];

		int x0 = p->x;
		int y0 = p->y;
		int x1 = p->x + BIG_LIGHT_WIDTH;
		int y1 = p->y + BIG_LIGHT_HEIGHT;

		//int xl = 0;
		//int yl = 0;

		/*if (y0 < 0) {
			yl = -y0;
			y0 = 0;
		}

		if (y1 > FB_HEIGHT) y1 = FB_HEIGHT;*/

		dst = lightmap + (y0 + LMAP_OFFSET_Y) * LMAP_WIDTH + x0 + LMAP_OFFSET_X;
		src = bigLight[i];// +yl * BIG_LIGHT_WIDTH + xl;

		if (i==0) {
			int yLine = 0;// yl;
			for (y = y0; y < y1; y++) {
				const short xIn = bigLightEdges[yLine].xIn;
				if (xIn != -1) {
					const short xOut = bigLightEdges[yLine].xOut;
					memcpy(dst + xIn, src + xIn, 2 * (xOut - xIn));
				}
				dst += LMAP_WIDTH;
				src += BIG_LIGHT_WIDTH;
				yLine++;
			}
		} else {
			int yLine = 0;// yl;
			for (y = y0; y < y1; y++) {
				const short xIn = bigLightEdges[yLine].xIn;
				if (xIn != -1) {
					const short xOut = bigLightEdges[yLine].xOut;

					uint16_t* srcIn = src + xIn;
					uint16_t* dstIn = dst + xIn;

					uint32_t* src32, * dst32;
					int count;

					int xp0 = x0 + xIn;
					int xp1 = x0 + xOut - 1;

					if (xp0 & 1) {
						*dstIn++ |= *srcIn++;
						++xp0;
					}

					src32 = (uint32_t*)srcIn;
					dst32 = (uint32_t*)dstIn;
					count = (xp1 - xp0) >> 1;
					while (count-- != 0) {
						*dst32++ |= *src32++;
					};

					srcIn = (uint16_t*)src32;
					dstIn = (uint16_t*)dst32;
					if (xp1 & 1) {
						*dstIn++ |= *srcIn++;
					}
				}
				dst += LMAP_WIDTH;
				src += BIG_LIGHT_WIDTH;
				yLine++;
			}
		}
	}
}

static void animateBigLights()
{
	int i;
	Point2D center;
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



	lightMinX = LMAP_WIDTH-1;
	lightMaxX = 0;
	lightMinY = LMAP_HEIGHT-1;
	lightMaxY = 0;

	for (i = 0; i < NUM_BIG_LIGHTS; ++i) {
		Point2D* p = &bigLightPoint[i];
		int x0, x1, y0, y1;

		x0 = p->x + LMAP_OFFSET_X;
		x1 = p->x + LMAP_OFFSET_X + BIG_LIGHT_WIDTH;
		y0 = p->y + LMAP_OFFSET_Y;
		y1 = p->y + LMAP_OFFSET_Y + BIG_LIGHT_HEIGHT;

		if (x0 < lightMinX) lightMinX = x0;
		if (x1 > lightMaxX) lightMaxX = x1;
		if (y0 < lightMinY) lightMinY = y0;
		if (y1 > lightMaxY) lightMaxY = y1;
	}
}


#define SAFE_OFF_Y_TOP 60
#define SAFE_OFF_Y_BOTTOM 60

static void renderBump(unsigned short *vram)
{
	int x,y;

	int* bumpSrc = bumpOffsetScreen;
	for (y = 0; y < FB_HEIGHT; ++y) {
		unsigned short* lightSrc = &lightmap[(y+LMAP_OFFSET_Y) * LMAP_WIDTH + LMAP_OFFSET_X];
		for (x = 0; x < FB_WIDTH; ++x) {
			*vram++ = lightSrc[x + *bumpSrc++];
		}
	}
}

static void animateParticles()
{
	int i;
	Point2D center;
	float dt = (float)(time_msec - startingTime) / 2000.0f;
	float tt = 0.25f * sin(dt) + 0.75f;

	center.x = (FB_WIDTH >> 1) - (PARTICLE_LIGHT_WIDTH / 2);
	center.y = (FB_HEIGHT >> 1) - (PARTICLE_LIGHT_HEIGHT / 2);

	for (i = 0; i < NUM_PARTICLES; i++) {
		particlePoint[i].x = center.x + (sin(1.2f * (i*i*i + dt)) * 74.0f + sin(0.6f * (i + dt)) * 144.0f) * tt;
		particlePoint[i].y = center.y + (sin(1.8f * (i + dt)) * 68.0f + sin(0.5f * (i*i + dt)) * 132.0f) * tt;
	}
}

static void eraseLightmapArea(int x0, int y0, int x1, int y1)
{
	if (x1 >= x0) {
		const int size = (x1 - x0 + 1) * 2;

		int y;
		for (y = y0; y <= y1; ++y) {
			memset(&lightmap[y * LMAP_WIDTH + x0], 0, size);
		}
	}
}

static void moveBumpOffset(int t)
{
	const int tt = t >> 5;
	int y;
	for (y = 0; y < FB_HEIGHT; ++y) {
		const int yp = (y + tt) & (HMAP_HEIGHT - 1);

		memcpy(&bumpOffsetScreen[y * FB_WIDTH], &bumpOffset[yp * HMAP_WIDTH], HMAP_WIDTH * sizeof(*bumpOffset));
		memcpy(&bumpOffsetScreen[y * FB_WIDTH + HMAP_WIDTH], &bumpOffset[yp * HMAP_WIDTH], (FB_WIDTH - HMAP_WIDTH) * sizeof(*bumpOffset));
	}
}

static void draw(void)
{
	const int t = time_msec - startingTime;

	eraseLightmapArea(lightMinX, lightMinY, lightMaxX, lightMaxY);

	animateBigLights();
	renderBigLights();

	/* animateParticles();
	renderParticles(); */

	moveBumpOffset(t);

	renderBump((unsigned short*)fb_pixels);

	swap_buffers(0);
}
