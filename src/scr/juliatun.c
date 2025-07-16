/* Julia Tunnel idea */

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "demo.h"
#include "screen.h"
#include "opt_rend.h"


/* Select half res (160x200) instead of full res (320x200) */
/* #define HALF_WIDTH_RES */

#ifdef HALF_WIDTH_RES
	#define FX_WIDTH (FB_WIDTH / 2)
#else
	#define FX_WIDTH FB_WIDTH
#endif
#define FX_HEIGHT FB_HEIGHT

/* Hack values to fix the nonmatching line in the middle */
#define DI_OFF_X 1
#define DI_OFF_Y -1
#define DI_SHIFT_X 1
#define DI_SHIFT_Y 1

#define JULIA_LAYERS 8
#define JULIA_COLORS 16
#define JULIA_PAL_TRANSITION_BITS 4
#define JULIA_PAL_TRANSITIONS (1 << JULIA_PAL_TRANSITION_BITS)
#define JULIA_PAL_COLORS (JULIA_COLORS * JULIA_LAYERS * JULIA_PAL_TRANSITIONS)

#define JULIA_ANIM_REPEAT_BITS 11
#define JULIA_ANIM_REPEAT (1 << JULIA_ANIM_REPEAT_BITS)

#define JULIA_QUARTER_WIDTH ((FX_WIDTH / 2) + 2)
#define JULIA_QUARTER_HEIGHT ((FX_HEIGHT / 2) + 2)

#define FP_SHR 12
#define FP_MUL (1 << FP_SHR)
#define DI_BITS 8

#define ESCAPE ((4 * FP_MUL) << FP_SHR)

#define FRAC_ITER_BLOCK \
						x1 = ((x0 * x0 - y0 * y0)>>FP_SHR) + xp; \
						y1 = ((x0 * y0)>>(FP_SHR-1)) + yp; \
						mj2 = x1 * x1 + y1 * y1; \
						x0 = x1; y0 = y1; c++; \
						if (mj2 > ESCAPE) break;


#define FRAC_ITER_TIMES_2 FRAC_ITER_BLOCK FRAC_ITER_BLOCK
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

static dseq_event *ev_fade;


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

	const float dr = (0.7f - sr) / JULIA_LAYERS;
	const float dg = (0.3f - sg) / JULIA_LAYERS;
	const float db = (0.9f - sb) / JULIA_LAYERS;

	const int juliaQuarterSize = JULIA_QUARTER_WIDTH * JULIA_QUARTER_HEIGHT;
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

	ev_fade = dseq_lookup("juliatunnel.fade");

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

static unsigned char calcJuliaPixel(int xk, int yk, int layer_iter)
{
	unsigned char c;
	int x0, y0;
	int x1, y1, mj2;
	int xp, yp;

	switch(layer_iter) {

		case JULIA_LAYERS-1:
			xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk; y0 = yk;
			FRAC_ITER_TIMES_14
			--layer_iter;

		case JULIA_LAYERS-2:
			xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk<<1; y0 = yk<<1;
			FRAC_ITER_TIMES_12
			--layer_iter;

		case JULIA_LAYERS-3:
			xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk<<2; y0 = yk<<2;
			FRAC_ITER_TIMES_10
			--layer_iter;

		case JULIA_LAYERS-4:
			xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk<<3; y0 = yk<<3;
			FRAC_ITER_TIMES_8
			--layer_iter;

		case JULIA_LAYERS-5:
			xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk<<4; y0 = yk<<4;
			FRAC_ITER_TIMES_6
			--layer_iter;

		case JULIA_LAYERS-6:
			xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk<<5; y0 = yk<<5;
			FRAC_ITER_TIMES_4
			--layer_iter;

		case JULIA_LAYERS-7:
			xp = xp_l[layer_iter]; yp = yp_l[layer_iter]; c = 255; x0 = xk<<6; y0 = yk<<6;
			FRAC_ITER_TIMES_2
			--layer_iter;

		case JULIA_LAYERS-8:
			return 0;
	}

	return c | (layer_iter << 4);
}


#define HACK_4x2 /* 30-40% increase with hard to notice artifacts */

