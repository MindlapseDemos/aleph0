// Just a test with a run of the mill plasma

#include <stdlib.h>
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
	"plasma",
	init,
	destroy,
	start,
	stop,
	draw
};

unsigned long startingTime;

#define PSIN_SIZE 4096
#define PPAL_SIZE 256

unsigned char *psin1, *psin2, *psin3;
unsigned short *plasmaPal;

unsigned short myBuffer[320*240];


struct screen *plasma_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i;

	initFpsFonts();

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

	//return 0xCAFE;
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

static void stop(long trans_time)
{
}

static void draw(void)
{
	int x, y;
	unsigned char c;
	unsigned char s1, s2;

	float dt = (float)(time_msec - startingTime) / 1000.0f;
	int t1 = sin(0.1f * dt) * 132 + 132;
	int t2 = sin(0.2f * dt) * 248 + 248;
	int t3 = sin(0.5f * dt) * 380 + 380;

	unsigned int *vram32 = (unsigned int*)vmem_back;
	unsigned int p0, p1;
	for (y = 0; y < fb_height; y++)
	{
		s1 = psin2[y + t2];
		s2 = psin3[y + t1];
		for (x = 0; x < fb_width; x+=2)
		{
			c = psin1[x + t1] + s1 + psin3[x + y + t3] + psin1[psin2[x + t2] + s2 + t3];
			p0 = plasmaPal[c];
			c = psin1[x + 1 + t1] + s1 + psin3[x + 1 + y + t3] + psin1[psin2[x + 1 + t2] + s2 + t3];
			p1 = plasmaPal[c];

			*vram32++ = (p1 << 16) | p0;
		}
	}

	drawFps((unsigned short*)fb_pixels);

	swap_buffers(0);
}
