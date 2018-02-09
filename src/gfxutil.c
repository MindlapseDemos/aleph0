#include "gfxutil.h"
#include "demo.h"

enum {
	IN		= 0,
	LEFT	= 1,
	RIGHT	= 2,
	TOP		= 4,
	BOTTOM	= 8
};

static int outcode(int x, int y, int xmin, int ymin, int xmax, int ymax)
{
	int code = 0;

	if(x < xmin) {
		code |= LEFT;
	} else if(x > xmax) {
		code |= RIGHT;
	}
	if(y < ymin) {
		code |= TOP;
	} else if(y > ymax) {
		code |= BOTTOM;
	}
	return code;
}

#define FIXMUL(a, b)	(((a) * (b)) >> 8)
#define FIXDIV(a, b)	(((a) << 8) / (b))

#define LERP(a, b, t)	((a) + FIXMUL((b) - (a), (t)))

int clip_line(int *x0, int *y0, int *x1, int *y1, int xmin, int ymin, int xmax, int ymax)
{
	int oc_out;

	int oc0 = outcode(*x0, *y0, xmin, ymin, xmax, ymax);
	int oc1 = outcode(*x1, *y1, xmin, ymin, xmax, ymax);

	long fx0, fy0, fx1, fy1, fxmin, fymin, fxmax, fymax;

	if(!(oc0 | oc1)) return 1;	/* both points are inside */

	fx0 = *x0 << 8;
	fy0 = *y0 << 8;
	fx1 = *x1 << 8;
	fy1 = *y1 << 8;
	fxmin = xmin << 8;
	fymin = ymin << 8;
	fxmax = xmax << 8;
	fymax = ymax << 8;

	for(;;) {
		long x, y, t;

		if(oc0 & oc1) return 0;		/* both have points with the same outbit, not visible */
		if(!(oc0 | oc1)) break;		/* both points are inside */

		oc_out = oc0 ? oc0 : oc1;

		if(oc_out & TOP) {
			t = FIXDIV(fymin - fy0, fy1 - fy0);
			x = LERP(fx0, fx1, t);
			y = fymin;
		} else if(oc_out & BOTTOM) {
			t = FIXDIV(fymax - fy0, fy1 - fy0);
			x = LERP(fx0, fx1, t);
			y = fymax;
		} else if(oc_out & LEFT) {
			t = FIXDIV(fxmin - fx0, fx1 - fx0);
			x = fxmin;
			y = LERP(fy0, fy1, t);
		} else if(oc_out & RIGHT) {
			t = FIXDIV(fxmax - fx0, fx1 - fx0);
			x = fxmax;
			y = LERP(fy0, fy1, t);
		}

		if(oc_out == oc0) {
			fx0 = x;
			fy0 = y;
			oc0 = outcode(fx0 >> 8, fy0 >> 8, xmin, ymin, xmax, ymax);
		} else {
			fx1 = x;
			fy1 = y;
			oc1 = outcode(fx1 >> 8, fy1 >> 8, xmin, ymin, xmax, ymax);
		}
	}

	*x0 = fx0 >> 8;
	*y0 = fy0 >> 8;
	*x1 = fx1 >> 8;
	*y1 = fy1 >> 8;
	return 1;
}

void draw_line(int x0, int y0, int x1, int y1, unsigned short color)
{
	int i, dx, dy, x_inc, y_inc, error;
	unsigned short *fb = fb_pixels;

	fb += y0 * fb_width + x0;

	dx = x1 - x0;
	dy = y1 - y0;

	if(dx >= 0) {
		x_inc = 1;
	} else {
		x_inc = -1;
		dx = -dx;
	}
	if(dy >= 0) {
		y_inc = fb_width;
	} else {
		y_inc = -fb_width;
		dy = -dy;
	}

	if(dx > dy) {
		error = dy * 2 - dx;
		for(i=0; i<=dx; i++) {
			*fb = color;
			if(error >= 0) {
				error -= dx * 2;
				fb += y_inc;
			}
			error += dy * 2;
			fb += x_inc;
		}
	} else {
		error = dx * 2 - dy;
		for(i=0; i<=dy; i++) {
			*fb = color;
			if(error >= 0) {
				error -= dy * 2;
				fb += x_inc;
			}
			error += dx * 2;
			fb += y_inc;
		}
	}
}


/* TODO bound blur rad to image size to avoid inner loop conditionals */
/* TODO make version with pow2 (rad*2+1) to avoid div with count everywhere */
void blur_grey_horiz(uint16_t *dest, uint16_t *src, int xsz, int ysz, int rad)
{
	int i, j;

	for(i=0; i<ysz; i++) {
		int sum = src[0] * (rad + 1);
		int count = rad * 2 + 1;
		uint16_t lastpix = src[xsz - 1];

		/* add up the contributions for the -1 pixel */
		for(j=0; j<rad; j++) {
			sum += src[j];
		}

		/* first part adding src[rad] and subtracting src[0] */
		for(j=0; j<rad; j++) {
			sum += src[rad] - src[0];
			++src;
			*dest++ = sum / count;
		}

		/* middle part adding src[rad] and subtracting src[-(rad+1)] */
		for(j=0; j<xsz - rad; j++) {
			sum += src[rad] - src[-(rad + 1)];
			++src;
			*dest++ = sum / count;
		}

		/* last part adding lastpix and subtracting src[-(rad+1)] */
		for(j=0; j<rad; j++) {
			sum += lastpix - src[-(rad + 1)];
			++src;
			*dest++ = sum / count;
		}
	}
}

void blur_grey_vert(uint16_t *dest, uint16_t *src, int xsz, int ysz, int radius)
{
}