#ifdef HACK_4x2
static void calcJuliaQuarter(float scale, int palAnimOffset)
{
	int x, y;

	const int screenWidthHalf = FX_WIDTH / 2;
	const int screenHeightHalf = FX_HEIGHT / 2;

	unsigned char *dst = juliaQuarterBuffer;

	int di = (int)((FP_MUL << DI_BITS) / (screenHeightHalf * scale)) / 2;
	const int xl = di * -screenWidthHalf + ((DI_OFF_X * di) >> DI_SHIFT_X);
	int yk = -di * -screenHeightHalf + ((DI_OFF_Y * di) >> DI_SHIFT_Y);

	const int di1 = 2 * di;
	const int di2 = 2 * di1; /* skip every other to prepare for a 4x2 instead of 2x2 */
	for (y=0; y<JULIA_QUARTER_HEIGHT/2; ++y)
	{
		int xk = xl;
		const int yki = yk >> DI_BITS;

		dst[0] = calcJuliaPixel(xk >> DI_BITS, yki, JULIA_LAYERS-1);
		xk+=di2;
		for (x=2; x<JULIA_QUARTER_WIDTH; x+=2)
		{
			const unsigned char c0 = dst[x-2];
			const unsigned char c1 = calcJuliaPixel(xk >> DI_BITS, yki, JULIA_LAYERS-1);

			dst[x] = c1;

			/* second pass for a 4x2 (tiny hard to see artifacts but some good performance improvement) */
			if (c0 == c1) {
				dst[x-1] = c0;
			} else {
				/* const int startingLayer = c0 >> 4;	// (10-20% increase if fed instead of JULIA_LAYERS-1 but very visible squary artifacts) */
				dst[x-1] = calcJuliaPixel((xk - di1) >> DI_BITS, yki, JULIA_LAYERS-1);
			}
			xk+=di2;
		}
		dst += JULIA_QUARTER_WIDTH;
        yk-=di1;
	}
}
#else
static void calcJuliaQuarter(float scale, int palAnimOffset)
{
	int x, y;

	const int screenWidthHalf = FX_WIDTH / 2;
	const int screenHeightHalf = FX_HEIGHT / 2;

	unsigned char *dst = juliaQuarterBuffer;

	int di = (int)((FP_MUL << DI_BITS) / (screenHeightHalf * scale)) / 2;
	const int xl = di * -screenWidthHalf + ((DI_OFF_X * di) >> DI_SHIFT_X);
	int yk = -di * -screenHeightHalf + ((DI_OFF_Y * di) >> DI_SHIFT_Y);

	di *= 2;
	for (y=0; y<JULIA_QUARTER_HEIGHT/2; ++y)
	{
		int xk = xl;
		for (x=0; x<JULIA_QUARTER_WIDTH; ++x)
		{
			const int xki = xk >> DI_BITS;
			const int yki = yk >> DI_BITS;

			dst[x] = calcJuliaPixel(xki, yki, JULIA_LAYERS-1);

			xk+=di;
		}
		dst += JULIA_QUARTER_WIDTH;
        yk-=di;
	}
}
#endif


