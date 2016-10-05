#include <string.h>
#include <limits.h>
#include "demo.h"
#include "screen.h"
#include "gfxutil.h"

struct vec2x {
	long x, y;
};

static int init(void);
static void destroy(void);
static void draw(void);
static int julia(long x, long y, long cx, long cy, int max_iter);

static struct screen scr = {
	"fract",
	init,
	destroy,
	0, 0,
	draw
};

/*static long aspect_24x8 = (long)(1.3333333 * 256.0);*/
static long xscale_24x8 = (long)(1.3333333 * 1.2 * 256.0);
static long yscale_24x8 = (long)(1.2 * 256.0);
static int cx, cy;
static int max_iter = 50;

#define WALK_SIZE	20

struct screen *fract_screen(void)
{
	return &scr;
}

static int init(void)
{
	return 0;
}

static void destroy(void)
{
}

#define PACK_RGB16(r, g, b) \
	(((((r) >> 3) & 0x1f) << 11) | ((((g) >> 2) & 0x3f) << 5) | (((b) >> 3) & 0x1f))

static void draw(void)
{
	int i, j;
	unsigned short *pixels = vmem_back;

	cx = mouse_x;
	cy = mouse_y;

	for(i=0; i<fb_height; i++) {
		for(j=0; j<fb_width; j++) {
			unsigned char pidx = julia(j, i, cx, cy, max_iter) & 0xff;
			*pixels++ = (pidx >> 3) | ((pidx >> 2) << 5) | ((pidx >> 3) << 11);
		}
	}

	pixels = vmem_back;
	pixels[mouse_y * fb_width + mouse_x] = 0xffe;
	swap_buffers(vmem_back);
}

static long normalize_coord(long x, long range)
{
	/* 2 * x / range - 1*/
	return (x << 17) / range - 65536;
}

static int julia(long x, long y, long cx, long cy, int max_iter)
{
	int i;

	/* convert to fixed point roughly [-1, 1] */
	x = (normalize_coord(x, fb_width) >> 8) * xscale_24x8;
	y = (normalize_coord(y, fb_height) >> 8) * yscale_24x8;
	cx = (normalize_coord(cx, fb_width) >> 8) * xscale_24x8;
	cy = (normalize_coord(cy, fb_height) >> 8) * yscale_24x8;

	for(i=0; i<max_iter; i++) {
		/* z_n = z_{n-1}**2 + c */
		long px = x >> 8;
		long py = y >> 8;

		if(px * px + py * py > (4 << 16)) {
			break;
		}
		x = px * px - py * py + cx;
		y = (px * py << 1) + cy;
	}

	return i < max_iter ? (256 * i / max_iter) : 0;
}
