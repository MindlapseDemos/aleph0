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

#define PSIN_SIZE 2048
#define PSIN_OFFSETS 4

static unsigned char *psin1, *psin2, *psin3;
static unsigned short *plasmaPal;
static unsigned int *plasmaPal32;

static unsigned char *plasmaBuffer;

struct screen *plasma_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i,j,k;

	plasmaBuffer = (unsigned char*)malloc(FB_WIDTH * FB_HEIGHT);

	psin1 = (unsigned char*)malloc(sizeof(unsigned char) * PSIN_SIZE * PSIN_OFFSETS);
	psin2 = (unsigned char*)malloc(sizeof(unsigned char) * PSIN_SIZE * PSIN_OFFSETS);
	psin3 = (unsigned char*)malloc(sizeof(unsigned char) * PSIN_SIZE * PSIN_OFFSETS);

	for (j = 0; j < PSIN_OFFSETS; j++) {
		for (i = 0; i < PSIN_SIZE; i++) {
			psin1[i + j * PSIN_SIZE] = (unsigned char)(sin((double)(i + j) / 45.0) * 63.0 + 63.0);
			psin2[i + j * PSIN_SIZE] = (unsigned char)(sin((double)(i + j) / 75.0) * 42.0 + 42.0);
			psin3[i + j * PSIN_SIZE] = (unsigned char)(sin((double)(i + j) / 32.0) * 88.0 + 88.0);
		}
	}

	plasmaPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);
	plasmaPal32 = (unsigned int*)malloc(sizeof(unsigned int) * 256 * 256);

	for (i=0; i<256; i++)
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

	k = 0;
	for (j=0; j<256; ++j) {
		const int c0 = plasmaPal[j];
		for (i=0; i<256; ++i) {
			const int c1 = plasmaPal[i];
			plasmaPal32[k++] = (c0 << 16) | c1;
		}
	}

	return 0;
}

static void destroy(void)
{
	free(plasmaBuffer);
	free(plasmaPal);
	free(plasmaPal32);

	free(psin1);
	free(psin2);
	free(psin3);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void plasmaBufferToVram32()
{
	int i;
	unsigned short *src = (unsigned short*)plasmaBuffer;
	unsigned int *dst = (unsigned int*)fb_pixels;

	for (i=0; i<FB_WIDTH * FB_HEIGHT / 2; ++i) {
		*dst++ = plasmaPal32[*src++];
	}
}

#define PSIN32(x) ((((x) & 3) * PSIN_SIZE) + ((x) & ~3))
#define VAL32(x) (((x) << 24) | ((x) << 16) | ((x) << 8) | (x))

static void draw(void)
{
	int x, y;
	unsigned int c;

	float dt = (float)(time_msec - startingTime) / 1000.0f;
	const int t1 = sin(0.1f * dt) * 132 + 132;
	const int t2 = sin(0.2f * dt) * 248 + 248;
	const int t3 = sin(0.5f * dt) * 380 + 380;

	unsigned int *psin1_a = (unsigned int*)&psin1[PSIN32(t1)];
	unsigned int *psin2_a = (unsigned int*)&psin2[PSIN32(t2)];
	unsigned int *dst = (unsigned int*)plasmaBuffer;

	for (y = 0; y < FB_HEIGHT; y++)
	{
		const unsigned int p2 = psin2[PSIN32(y+t2)];
		const int s1 = psin2[y + t2];
		const unsigned int ss1 = VAL32(s1);


		const int pp13 = psin3[y + t1]+t3;
		unsigned int *psin1_b = (unsigned int*)&psin1[PSIN32(pp13)];

		unsigned int *psin3_a = (unsigned int*)&psin3[PSIN32(y+t3)];
		for (x = 0; x < FB_WIDTH/4; x+=8) {
			*dst++ = psin1_a[x] + ss1 + psin3_a[x] + psin1_b[x];
			*dst++ = psin1_a[x+1] + ss1 + psin3_a[x+1] + psin1_b[x+1];
			*dst++ = psin1_a[x+2] + ss1 + psin3_a[x+2] + psin1_b[x+2];
			*dst++ = psin1_a[x+3] + ss1 + psin3_a[x+3] + psin1_b[x+3];
			*dst++ = psin1_a[x+4] + ss1 + psin3_a[x+4] + psin1_b[x+4];
			*dst++ = psin1_a[x+5] + ss1 + psin3_a[x+5] + psin1_b[x+5];
			*dst++ = psin1_a[x+6] + ss1 + psin3_a[x+6] + psin1_b[x+6];
			*dst++ = psin1_a[x+7] + ss1 + psin3_a[x+7] + psin1_b[x+7];
		}
	}

	plasmaBufferToVram32();

	swap_buffers(0);
}