#if FX_WIDTH==(FB_WIDTH / 2)
/* 160x200 version */
static void renderJuliaQuarter(float scale, int palAnimOffset)
{
	int x, y;

	const int screenWidthHalf = FX_WIDTH / 2;
	const int screenHeightHalf = FX_HEIGHT / 2;

	unsigned char *src = juliaQuarterBuffer;

	unsigned short *vramUp = (unsigned short*)fb_pixels;
	unsigned short *vramDown = vramUp + FB_WIDTH * (FB_HEIGHT - 1) - 4;
	unsigned int *vramUp32 = (unsigned int*)vramUp;
	unsigned int *vramDown32 = (unsigned int*)vramDown;
	unsigned int *pal32 = &juliaTunnelPal32[JULIA_COLORS * JULIA_LAYERS * palAnimOffset];

	const int di = (int)((FP_MUL << DI_BITS) / (screenHeightHalf * scale)) / 2;
	const int xl = di * -screenWidthHalf + ((DI_OFF_X * di) >> DI_SHIFT_X);
	int yk = -di * -screenHeightHalf + ((DI_OFF_Y * di) >> DI_SHIFT_Y);

	for (y=0; y<JULIA_QUARTER_HEIGHT/2-1; ++y)
	{
		int xk = xl;
		for (x=0; x<JULIA_QUARTER_WIDTH-2; ++x)
		{
			/* Comparing the two nearby char julia values on x, ignoring the bottom two, works also without visible artifacts, and is quite faster than all four.
			   So the final version, comparing unsigned char c0 and c1 alone, even if not entirely accurate does give the effect without visible artifacts and quiter better speed. */
			const unsigned char c0 = *src;
			const unsigned char c1 = *(src + 1);
			/* const unsigned char c2 = *(src + JULIA_QUARTER_WIDTH); */
			/* const unsigned char c3 = *(src + JULIA_QUARTER_WIDTH + 1); */

			/* NOTE: It just happens luckily that the format from julia calc is low 4bits = iterations, 4bits high = layer, and JULIA_COLORS = 16 right now
			   So it fits perfectly with how I store the palete per 16 colors per whatever layers, so the extra step is not necessary. So directly saying pal32[calcJuliaPixel] is fine, without breaking the bits.
			   But in case someone changes JULIA_COLORS I added extra code commented out, the two lines below and also 3 times in the 1x1 extra step should be uncommented and the 3rd line after commented
			   Slight but not significant increase in performance. I also like it as it is right now. */

			/* const unsigned char col = (c0 >> 4) * JULIA_COLORS + (c0 & 15); */
			/* unsigned int cc = pal32[col]; */
			unsigned int cc = pal32[c0];	/* it fits the 4bit layer - 4bit iteration value */

			*vramUp32 = cc;
			*(vramDown32+FX_WIDTH + 1) = cc;
			if (c0==c1) {
			/* if (c0==c1 && c0==c2 && c0==c3) { */
				*(vramUp32+1) = cc;
				*(vramUp32+FX_WIDTH) = cc;
				*(vramUp32+FX_WIDTH+1) = cc;
				*vramDown32 = cc;
				*(vramDown32+1) = cc;
				*(vramDown32+FX_WIDTH) = cc;
			} else {
				const int xki = xk >> DI_BITS;
				const int xki1 = (xk+di) >> DI_BITS;
				const int yki = yk >> DI_BITS;
				const int yki1 = (yk-di) >> DI_BITS;

				const int startingLayer = c0 >> 4;

				/* int cj = calcJuliaPixel(xki1, yki, startingLayer); */
				/* cc = pal32[(cj >> 4) * JULIA_COLORS + (cj & 15)]; */
				cc = pal32[calcJuliaPixel(xki1, yki, startingLayer)];	/* same */
				*(vramUp32+1) = cc;
				*(vramDown32+FX_WIDTH) = cc;

				/* cj = calcJuliaPixel(xki, yki1, startingLayer); */
				/* cc = pal32[(cj >> 4) * JULIA_COLORS + (cj & 15)]; */
				cc = pal32[calcJuliaPixel(xki, yki1, startingLayer)];	/* ditto */
				*(vramUp32+FX_WIDTH) = cc;
				*(vramDown32+1) = cc;

				/* cj = calcJuliaPixel(xki1, yki1, startingLayer); */
				/* cc = pal32[(cj >> 4) * JULIA_COLORS + (cj & 15)]; */
				cc = pal32[calcJuliaPixel(xki1, yki1, startingLayer)];	/* etc */
				*(vramUp32+FX_WIDTH+1) = cc;
				*vramDown32 = cc;
			}

			src++;
			vramUp32 += 2;
			vramDown32 -= 2;

			xk+=2*di;
		}
		src+=2;
		vramUp32 += FX_WIDTH;
		vramDown32 -= FX_WIDTH;
        yk-=2*di;
	}
}
#else
/* 320x200 version */
static void renderJuliaQuarter(float scale, int palAnimOffset)
{
	int x, y;

	const int screenWidthHalf = FX_WIDTH / 2;
	const int screenHeightHalf = FX_HEIGHT / 2;

	unsigned char *src = juliaQuarterBuffer;

	unsigned short *vramUp = (unsigned short*)fb_pixels;
	unsigned short *vramDown = vramUp + FB_WIDTH * (FB_HEIGHT - 1) - 2;
	unsigned short *pal = &juliaTunnelPal[JULIA_COLORS * JULIA_LAYERS * palAnimOffset];

	const int di = (int)((FP_MUL << DI_BITS) / (screenHeightHalf * scale)) / 2;
	const int xl = di * -screenWidthHalf + ((DI_OFF_X * di) >> DI_SHIFT_X);
	int yk = -di * -screenHeightHalf + ((DI_OFF_Y * di) >> DI_SHIFT_Y);

	for (y=0; y<JULIA_QUARTER_HEIGHT/2-1; ++y)
	{
		int xk = xl;
		for (x=0; x<JULIA_QUARTER_WIDTH-2; ++x)
		{
			/* Comparing the two nearby char julia values on x, ignoring the bottom two, works also without visible artifacts, and is quite faster than all four.
			   So the final version, comparing unsigned char c0 and c1 alone, even if not entirely accurate does give the effect without visible artifacts and quiter better speed. */
			const unsigned char c0 = *src;
			const unsigned char c1 = *(src + 1);
			/* const unsigned char c2 = *(src + JULIA_QUARTER_WIDTH); */
			/* const unsigned char c3 = *(src + JULIA_QUARTER_WIDTH + 1); */

			/* NOTE: It just happens luckily that the format from julia calc is low 4bits = iterations, 4bits high = layer, and JULIA_COLORS = 16 right now
			   So it fits perfectly with how I store the palete per 16 colors per whatever layers, so the extra step is not necessary. So directly saying pal32[calcJuliaPixel] is fine, without breaking the bits.
			   But in case someone changes JULIA_COLORS I added extra code commented out, the two lines below and also 3 times in the 1x1 extra step should be uncommented and the 3rd line after commented
			   Slight but not significant increase in performance. I also like it as it is right now. */

			/* const unsigned char col = (c0 >> 4) * JULIA_COLORS + (c0 & 15); */
			/* unsigned int cc = pal32[col]; */
			unsigned short cc = pal[c0];	/* it fits the 4bit layer - 4bit iteration value */

			*vramUp = cc;
			*(vramDown+FX_WIDTH + 1) = cc;
			if (c0==c1) {
			/* if (c0==c1 && c0==c2 && c0==c3) { */
				*(vramUp+1) = cc;
				*(vramUp+FX_WIDTH) = cc;
				*(vramUp+FX_WIDTH+1) = cc;
				*vramDown = cc;
				*(vramDown+1) = cc;
				*(vramDown+FX_WIDTH) = cc;
			} else {
				const int xki = xk >> DI_BITS;
				const int xki1 = (xk+di) >> DI_BITS;
				const int yki = yk >> DI_BITS;
				const int yki1 = (yk-di) >> DI_BITS;

				const int startingLayer = c0 >> 4;

				/* short cj = calcJuliaPixel(xki1, yki, startingLayer); */
				/* cc = pal[(cj >> 4) * JULIA_COLORS + (cj & 15)]; */
				cc = pal[calcJuliaPixel(xki1, yki, startingLayer)];	/* same */
				*(vramUp+1) = cc;
				*(vramDown+FX_WIDTH) = cc;

				/* short = calcJuliaPixel(xki, yki1, startingLayer); */
				/* cc = pal[(cj >> 4) * JULIA_COLORS + (cj & 15)]; */
				cc = pal[calcJuliaPixel(xki, yki1, startingLayer)];	/* ditto */
				*(vramUp+FX_WIDTH) = cc;
				*(vramDown+1) = cc;

				/* short = calcJuliaPixel(xki1, yki1, startingLayer); */
				/* cc = pal[(cj >> 4) * JULIA_COLORS + (cj & 15)]; */
				cc = pal[calcJuliaPixel(xki1, yki1, startingLayer)]; /* etc */
				*(vramUp+FX_WIDTH+1) = cc;
				*vramDown = cc;
			}

			src++;
			vramUp += 2;
			vramDown -= 2;

			xk+=2*di;
		}
		src+=2;
		vramUp += FX_WIDTH;
		vramDown -= FX_WIDTH;
        yk-=2*di;
	}
}
#endif

