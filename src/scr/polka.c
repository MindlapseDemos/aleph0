/* Dummy part to test the performance of my mini 3d engine, also occupied for future 3D Polka dots idea */

#include "demo.h"
#include "screen.h"

#include "opt_3d.h"
#include "opt_rend.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static unsigned short *polkaPal;
static unsigned int *polkaPal32;
static unsigned char *polkaBuffer;

static struct screen scr = {
	"polka",
	init,
	destroy,
	start,
	0,
	draw
};

static unsigned long startingTime;

struct screen *polka_screen(void)
{
	return &scr;
}


static void updateDotsVolumeBufferPlasma(int t)
{
	unsigned char *dst = getDotsVolumeBuffer();
	const float tt = t * 0.001f;

	int countZ = VERTICES_DEPTH;
	do {
		int countY = VERTICES_HEIGHT;
		do {
			int countX = VERTICES_WIDTH;
			do {
				unsigned char c = 0;
				float n = fmod(fabs(sin((float)countX/8.0f + tt) + sin((float)countY/6.0f + 2.0f * tt) + sin((float)countZ/7.0f - tt)), 1.0f);
				if (n > 0.9f) c = 1;

				*dst++ = c;
			} while(--countX != 0);
		} while(--countY != 0);
	} while(--countZ != 0);
}

static int init(void)
{
	int i;

	Opt3Dinit();
	initBlobGfx();

	polkaBuffer = (unsigned char*)malloc(FB_WIDTH * FB_HEIGHT);
	polkaPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);

	for (i=0; i<128; i++) {
		int r = i >> 2;
		int g = i >> 1;
		int b = i >> 1;
		if (b > 31) b = 31;
		polkaPal[i] = (r<<11) | (g<<5) | b;
	}
	for (i=128; i<256; i++) {
		int r = 31 - ((i-128) >> 4);
		int g = 63 - ((i-128) >> 2);
		int b = 31 - ((i-128) >> 3);
		polkaPal[i] = (r<<11) | (g<<5) | b;
	}

	polkaPal32 = createColMap16to32(polkaPal);

	updateDotsVolumeBufferPlasma(0);

	return 0;
}

static void destroy(void)
{
	Opt3Dfree();
	freeBlobGfx();

	free(polkaBuffer);
	free(polkaPal);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void draw(void)
{
	const int t = time_msec - startingTime;

	Opt3Drun(t);

	swap_buffers(0);
}
