/* Julia Tunnel idea */

#include <stdlib.h>
#include <math.h>

#include "demo.h"
#include "screen.h"


#define JULIA_LAYERS 4
#define JULIA_COLORS 16
#define JULIA_PAL_COLORS (JULIA_COLORS * JULIA_LAYERS)

#define FP_SHR 12
#define FP_MUL (1 << FP_SHR)

#define ESCAPE ((4 * FP_MUL) << FP_SHR)

#define FRAC_ITER_BLOCK \
						x1 = ((x0 * x0 - y0 * y0)>>FP_SHR) + xp; \
						y1 = ((x0 * y0)>>(FP_SHR-1)) + yp; \
						mj2 = x1 * x1 + y1 * y1; \
						x0 = x1; y0 = y1; c++; \
						if (mj2 > ESCAPE) goto end1;

#define FRAC_ITER_TIMES_2 FRAC_ITER_BLOCK FRAC_ITER_BLOCK
#define FRAC_ITER_TIMES_4 FRAC_ITER_TIMES_2 FRAC_ITER_TIMES_2
#define FRAC_ITER_TIMES_8 FRAC_ITER_TIMES_4 FRAC_ITER_TIMES_4
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


static unsigned short *juliaTunelPal;

static int xp_l[JULIA_LAYERS];
static int yp_l[JULIA_LAYERS];


struct screen *juliatunnel_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i,j,k=0;

	juliaTunelPal = (unsigned short*)malloc(sizeof(unsigned short) * JULIA_PAL_COLORS);

	for (j=0; j<JULIA_LAYERS; ++j) {
		for (i=0; i<JULIA_COLORS; ++i) {
			int r,g,b;
			int c = abs(JULIA_COLORS / 2 - 1 - i);
			if (c >= JULIA_COLORS / 2) c = JULIA_COLORS / 2 - 1;
			c = JULIA_COLORS / 2 - c - 1;

			r = c << 2;
			g = c << 2;
			b = c << 1;

			juliaTunelPal[k++] = (r<<11) | (g<<5) | b;
		}
	}

	return 0;
}

static void destroy(void)
{
}

static void start(long trans_time)
{
}

static unsigned char renderJuliaPixel(int xk, int yk, int layer_iter)
{
	int x1,y1,mj2;

	int x0 = xk;
	int y0 = yk;
	unsigned char c = 255;

	const int xp = xp_l[layer_iter-1];
	const int yp = yp_l[layer_iter-1];

	FRAC_ITER_TIMES_16

	end1:
	
	if (c>=15 && --layer_iter != 0) return renderJuliaPixel(xk<<1, yk<<1, layer_iter);

	return c;
}

static void renderJulia()
{
	int x, y;

	const int screenWidthHalf = FB_WIDTH / 2;
	const int screenHeightHalf = FB_HEIGHT / 2;

	const int di = (int)(FP_MUL / (screenHeightHalf * 1.5));

	unsigned short *vramUp = (unsigned short*)fb_pixels;
	unsigned short *vramDown = vramUp + FB_WIDTH * FB_HEIGHT - 1;

	int yk = -di * -screenHeightHalf;
	int xl = di * -screenWidthHalf;

	for (y=0; y<screenHeightHalf; y++)
	{
		int xk = xl;
		for (x=0; x<FB_WIDTH; x++)
		{
			const unsigned char c = renderJuliaPixel(xk, yk, JULIA_LAYERS);
			const unsigned short cc = juliaTunelPal[c];

			*vramUp++ = cc;
			*vramDown-- = cc;

			xk+=di;
		}
        yk-=di;
	}
}

static void draw(void)
{
	int i;

	for (i=0; i<JULIA_LAYERS; ++i) {
		const int t = i << 9;
		xp_l[i] = (int)(sin((time_msec + t)/812.0) * FP_MUL) >> 1;
		yp_l[i] = (int)(sin((time_msec + t)/1482.0) * FP_MUL) >> 1;
	}

	renderJulia();

	swap_buffers(0);
}
