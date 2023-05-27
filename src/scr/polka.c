/* Dummy part to test the performance of my mini 3d engine, also occupied for future 3D Polka dots idea */

#include "demo.h"
#include "screen.h"

#include "opt_3d.h"
#include "opt_rend.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>


#define PSIN_SIZE 2048

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static unsigned short *polkaPal;
static unsigned int *polkaPal32;
static unsigned char *polkaBuffer;

static unsigned char *psin1, *psin2, *psin3;

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
	const int tt = t >> 5;

	int countZ = VERTICES_DEPTH;
	do {
		const int p3z = psin3[(3*countZ-tt) & 2047];
		int countY = VERTICES_HEIGHT;
		do {
			const int p2y = psin2[(2*countY+tt) & 2047];
			int countX = VERTICES_WIDTH;
			do {
				unsigned char c = psin1[(2*countX+tt) & 2047] + p2y + p3z;
				if (c < 192) {
					c = 0;
				} else {
					c = (c - 192) << 2;
				}
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


	psin1 = (unsigned char*)malloc(sizeof(unsigned char) * PSIN_SIZE);
	psin2 = (unsigned char*)malloc(sizeof(unsigned char) * PSIN_SIZE);
	psin3 = (unsigned char*)malloc(sizeof(unsigned char) * PSIN_SIZE);

	for (i = 0; i < PSIN_SIZE; i++) {
		const float s = 0.7f;
		const float l = 0.9f;
		psin1[i] = (unsigned char)(sin((l * (double)i) / 7.0) * s*123.0 + s*123.0);
		psin2[i] = (unsigned char)(sin((l * (double)i) / 11.0) * s*176.0 + s*176.0);
		psin3[i] = (unsigned char)(sin((l * (double)i) / 9.0) * s*118.0 + s*118.0);
	}

	updateDotsVolumeBufferPlasma(0);

	return 0;
}

static void destroy(void)
{
	Opt3Dfree();
	freeBlobGfx();

	free(polkaBuffer);
	free(polkaPal);

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
	const int t = time_msec - startingTime;
	
	memset(polkaBuffer, 0, FB_WIDTH * FB_HEIGHT);

	Opt3Drun(polkaBuffer, t);

	buffer8bppToVram(polkaBuffer, polkaPal32);

	updateDotsVolumeBufferPlasma(t);

	swap_buffers(0);
}