static void draw(void)
{
	int i;
	float tt = 0;

	const float ft = dseq_value(ev_fade);

	/* const int t = 3200; */
	/* const int layerAdv = 0; */

	const int t = (int)((time_msec - startingTime) * 1.275f);
	const int layerAdv = t >> JULIA_ANIM_REPEAT_BITS;
	const int palAnimOffset = (t >> (JULIA_ANIM_REPEAT_BITS - JULIA_PAL_TRANSITION_BITS)) & (JULIA_PAL_TRANSITIONS - 1);

	const float ts = (float)((t & (JULIA_ANIM_REPEAT-1)) / (float)(JULIA_ANIM_REPEAT-1));
	const float scale = 1.0f + pow(ts, 1.15);	/* magic number for smooth transmition between repeat scale, don.t know why this value works exactly :P */

	//tt = t / 1024.0f;
	//CLAMP(tt, 0, 1);
	tt = sin(t / 512.0f) * 0.125 + 0.875;

	for (i=0; i<JULIA_LAYERS; ++i) {
		const int ti = (i - layerAdv) << 9;
		xp_l[i] = (int)((sin((t + ti)/812.0) * FP_MUL) / (1.5 + 1.5*tt));
		yp_l[i] = (int)((sin((t + ti)/1482.0) * FP_MUL) / (1 + tt));
		/* xp_l[i] = 1280; */
		/* yp_l[i] = 1280; */
	}

	calcJuliaQuarter(scale, palAnimOffset);
	renderJuliaQuarter(scale, palAnimOffset);

	fadeToBlack16bpp(ft, fb_pixels, FB_WIDTH, FB_HEIGHT, FB_WIDTH);

	swap_buffers(0);
}
