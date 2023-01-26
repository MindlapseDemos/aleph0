/* Julia Tunnel idea */

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "demo.h"
#include "screen.h"


#define JULIA_LAYERS 8
#define JULIA_COLORS 16
#define JULIA_PAL_TRANSITION_BITS 4
#define JULIA_PAL_TRANSITIONS (1 << JULIA_PAL_TRANSITION_BITS)
#define JULIA_PAL_COLORS (JULIA_COLORS * JULIA_LAYERS * JULIA_PAL_TRANSITIONS)

#define JULIA_ANIM_REPEAT_BITS 11
#define JULIA_ANIM_REPEAT (1 << JULIA_ANIM_REPEAT_BITS)

#define JULIA_QUARTER_WIDTH (FB_WIDTH / 2)
#define JULIA_QUARTER_HEIGHT (FB_HEIGHT / 2)

#define FP_SHR 12
#define FP_MUL (1 << FP_SHR)
#define DI_BITS 8

#define ESCAPE ((4 * FP_MUL) << FP_SHR)

#define FRAC_ITER_BLOCK \
						x1 = ((x0 * x0 - y0 * y0)>>FP_SHR) + xp; \
						y1 = ((x0 * y0)>>(FP_SHR-1)) + yp; \
						mj2 = x1 * x1 + y1 * y1; \
						x0 = x1; y0 = y1; c++; \
						if (mj2 > ESCAPE) goto endr1;

#define FRAC_ITER_BLOCX \
						x1 = ((x0 * x0 - y0 * y0)>>FP_SHR) + xp; \
						y1 = ((x0 * y0)>>(FP_SHR-1)) + yp; \
						mj2 = x1 * x1 + y1 * y1; \
						x0 = x1; y0 = y1; c++; \
						if (mj2 > ESCAPE) goto end1;


#define FRAC_ITER_TIMES_2 FRAC_ITER_BLOCX FRAC_ITER_BLOCX
#define FRAC_ITER_TIMES_4 FRAC_ITER_TIMES_2 FRAC_ITER_TIMES_2
#define FRAC_ITER_TIMES_6 FRAC_ITER_TIMES_4 FRAC_ITER_TIMES_2
#define FRAC_ITER_TIMES_8 FRAC_ITER_TIMES_4 FRAC_ITER_TIMES_4
#define FRAC_ITER_TIMES_10 FRAC_ITER_TIMES_8 FRAC_ITER_TIMES_2
#define FRAC_ITER_TIMES_12 FRAC_ITER_TIMES_8 FRAC_ITER_TIMES_4
#define FRAC_ITER_TIMES_14 FRAC_ITER_TIMES_12 FRAC_ITER_TIMES_2
#define FRAC_ITER_TIMES_16 FRAC_ITER_TIMES_8 FRAC_ITER_TIMES_8

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"juliatunnel",
	init,
	destroy,
	start,
	0,
	draw
};

static unsigned long startingTime;

static unsigned short *juliaTunnelPal;
static unsigned int *juliaTunnelPal32;

static int xp_l[JULIA_LAYERS];
static int yp_l[JULIA_LAYERS];

unsigned char *juliaQuarterBuffer;

struct screen *juliatunnel_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i,j,k,n=0;

	float sr = 0.0f;
	float sg = 0.5f;
	float sb = 1.0f;

	const float dr = (1.0f - sr) / JULIA_LAYERS;
	const float dg = (0.6f - sg) / JULIA_LAYERS;
	const float db = (0.2f - sb) / JULIA_LAYERS;

	const int juliaQuarterSize = JULIA_QUARTER_WIDTH * ((JULIA_QUARTER_HEIGHT / 2) + 1);
	juliaQuarterBuffer = (unsigned char*)malloc(juliaQuarterSize);
	memset(juliaQuarterBuffer, 0, juliaQuarterSize);

	juliaTunnelPal = (unsigned short*)malloc(sizeof(unsigned short) * JULIA_PAL_COLORS);
	juliaTunnelPal32 = (unsigned int*)malloc(sizeof(unsigned int) * JULIA_PAL_COLORS);

	for (k=0; k<JULIA_PAL_TRANSITIONS; ++k) {
		float d = (float)k / JULIA_PAL_TRANSITIONS;
		float cr = sr * (1.0f - d) + (sr + dr) * d;
		float cg = sg * (1.0f - d) + (sg + dg) * d;
		float cb = sb * (1.0f - d) + (sb + db) * d;
		for (j=0; j<JULIA_LAYERS; ++j) {
			for (i=0; i<JULIA_COLORS; ++i) {
				int r,g,b;
				int c = abs(JULIA_COLORS / 2 - 1 - i);
				if (c >= JULIA_COLORS / 2) c = JULIA_COLORS / 2 - 1;
				c = JULIA_COLORS / 2 - c - 1;

				r = (int)((c << 2) * cr);
				g = (int)((c << 3) * cg);
				b = (int)((c << 2) * cb);

				c = (r<<11) | (g<<5) | b;
				juliaTunnelPal[n] = c;
				juliaTunnelPal32[n] = (c << 16) | c;
				++n;
			}
			cr += dr;
			cg += dg;
			cb += db;
		}
	}

	return 0;
}

