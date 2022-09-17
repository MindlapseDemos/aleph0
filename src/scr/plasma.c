/* Just a test with a run of the mill plasma */

#include <stdlib.h>
#include <math.h>

#include "demo.h"
#include "screen.h"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"plasma",
	init,
	destroy,
	start,
	0,
	draw
};

static unsigned long startingTime;

#define PSIN_SIZE 4096
#define PPAL_SIZE 256

static unsigned char *psin1, *psin2, *psin3;
static unsigned short *plasmaPal;

static unsigned short myBuffer[320 * 240];


struct screen *plasma_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i;

	psin1 = (unsigned char*)malloc(sizeof(unsigned char) * PSIN_SIZE);
	psin2 = (unsigned char*)malloc(sizeof(unsigned char) * PSIN_SIZE);
	psin3 = (unsigned char*)malloc(sizeof(unsigned char) * PSIN_SIZE);

	for (i = 0; i < PSIN_SIZE; i++)
	{
		psin1[i] = (unsigned char)(sin((double)i / 45.0) * 63.0 + 63.0);
		psin2[i] = (unsigned char)(sin((double)i / 75.0) * 42.0 + 42.0);
		psin3[i] = (unsigned char)(sin((double)i / 32.0) * 88.0 + 88.0);
	}

	plasmaPal = (unsigned short*)malloc(sizeof(unsigned short) * PPAL_SIZE);
	for (i=0; i<PPAL_SIZE; i++)
	{
		int r, g, b;
		int c = i;
		if (c > 127) c = 255 - c;
		c >>= 2;
		g = 31 - c;
		r = c;
		b = g;
		plasmaPal[i] = (r<<11) | (g<<5) | b;
	}

	return 0;
}

static void destroy(void)
{
	free(psin1);
	free(psin2);
	free(psin3);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void draw(void)
{
	int x, y;
	unsigned char c;

	float dt = (float)(time_msec - startingTime) / 1000.0f;
	const int t1 = sin(0.1f * dt) * 132 + 132;
	const int t2 = sin(0.2f * dt) * 248 + 248;
	const int t3 = sin(0.5f * dt) * 380 + 380;

	unsigned char *psin1_a = (unsigned char*)&psin1[t1];
	unsigned char *psin2_a = (unsigned char*)&psin2[t2];
	unsigned int *vram32 = (unsigned int*)fb_pixels;
	for (y = 0; y < FB_HEIGHT; y++)
	{
		const unsigned char s1 = psin2[y + t2];
		unsigned char *psin1_b = (unsigned char*)&psin1[psin3[y + t1]+t3];
		unsigned char *psin3_a = (unsigned char*)&psin3[y+t3];
		for (x = 0; x < FB_WIDTH; x+=8)
		{
			unsigned int p0, p1;
	
			c = psin1_a[x] + s1 + psin3_a[x] + psin1_b[psin2_a[x]];
			p0 = plasmaPal[c];
			c = psin1_a[x + 1] + s1 + psin3_a[x + 1] + psin1_b[psin2_a[x + 1]];
			p1 = plasmaPal[c];
			*vram32++ = (p1 << 16) | p0;

			c = psin1_a[x + 2] + s1 + psin3_a[x + 2] + psin1_b[psin2_a[x + 2]];
			p0 = plasmaPal[c];
			c = psin1_a[x + 3] + s1 + psin3_a[x + 3] + psin1_b[psin2_a[x + 3]];
			p1 = plasmaPal[c];
			*vram32++ = (p1 << 16) | p0;

			c = psin1_a[x + 4] + s1 + psin3_a[x + 4] + psin1_b[psin2_a[x + 4]];
			p0 = plasmaPal[c];
			c = psin1_a[x + 5] + s1 + psin3_a[x + 5] + psin1_b[psin2_a[x + 5]];
			p1 = plasmaPal[c];
			*vram32++ = (p1 << 16) | p0;

			c = psin1_a[x + 6] + s1 + psin3_a[x + 6] + psin1_b[psin2_a[x + 6]];
			p0 = plasmaPal[c];
			c = psin1_a[x + 7] + s1 + psin3_a[x + 7] + psin1_b[psin2_a[x + 7]];
			p1 = plasmaPal[c];
			*vram32++ = (p1 << 16) | p0;
		}
	}

	swap_buffers(0);
}
