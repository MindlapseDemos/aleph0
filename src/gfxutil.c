#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "demo.h"
#include "gfxutil.h"
#include "util.h"
#include "3dgfx/3dgfx.h"

void (*overlay_alpha)(struct image *dest, int x, int y, const struct image *src,
		int width, int height);
static void overlay_alpha_c(struct image *dest, int x, int y, const struct image *src,
		int width, int height);
static void overlay_alpha_asm(struct image *dest, int x, int y, const struct image *src,
		int width, int height);
static void overlay_alpha_mmx(struct image *dest, int x, int y, const struct image *src,
		int width, int height);

void init_gfxutil(void)
{
	overlay_alpha = overlay_alpha_c;
}

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
		} else /*if(oc_out & RIGHT)*/ {
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


#define BLUR(w, h, pstep, sstep) \
	for(i=0; i<h; i++) { \
		int r, g, b; \
		int rsum = UNPACK_R16(sptr[0]) * (rad + 1); \
		int gsum = UNPACK_G16(sptr[0]) * (rad + 1); \
		int bsum = UNPACK_B16(sptr[0]) * (rad + 1); \
		int count = (rad * 2 + 1) << 8; \
		int midsize = w - rad * 2; \
		int rfirstpix = UNPACK_R16(sptr[0]); \
		int rlastpix = UNPACK_R16(sptr[pstep * (w - 1)]); \
		int gfirstpix = UNPACK_G16(sptr[0]); \
		int glastpix = UNPACK_G16(sptr[pstep * (w - 1)]); \
		int bfirstpix = UNPACK_B16(sptr[0]); \
		int blastpix = UNPACK_B16(sptr[pstep * (w - 1)]); \
		/* add up the contributions for the -1 pixel */ \
		for(j=0; j<rad; j++) { \
			rsum += UNPACK_R16(sptr[pstep * j]); \
			gsum += UNPACK_G16(sptr[pstep * j]); \
			bsum += UNPACK_B16(sptr[pstep * j]); \
		} \
		/* first part adding sptr[rad] and subtracting sptr[0] */ \
		for(j=0; j<=rad; j++) { \
			rsum += UNPACK_R16((int)sptr[pstep * rad]) - rfirstpix; \
			gsum += UNPACK_G16((int)sptr[pstep * rad]) - gfirstpix; \
			bsum += UNPACK_B16((int)sptr[pstep * rad]) - bfirstpix; \
			sptr += pstep; \
			r = scale * rsum / count; \
			g = scale * gsum / count; \
			b = scale * bsum / count; \
			*dptr = PACK_RGB16(r, g, b); \
			dptr += pstep; \
		} \
		/* middle part adding sptr[rad] and subtracting sptr[-(rad+1)] */ \
		for(j=1; j<midsize; j++) { \
			rsum += UNPACK_R16((int)sptr[pstep * rad]) - UNPACK_R16((int)sptr[-(rad + 1) * pstep]); \
			gsum += UNPACK_G16((int)sptr[pstep * rad]) - UNPACK_G16((int)sptr[-(rad + 1) * pstep]); \
			bsum += UNPACK_B16((int)sptr[pstep * rad]) - UNPACK_B16((int)sptr[-(rad + 1) * pstep]); \
			sptr += pstep; \
			r = scale * rsum / count; \
			g = scale * gsum / count; \
			b = scale * bsum / count; \
			*dptr = PACK_RGB16(r, g, b); \
			dptr += pstep; \
		} \
		/* last part adding lastpix and subtracting sptr[-(rad+1)] */ \
		for(j=0; j<rad; j++) { \
			rsum += rlastpix - UNPACK_R16((int)sptr[-(rad + 1) * pstep]); \
			gsum += glastpix - UNPACK_G16((int)sptr[-(rad + 1) * pstep]); \
			bsum += blastpix - UNPACK_B16((int)sptr[-(rad + 1) * pstep]); \
			sptr += pstep; \
			r = scale * rsum / count; \
			g = scale * gsum / count; \
			b = scale * bsum / count; \
			*dptr = PACK_RGB16(r, g, b); \
			dptr += pstep; \
		} \
		sptr += sstep; \
		dptr += sstep; \
	}

/* TODO bound blur rad to image size to avoid inner loop conditionals */
/* TODO make version with pow2 (rad*2+1) to avoid div with count everywhere */
void blur_horiz(uint16_t *dest, uint16_t *src, int xsz, int ysz, int rad, int scale)
{
	int i, j;
	uint16_t *dptr = dest;
	uint16_t *sptr = src;

	BLUR(xsz, ysz, 1, 0);
}


void blur_vert(uint16_t *dest, uint16_t *src, int xsz, int ysz, int rad, int scale)
{
	int i, j;
	uint16_t *dptr = dest;
	uint16_t *sptr = src;
	int pixel_step = xsz;
	int scanline_step = 1 - ysz * pixel_step;

	BLUR(ysz, xsz, pixel_step, scanline_step);
}

/* hardcode and approximate everything */
#define XYZZY_RAD		5
#define XYZZY_MIDCOUNT	(320 - XYZZY_RAD * 2)
/*#define XYZZY_WSUM(x)	(240 * (x) / (11 << 8))*/
#define XYZZY_WSUM(x)	(((x) >> 4) + ((x) >> 6))
#define XYZZY_CLAMP(dest, val) dest = val
/*
#define XYZZY_CLAMP(dest, val) \
	do { \
		int res = val; \
		if(res < 0) res = 0; \
		if(res > 255) res = 255; \
		dest = res; \
	} while(0)
*/


void blur_xyzzy_horiz8(uint8_t *dest, uint8_t *src)
{
	int i, j;
	unsigned char *sptr, *dptr;
	int sum, first, last;

	sptr = src;
	dptr = dest;

	for(i=0; i<240; i++) {
		first = sptr[0];
		last = sptr[319];

		/* start with the sum corresponding to the pixel just outside */
		/* add up all outside boundary pixels */
		sum = first * (XYZZY_RAD + 1);
		/* add the right part of the kernel which are all inside */
		for(j=0; j<XYZZY_RAD; j++) {
			sum += sptr[j];
		}

		/* first stage: add sptr[rad], subtract one of the outside pixels */
		for(j=0; j<=XYZZY_RAD; j++) {
			sum = sum + sptr[XYZZY_RAD] - first;
			sptr++;
			XYZZY_CLAMP(dptr[j], XYZZY_WSUM(sum));
		}
		dptr += XYZZY_RAD + 1;

		/* iterate for the rest of the scanline until the end boundary condition */
		for(j=0; j<XYZZY_MIDCOUNT-1; j++) {
			sum = sum + sptr[XYZZY_RAD] - sptr[-XYZZY_RAD-1];
			sptr++;
			XYZZY_CLAMP(dptr[j], XYZZY_WSUM(sum));
		}
		dptr += XYZZY_MIDCOUNT - 1;

		/* last stage: add last, subtract left */
		for(j=0; j<XYZZY_RAD; j++) {
			sum = sum + last - sptr[-XYZZY_RAD-1];
			sptr++;
			XYZZY_CLAMP(dptr[j], XYZZY_WSUM(sum));
		}
		dptr += XYZZY_RAD;
	}
}

#define XYZZY_INOFFS	(XYZZY_RAD * 320)
#define XYZZY_OUTOFFS	(-(XYZZY_RAD + 1) * 320)
void blur_xyzzy_vert8(uint8_t *dest, uint8_t *src)
{
	int i, j;
	unsigned char *sptr, *dptr;
	int sum, first, last;

	for(i=0; i<320; i++) {
		sptr = src + i;
		dptr = dest + i;

		first = sptr[0];
		last = sptr[320 * 239];

		/* start with the sum corresponding to the pixel just outside */
		/* add up all outside boundary pixels */
		sum = first * (XYZZY_RAD + 1);
		/* add the right part of the kernel which are all inside */
		for(j=0; j<XYZZY_RAD; j++) {
			sum += sptr[XYZZY_INOFFS];
		}

		/* first stage: add sptr[XYZZY_RAD], subtract one of the outside pixels */
		for(j=0; j<=XYZZY_RAD; j++) {
			sum = sum + sptr[XYZZY_INOFFS] - first;
			sptr += 320;
			XYZZY_CLAMP(*dptr, XYZZY_WSUM(sum));
			dptr += 320;
		}

		/* iterate for the rest of the column until the end boundary condition */
		for(j=0; j<XYZZY_MIDCOUNT-1; j++) {
			sum = sum + sptr[XYZZY_INOFFS] - sptr[XYZZY_OUTOFFS];
			sptr += 320;
			XYZZY_CLAMP(*dptr, XYZZY_WSUM(sum));
			dptr += 320;
		}

		/* last stage: add last, subtract left */
		for(j=0; j<XYZZY_RAD; j++) {
			sum = sum + last - sptr[XYZZY_OUTOFFS];
			sptr += 320;
			XYZZY_CLAMP(*dptr, XYZZY_WSUM(sum));
			dptr += 320;
		}
	}
}

void convimg_rgb24_rgb16(uint16_t *dest, unsigned char *src, int xsz, int ysz)
{
	int i;
	int npixels = xsz * ysz;

	for(i=0; i<npixels; i++) {
		int r = *src++;
		int g = *src++;
		int b = *src++;
		*dest++ = PACK_RGB16(r, g, b);
	}
}

void blitfb(uint16_t *dest, uint16_t *src, int width, int height, int pitch_pix)
{
	int i;
	for(i=0; i<height; i++) {
		if((width & 3) == 0) {
			memcpy64(dest, src, width >> 2);
		} else {
			memcpy(dest, src, width << 1);
		}
		dest += 320;
		src += pitch_pix;
	}
}

void blit(uint16_t *dest, int destwidth, uint16_t *src, int width, int height, int pitch_pix)
{
	int i, spansz = width << 1;
	for(i=0; i<height; i++) {
		memcpy(dest, src, spansz);
		dest += destwidth;
		src += pitch_pix;
	}
}

void blit_key(uint16_t *dest, int destwidth, uint16_t *src, int width, int height, int pitch_pix, uint16_t key)
{
	int i, j;
	int dadv = destwidth - width;
	int sadv = pitch_pix - width;

	for(i=0; i<height; i++) {
		for(j=0; j<width; j++) {
			uint16_t scol = *src++;
			if(scol != key) *dest = scol;
			dest++;
		}
		dest += dadv;
		src += sadv;
	}

}

void overlay_add_full(uint16_t *dest, uint16_t *src)
{
	/* TODO */
}

void overlay_add_pal(uint16_t *dest, uint8_t *src, int xsz, int ysz, int pitch_pix, unsigned int *pal)
{
	int i, j;
	unsigned int r, g, b;
	unsigned int *col;
	uint16_t pixel;

	for(i=0; i<ysz; i++) {
		for(j=0; j<xsz; j++) {
			pixel = dest[j];
			col = pal + ((unsigned int)src[j] << 2);
			r = UNPACK_R16(pixel) + col[0];
			g = UNPACK_G16(pixel) + col[1];
			b = UNPACK_B16(pixel) + col[2];
			if(r > 255) r = 255;
			if(g > 255) g = 255;
			if(b > 255) b = 255;
			dest[j] = PACK_RGB16(r, g, b);
		}
		src += pitch_pix;
		dest += 320;
	}
}

void overlay_full_add_pal(uint16_t *dest, uint8_t *src, unsigned int *pal)
{
	int i;
	unsigned int r, g, b;
	unsigned int *col;
	uint16_t pixel;

	/*perf_start();*/

	for(i=0; i<320*240; i++) {
		pixel = dest[i];
		col = pal + ((unsigned int)src[i] << 2);
		r = UNPACK_R16(pixel) + col[0];
		g = UNPACK_G16(pixel) + col[1];
		b = UNPACK_B16(pixel) + col[2];
		if(r > 255) r = 255;
		if(g > 255) g = 255;
		if(b > 255) b = 255;
		dest[i] = PACK_RGB16(r, g, b);
	}

	/*perf_end();
	printf("%lu\n", (unsigned long)perf_interval_count);*/
}

static void overlay_alpha_c(struct image *dest, int x, int y, const struct image *src,
		int width, int height)
{
	int i, j;
	unsigned int sr, sg, sb, dr, dg, db;
	unsigned int alpha, invalpha;
	uint16_t *dptr, *sptr, pix;
	uint32_t pixpair;
	unsigned char *aptr;

	assert(dest->width == 320);

	dptr = dest->pixels + (y << 8) + (y << 6) + x;
	sptr = src->pixels;
	aptr = src->alpha;

	for(i=0; i<height; i++) {
		for(j=0; j<width/8; j++) {
			int idx = j << 3;
#define INNERLOOP \
			alpha = aptr[idx];	\
			invalpha = ~alpha & 0xff;	\
			pix = sptr[idx];	\
			sr = UNPACK_R16(pix);	\
			sg = UNPACK_G16(pix);	\
			sb = UNPACK_B16(pix);	\
			pix = dptr[idx];	\
			dr = ((UNPACK_R16(pix) * invalpha) >> 8) + sr;	\
			dg = ((UNPACK_G16(pix) * invalpha) >> 8) + sg;	\
			db = ((UNPACK_B16(pix) * invalpha) >> 8) + sb;	\
			dptr[idx++] = PACK_RGB16(dr, dg, db)

			INNERLOOP;
			INNERLOOP;
			INNERLOOP;
			INNERLOOP;
			INNERLOOP;
			INNERLOOP;
			INNERLOOP;
			INNERLOOP;
		}
		for(j=width&0xfff8; j<width; j++) {
			int idx = j;
			INNERLOOP;
		}
		dptr += 320;
		sptr += src->scanlen;
		aptr += src->scanlen;
	}
}

static void overlay_alpha_mmx(struct image *dest, int x, int y, const struct image *src,
		int width, int height)
{
}

void draw_billboard(float x, float y, float z, float size, int r, int g, int b, int a)
{
	float m[16];
	size *= 0.5f;

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_push_matrix();

	g3d_translate(x, y, z);

	g3d_get_matrix(G3D_MODELVIEW, m);
	/* make the upper 3x3 part of the matrix identity */
	m[0] = m[5] = m[10] = 1.0f;
	m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = 0.0f;
	g3d_load_matrix(m);

	g3d_begin(G3D_QUADS);
	g3d_color4b(r, g, b, a);
	g3d_texcoord(0, 0);
	g3d_vertex(-size, -size, 0);
	g3d_texcoord(1, 0);
	g3d_vertex(size, -size, 0);
	g3d_texcoord(1, 1);
	g3d_vertex(size, size, 0);
	g3d_texcoord(0, 1);
	g3d_vertex(-size, size, 0);
	g3d_end();

	g3d_pop_matrix();
}