static void destroy(void)
{
	free(juliaTunnelPal);
	free(juliaTunnelPal32);
	free(juliaQuarterBuffer);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

/*static unsigned char renderJuliaPixelRec(int xk, int yk, int layer_iter)
{
	int x1,y1,mj2;

	int x0 = xk;
	int y0 = yk;
	unsigned char c = 255;

	const int xp = xp_l[layer_iter-1];
	const int yp = yp_l[layer_iter-1];

	FRAC_ITER_TIMES_16

	endr1:
	
	if (c>=15 && --layer_iter != 0) return renderJuliaPixel(xk<<1, yk<<1, layer_iter);

	return c + (layer_iter - 1) * JULIA_COLORS;
}*/

static unsigned char renderJuliaPixel(int xk, int yk, int layer_iter)
{
	unsigned char c;
	int x0, y0;
	int x1, y1, mj2;
	int xp, yp;

	--layer_iter; xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk; y0 = yk;
	FRAC_ITER_TIMES_14

	--layer_iter; xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk<<1; y0 = yk<<1;
	FRAC_ITER_TIMES_12

	--layer_iter; xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk<<2; y0 = yk<<2;
	FRAC_ITER_TIMES_10

	--layer_iter; xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk<<3; y0 = yk<<3;
	FRAC_ITER_TIMES_8

	--layer_iter; xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk<<4; y0 = yk<<4;
	FRAC_ITER_TIMES_6

	--layer_iter; xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk<<5; y0 = yk<<5;
	FRAC_ITER_TIMES_4

	--layer_iter; xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk<<6; y0 = yk<<6;
	FRAC_ITER_TIMES_2

	return 0;
	end1:
	
	return c + layer_iter * JULIA_COLORS;
}

/*static void renderJulia(float scale, int palAnimOffset)
{
	int x, y;

	const int screenWidthHalf = FB_WIDTH / 2;
	const int screenHeightHalf = FB_HEIGHT / 2;

	const int di = (int)((FP_MUL << DI_BITS) / (screenHeightHalf * scale)) / 2;

	unsigned short *vramUp = (unsigned short*)fb_pixels;
	unsigned short *vramDown = vramUp + FB_WIDTH * FB_HEIGHT - 1;

	unsigned short *pal = &juliaTunnelPal[JULIA_COLORS * JULIA_LAYERS * palAnimOffset];

	int yk = -di * -screenHeightHalf;
	int xl = di * -screenWidthHalf;

	for (y=0; y<screenHeightHalf; y++)
	{
		int xk = xl;
		for (x=0; x<FB_WIDTH; x++)
		{
			const int xki = xk >> DI_BITS;
			const int yki = yk >> DI_BITS;
			const unsigned char c = renderJuliaPixel(xki, yki, JULIA_LAYERS);
			const unsigned short cc = pal[c];

			*vramUp++ = cc;
			*vramDown-- = cc;

			xk+=di;
		}
        yk-=di;
	}
}*/

static void calcJuliaQuarter(float scale, int palAnimOffset)
{
	int x, y;

	const int screenWidthHalf = FB_WIDTH / 2;
	const int screenHeightHalf = FB_HEIGHT / 2;

	unsigned char *dst = juliaQuarterBuffer;

	int di = (int)((FP_MUL << DI_BITS) / (screenHeightHalf * scale)) / 2;
	const int xl = di * -screenWidthHalf;
	int yk = -di * -screenHeightHalf;

	di *= 2;
	for (y=0; y<JULIA_QUARTER_HEIGHT/2+1; ++y)
	{
		int xk = xl;
		for (x=0; x<JULIA_QUARTER_WIDTH; ++x)
		{
			const int xki = xk >> DI_BITS;
			const int yki = yk >> DI_BITS;

			*dst++ = renderJuliaPixel(xki, yki, JULIA_LAYERS);

			xk+=di;
		}
        yk-=di;
	}
}

/*static void renderJuliaQuarter(float scale, int palAnimOffset)
{
	int x, y;

	const int screenWidthHalf = FB_WIDTH / 2;
	const int screenHeightHalf = FB_HEIGHT / 2;

	unsigned char *src = juliaQuarterBuffer;
	unsigned short *vramUp = (unsigned short*)fb_pixels;
	unsigned short *vramDown = vramUp + FB_WIDTH * (FB_HEIGHT - 1) - 2;
	unsigned short *pal = &juliaTunnelPal[JULIA_COLORS * JULIA_LAYERS * palAnimOffset];

	const int di = (int)((FP_MUL << DI_BITS) / (screenHeightHalf * scale)) / 2;
	const int xl = di * -screenWidthHalf;
	int yk = -di * -screenHeightHalf;

	for (y=0; y<JULIA_QUARTER_HEIGHT/2; ++y)
	{
		int xk = xl;
		for (x=0; x<JULIA_QUARTER_WIDTH; ++x)
		{
			unsigned short *src16 = (unsigned short*)src;
			const unsigned short c0 = *src16;
			const unsigned short c1 = *(src16 + (JULIA_QUARTER_WIDTH / 2));

			unsigned short cc = pal[c0 & 255];
			*vramUp = cc;
			*(vramDown+FB_WIDTH+1) = cc;
			if (c0==c1) {
				*(vramUp+1) = cc;
				*(vramUp+FB_WIDTH) = cc;
				*(vramUp+FB_WIDTH+1) = cc;
				*vramDown = cc;
				*(vramDown+1) = cc;
				*(vramDown+FB_WIDTH) = cc;
			} else {
				const int xki = xk >> DI_BITS;
				const int xki1 = (xk+di) >> DI_BITS;
				const int yki = yk >> DI_BITS;
				const int yki1 = (yk-di) >> DI_BITS;

				cc = pal[renderJuliaPixel(xki1, yki, JULIA_LAYERS)];
				*(vramUp+1) = cc;
				*(vramDown+FB_WIDTH) = cc;

				cc = pal[renderJuliaPixel(xki, yki1, JULIA_LAYERS)];
				*(vramUp+FB_WIDTH) = cc;
				*(vramDown+1) = cc;

				cc = pal[renderJuliaPixel(xki1, yki1, JULIA_LAYERS)];
				*(vramUp+FB_WIDTH+1) = cc;
				*vramDown = cc;
			}

			src++;
			vramUp += 2;
			vramDown -= 2;

			xk+=2*di;
		}
		vramUp += FB_WIDTH;
		vramDown -= FB_WIDTH;
        yk-=2*di;
	}
}*/

static void renderJuliaQuarter(float scale, int palAnimOffset)
{
	int x, y;

	const int screenWidthHalf = FB_WIDTH / 2;
	const int screenHeightHalf = FB_HEIGHT / 2;

	unsigned char *src = juliaQuarterBuffer;
	unsigned short *vramUp = (unsigned short*)fb_pixels;
	unsigned short *vramDown = vramUp + FB_WIDTH * (FB_HEIGHT - 1) - 2;

	unsigned int *vramUp32 = (unsigned int*)vramUp;
	unsigned int *vramDown32 = (unsigned int*)vramDown;
	unsigned int *pal32 = &juliaTunnelPal32[JULIA_COLORS * JULIA_LAYERS * palAnimOffset];

	const int di = (int)((FP_MUL << DI_BITS) / (screenHeightHalf * scale)) / 2;
	const int xl = di * -screenWidthHalf;
	int yk = -di * -screenHeightHalf;

	for (y=0; y<JULIA_QUARTER_HEIGHT/2; ++y)
	{
		int xk = xl;
		for (x=0; x<JULIA_QUARTER_WIDTH; ++x)
		{
			unsigned short *src16 = (unsigned short*)src;
			const unsigned short c0 = *src16;
			const unsigned short c1 = *(src16 + (JULIA_QUARTER_WIDTH / 2));

			unsigned int cc = pal32[c0 & 255];
			*vramUp32 = cc;
			*(vramDown32+FB_WIDTH/2) = cc;
			if (c0==c1) {
				*(vramUp32+FB_WIDTH/2) = cc;
				*vramDown32 = cc;
			} else {
				const int xki = xk >> DI_BITS;
				const int yki1 = (yk-di) >> DI_BITS;

				cc = pal32[renderJuliaPixel(xki, yki1, JULIA_LAYERS)];
				*(vramUp32+FB_WIDTH/2) = cc;
				*(vramDown32) = cc;
			}

			src++;
			vramUp32++;
			vramDown32--;

			xk+=2*di;
		}
		vramUp32 += FB_WIDTH/2;
		vramDown32 -= FB_WIDTH/2;
        yk-=2*di;
	}
}

static void draw(void)
{
	int i;

	//const int t = 2200;
	//const int layerAdv = 0;

	const int t = time_msec - startingTime;
	const int layerAdv = t >> JULIA_ANIM_REPEAT_BITS;
	const int palAnimOffset = (t >> (JULIA_ANIM_REPEAT_BITS - JULIA_PAL_TRANSITION_BITS)) & (JULIA_PAL_TRANSITIONS - 1);

	const float ts = (float)((t & (JULIA_ANIM_REPEAT-1)) / (float)(JULIA_ANIM_REPEAT-1));
	const float scale = 1.0f + pow(ts, 1.15);	// hmm

	for (i=0; i<JULIA_LAYERS; ++i) {
		const int ti = (i - layerAdv) << 9;
		xp_l[i] = (int)(sin((t + ti)/812.0) * FP_MUL) / 3;
		yp_l[i] = (int)(sin((t + ti)/1482.0) * FP_MUL) / 2;
		//xp_l[i] = 1280;
		//yp_l[i] = 1280;
	}

	calcJuliaQuarter(scale, palAnimOffset);
	renderJuliaQuarter(scale, palAnimOffset);

	//renderJulia(scale, palAnimOffset);

	swap_buffers(0);
}
