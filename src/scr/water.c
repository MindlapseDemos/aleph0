/* Water effect */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"
#include "opt_rend.h"
#include "gfxutil.h"
#include "noise.h"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

#define FP_SCALE 16

static struct screen scr = {
	"water",
	init,
	destroy,
	start,
	0,
	draw
};

#define CLOUD_TEX_WIDTH 256
#define CLOUD_TEX_HEIGHT 256
#define CLOUD_PERM 7

#define SKY_HEIGHT 4096
#define SKY_PROJ 256

static unsigned long startingTime;

static unsigned short *waterPal;
static unsigned int *waterPal32;

static unsigned char *waterBuffer1;
static unsigned char *waterBuffer2;

unsigned char *wb1, *wb2;

unsigned char *cloudTex;
unsigned short *cloudPal;

unsigned char* skyTex;


static void swapWaterBuffers()
{
	unsigned char *temp = wb2;
	wb2 = wb1;
	wb1 = temp;
}

struct screen *water_screen(void)
{
	return &scr;
}

static void initCloudTex()
{
	int x, y, i, n;

	cloudTex = (unsigned char*)malloc(CLOUD_TEX_WIDTH * CLOUD_TEX_HEIGHT);
	cloudPal = (unsigned short*)malloc(256 * sizeof(unsigned short));

	skyTex = (unsigned char*)malloc(CLOUD_TEX_WIDTH * CLOUD_TEX_HEIGHT);

	i = 0;
	for (y = 0; y < CLOUD_TEX_HEIGHT; ++y) {
		int yp = y;
		for (x = 0; x < CLOUD_TEX_WIDTH; ++x) {
			float sumF = 0.0f;
			int xp = x;
			float d = 1.0f;
			for (n = 0; n < CLOUD_PERM; ++n) {
				float m = 1.0f / (CLOUD_PERM - n);
				int repX = CLOUD_TEX_WIDTH * d;
				int repY = CLOUD_TEX_HEIGHT * d;
				if (repX != 0 && repY != 0) {
					sumF += pnoise2((float)xp * d, (float)yp * d, repX, repY) * m;
				}
				d /= 2.0f;
			}

			sumF += 0.25f;
			CLAMP01(sumF)
			cloudTex[i++] = (unsigned char)(sumF * 127);
		}
	}

	i = 0;
	for (n = 0; n < 255; ++n) {
		int r = n;
		int g = n - 128;
		int b = n + 64;

		CLAMP(r, 0, 255)
		CLAMP(g, 0, 255)
		CLAMP(b, 0, 255)

		cloudPal[i++] = PACK_RGB16(r, g, b);
	}
}

static int init(void)
{
	const int size = FB_WIDTH * FB_HEIGHT;

	waterBuffer1 = (unsigned char*)malloc(size);
	waterBuffer2 = (unsigned char*)malloc(size);
	memset(waterBuffer1, 0, size);
	memset(waterBuffer2, 0, size);

	wb1 = waterBuffer1;
	wb2 = waterBuffer2;

	waterPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);

	setPalGradient(0,127, 0,0,0, 31,63,31, waterPal);
	setPalGradient(128,255, 0,0,0, 0,0,0, waterPal);

	waterPal32 = createColMap16to32(waterPal);

	initCloudTex();

	return 0;
}

static void destroy(void)
{
	free(cloudTex);
	free(cloudPal);
	free(skyTex);
	free(waterBuffer1);
	free(waterBuffer2);
	free(waterPal);
	free(waterPal32);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void waterBufferToVram32()
{
	int i;
	unsigned short *src = (unsigned short*)wb1;
	unsigned int *dst = (unsigned int*)fb_pixels;

	for (i=0; i<FB_WIDTH * FB_HEIGHT / 2; ++i) {
		*dst++ = waterPal32[*src++];
	}
}

#ifdef __WATCOMC__
void updateWaterAsm5(void *buffer1, void *buffer2, void *vramStart);
#else
static void updateWater32(unsigned char *buffer1, unsigned char *buffer2)
{
	int count = (FB_WIDTH / 4) * (FB_HEIGHT - 2) - 2;

	unsigned int *src1 = (unsigned int*)buffer1;
	unsigned int *src2 = (unsigned int*)buffer2;
	unsigned int *vram = (unsigned int*)((unsigned char*)wb1 + FB_WIDTH + 4);

	do {
		const unsigned int c0 = *(unsigned int*)((unsigned char*)src1-1);
		const unsigned int c1 = *(unsigned int*)((unsigned char*)src1+1);
		const unsigned int c2 = *(src1-(FB_WIDTH / 4));
		const unsigned int c3 = *(src1+(FB_WIDTH / 4));

		// Subtract and then absolute value of 4 bytes packed in 8bits (From Hacker's Delight)
		const unsigned int c = (((c0 + c1 + c2 + c3) >> 1) & 0x7f7f7f7f) - *src2;
		const unsigned int a = c & 0x80808080;
		const unsigned int b = a >> 7;
		const unsigned int cc = (c ^ ((a - b) | a)) + b;

		*src2++ = cc;
		src1++;
	} while (--count > 0);
}
#endif

static void renderBlob(int xp, int yp, unsigned char *buffer)
{
	int i,j;
	unsigned char *dst = buffer + yp * FB_WIDTH + xp;

	for (j=0; j<3; ++j) {
		for (i=0; i<3; ++i) {
			*(dst + i) = 0x3f;
		}
		dst += FB_WIDTH;
	}
}

static void makeRipples(unsigned char *buff, int t)
{
	int i;

	renderBlob(32 + (rand() & 255), 32 + (rand() & 127), buff);

	for (i=0; i<3; ++i) {
		const int xp = FB_WIDTH / 2 + (int)(sin((t / 32 + 64*i)/24.0) * 128);
		const int yp = FB_HEIGHT / 2 + (int)(sin((t / 32 + 64*i)/16.0) * 64);

		renderBlob(xp,yp,buff);
	}
}

static void runWaterEffect(int t)
{
	static int prevT = 0;
	const int dt = t - prevT;
	if (dt < 0 || dt > 20) {
		unsigned char* buff1 = wb1 + FB_WIDTH + 4;
		unsigned char* buff2 = wb2 + FB_WIDTH + 4;
		unsigned char* vramOffset = (unsigned char*)buff1 + FB_WIDTH + 4;

		makeRipples(buff1, t);

		#ifdef __WATCOMC__
				updateWaterAsm5(buff1, buff2, vramOffset);
		#else
				updateWater32(buff1, buff2);
		#endif

		waterBufferToVram32();

		swapWaterBuffers();

		prevT = t;
	}
}

static void renderBitmapLineX(int u, int du, int texWidth, int length, unsigned char* src, unsigned short *pal, unsigned short* dst)
{
	while (length-- > 0) {
		int tu = (u >> FP_SCALE) & (texWidth - 1);
		*dst++ = pal[src[tu]];
		u += du;
	};
}

static void blendClouds(int t)
{
	int x, y;
	unsigned char* dst = skyTex;

	for (y = 0; y < CLOUD_TEX_HEIGHT; ++y) {
		int v1 = (y + t) & (CLOUD_TEX_HEIGHT - 1);
		int v2 = (y + t * 2) & (CLOUD_TEX_HEIGHT - 1);
		unsigned char *src1 = &cloudTex[v1 * CLOUD_TEX_WIDTH];
		unsigned char *src2 = &cloudTex[v2 * CLOUD_TEX_WIDTH];
		for (x = 0; x < CLOUD_TEX_WIDTH; ++x) {
			*dst++ = *src1++ + *src2++;
		}
	}
}

static void testBlitCloudTex()
{
	unsigned short* dst = (unsigned short*)fb_pixels;
	int y;
	for (y = 0; y < FB_HEIGHT/2; ++y) {
		unsigned char *src;
		int z, u, v, du;

		int yp = FB_HEIGHT / 2 - y;
		if (yp == 0) yp = 1;
		z = (SKY_HEIGHT * SKY_PROJ) / yp;

		du = z * 2;
		u = (-FB_WIDTH / 2) * du;

		v = (z >> 8) & (CLOUD_TEX_HEIGHT - 1);
		src = &skyTex[v * CLOUD_TEX_WIDTH];

		renderBitmapLineX(u, du, CLOUD_TEX_WIDTH, FB_WIDTH, src, cloudPal, dst);

		dst += FB_WIDTH;
	}
}

static void draw(void)
{
	const int t = time_msec - startingTime;

	runWaterEffect(t);

	blendClouds(t >> 4);
	testBlitCloudTex();

	swap_buffers(0);
}
