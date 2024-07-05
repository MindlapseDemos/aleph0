#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "polyfill.h"
#include "gfxutil.h"
#include "util.h"

/*#define DEBUG_OVERDRAW	G3D_PACK_RGB(10, 10, 10)*/

#define FILL_POLY_BITS	0x03

/*void polyfill_tex_flat_new(struct pvertex *varr);*/

/* mode bits: 00-wire 01-flat 10-gouraud 11-reserved
 *     bit 2: texture
 *     bit 3-4: blend mode: 00-none 01-alpha 10-additive 11-reserved
 *     bit 5: zbuffering
 *     bit 6: additive texturing
 */
void (*fillfunc[])(struct pvertex*) = {
	polyfill_wire, polyfill_flat, polyfill_gouraud, 0,
	polyfill_tex_wire, polyfill_tex_flat, polyfill_tex_gouraud, 0,
	polyfill_alpha_wire, polyfill_alpha_flat, polyfill_alpha_gouraud, 0,
	polyfill_alpha_tex_wire, polyfill_alpha_tex_flat, polyfill_alpha_tex_gouraud, 0,
	polyfill_add_wire, polyfill_add_flat, polyfill_add_gouraud, 0,
	polyfill_add_tex_wire, polyfill_add_tex_flat, polyfill_add_tex_gouraud, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	polyfill_wire, polyfill_flat_zbuf, polyfill_gouraud_zbuf, 0,
	polyfill_tex_wire, polyfill_tex_flat_zbuf, polyfill_tex_gouraud_zbuf, 0,
	polyfill_alpha_wire, polyfill_alpha_flat_zbuf, polyfill_alpha_gouraud_zbuf, 0,
	polyfill_alpha_tex_wire, polyfill_alpha_tex_flat_zbuf, polyfill_alpha_tex_gouraud_zbuf, 0,
	polyfill_add_wire, polyfill_add_flat_zbuf, polyfill_add_gouraud_zbuf, 0,
	polyfill_add_tex_wire, polyfill_add_tex_flat_zbuf, polyfill_add_tex_gouraud_zbuf, 0,
	0, 0, 0, 0, 0, 0, 0, 0,

	polyfill_wire, polyfill_flat, polyfill_gouraud, 0,
	0, polyfill_addtex_flat, polyfill_addtex_gouraud, 0,
	polyfill_alpha_wire, polyfill_alpha_flat, polyfill_alpha_gouraud, 0,
	0, 0, 0, 0,
	polyfill_add_wire, polyfill_add_flat, polyfill_add_gouraud, 0,
	0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	polyfill_wire, polyfill_flat_zbuf, polyfill_gouraud_zbuf, 0,
	0, polyfill_addtex_flat_zbuf, polyfill_addtex_gouraud_zbuf, 0,
	polyfill_alpha_wire, polyfill_alpha_flat_zbuf, polyfill_alpha_gouraud_zbuf, 0,
	0, 0, 0, 0,
	polyfill_add_wire, polyfill_add_flat_zbuf, polyfill_add_gouraud_zbuf, 0,
	0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};

struct pimage pfill_fb, pfill_tex;
uint32_t *pfill_zbuf;
struct pgradient pgrad;

#define EDGEPAD	8
static struct pvertex *edgebuf, *left, *right;
static int edgebuf_size;
static int fbheight;

/*
#define CHECKEDGE(x) \
	do { \
		assert(x >= 0); \
		assert(x < fbheight); \
	} while(0)
*/
#define CHECKEDGE(x)


void polyfill_fbheight(int height)
{
	int newsz = (height * 2 + EDGEPAD * 3) * sizeof *edgebuf;

	if(newsz > edgebuf_size) {
		free(edgebuf);
		if(!(edgebuf = malloc(newsz))) {
			fprintf(stderr, "failed to allocate edge table buffer (%d bytes)\n", newsz);
			abort();
		}
		edgebuf_size = newsz;

		left = edgebuf + EDGEPAD;
		right = edgebuf + height + EDGEPAD * 2;

#ifndef NDEBUG
		memset(edgebuf, 0xaa, EDGEPAD * sizeof *edgebuf);
		memset(edgebuf + height + EDGEPAD, 0xaa, EDGEPAD * sizeof *edgebuf);
		memset(edgebuf + height * 2 + EDGEPAD * 2, 0xaa, EDGEPAD * sizeof *edgebuf);
#endif
	}

	fbheight = height;
}

void polyfill(int mode, struct pvertex *verts)
{
#ifndef NDEBUG
	if(!fillfunc[mode]) {
		fprintf(stderr, "polyfill mode %d not implemented\n", mode);
		abort();
	}
#endif

	fillfunc[mode](verts);
}

void polyfill_wire(struct pvertex *verts)
{
	int i, x0, y0, x1, y1;
	struct pvertex *v = verts;
	unsigned short color = ((v->r << 8) & 0xf800) |
		((v->g << 3) & 0x7e0) | ((v->b >> 3) & 0x1f);

	for(i=0; i<2; i++) {
		x0 = v->x >> 8;
		y0 = v->y >> 8;
		++v;
		x1 = v->x >> 8;
		y1 = v->y >> 8;
		if(clip_line(&x0, &y0, &x1, &y1, 0, 0, pfill_fb.width, pfill_fb.height)) {
			draw_line(x0, y0, x1, y1, color);
		}
	}
	x0 = verts[0].x >> 8;
	y0 = verts[0].y >> 8;
	if(clip_line(&x1, &y1, &x0, &y0, 0, 0, pfill_fb.width, pfill_fb.height)) {
		draw_line(x1, y1, x0, y0, color);
	}
}

void polyfill_tex_wire(struct pvertex *verts)
{
	polyfill_wire(verts);	/* TODO */
}

void polyfill_alpha_wire(struct pvertex *verts)
{
	polyfill_wire(verts);	/* TODO */
}

void polyfill_alpha_tex_wire(struct pvertex *verts)
{
	polyfill_wire(verts);	/* TODO */
}

void polyfill_add_wire(struct pvertex *verts)
{
	polyfill_wire(verts);	/* TODO */
}

void polyfill_add_tex_wire(struct pvertex *verts)
{
	polyfill_wire(verts);	/* TODO */
}

#define VNEXT(p)	(((p) == varr + 2) ? varr : (p) + 1)
#define VPREV(p)	((p) == varr ? varr + 2 : (p) - 1)
#define VSUCC(p, side)	((side) == 0 ? VNEXT(p) : VPREV(p))

/* extra bits of precision to use when interpolating colors.
 * try tweaking this if you notice strange quantization artifacts.
 */
#define COLOR_SHIFT	12


#define POLYFILL polyfill_flat
#undef GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_gouraud
#define GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_tex_flat
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_tex_gouraud
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_flat
#undef GOURAUD
#undef TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_gouraud
#define GOURAUD
#undef TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_tex_flat
#undef GOURAUD
#define TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_tex_gouraud
#define GOURAUD
#define TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_flat
#undef GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_gouraud
#define GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_tex_flat
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_tex_gouraud
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

/* ---- zbuffer variants ----- */

#define POLYFILL polyfill_flat_zbuf
#undef GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_gouraud_zbuf
#define GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_tex_flat_zbuf
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_tex_gouraud_zbuf
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_flat_zbuf
#undef GOURAUD
#undef TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_gouraud_zbuf
#define GOURAUD
#undef TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_tex_flat_zbuf
#undef GOURAUD
#define TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_tex_gouraud_zbuf
#define GOURAUD
#define TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_flat_zbuf
#undef GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_gouraud_zbuf
#define GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_tex_flat_zbuf
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_tex_gouraud_zbuf
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_addtex_flat
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#define TEX_ADD
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_addtex_gouraud
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#define TEX_ADD
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_addtex_flat_zbuf
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#define TEX_ADD
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_addtex_gouraud_zbuf
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#define TEX_ADD
#include "polytmpl.h"
#undef POLYFILL

#if 0
void polyfill_tex_flat_new(struct pvertex *varr)
{
	int i, line, top, bot;
	struct pvertex *v, *vn, *tab;
	int32_t x, y0, y1, dx, dy, slope, fx, fy;
	int start, len;
	g3d_pixel *fbptr, *pptr, color;
	int32_t tu, tv, du, dv, uslope, vslope;
	int tx, ty;
	g3d_pixel texel;

	top = pfill_fb.height;
	bot = 0;

	for(i=0; i<3; i++) {
		/* scan the edge between the current and next vertex */
		v = varr + i;
		vn = VNEXT(v);

		if(vn->y == v->y) continue;	/* XXX ??? */

		if(vn->y >= v->y) {
			/* inrementing Y: left side */
			tab = left;
		} else {
			/* decrementing Y: right side, flip vertices to trace bottom->up */
			tab = right;
			v = vn;
			vn = varr + i;
		}

		/* calculate edge slope */
		dx = vn->x - v->x;
		dy = vn->y - v->y;
		slope = (dx << 8) / dy;

		tu = v->u;
		tv = v->v;
		du = vn->u - tu;
		dv = vn->v - tv;
		uslope = (du << 8) / dy;
		vslope = (dv << 8) / dy;

		y0 = (v->y + 0x100) & 0xffffff00;	/* start from the next scanline */
		fy = y0 - v->y;						/* fractional part before the next scanline */
		fx = (fy * slope) >> 8;				/* X adjust for the step to the next scanline */
		x = v->x + fx;						/* adjust X */
		y1 = vn->y & 0xffffff00;			/* last scanline of the edge <= vn->y */

		/* also adjust other interpolated attributes */
		tu += (fy * uslope) >> 8;
		tv += (fy * vslope) >> 8;

		line = y0 >> 8;
		if(line < top) top = line;
		if((y1 >> 8) > bot) bot = y1 >> 8;

		if(line > 0) tab += line;

		while(line <= (y1 >> 8) && line < pfill_fb.height) {
			if(line >= 0) {
				int val = x < 0 ? 0 : x >> 8;
				tab->x = val < pfill_fb.width ? val : pfill_fb.width - 1;
				tab->u = tu;
				tab->v = tv;
				tab++;
			}
			x += slope;
			tu += uslope;
			tv += vslope;
			line++;
		}
	}

	if(top < 0) top = 0;
	if(bot >= pfill_fb.height) bot = pfill_fb.height - 1;

	fbptr = pfill_fb.pixels + top * pfill_fb.width;
	for(i=top; i<=bot; i++) {
		start = left[i].x;
		len = right[i].x - start;
		/* XXX we probably need more precision in left/right.x */

		dx = len == 0 ? 256 : (len << 8);

		tu = left[i].u;
		tv = left[i].v;

		pptr = fbptr + start;
		while(len-- > 0) {
			int cr, cg, cb;

			tx = (tu >> (16 - pfill_tex.xshift)) & pfill_tex.xmask;
			ty = (tv >> (16 - pfill_tex.yshift)) & pfill_tex.ymask;
			texel = pfill_tex.pixels[(ty << pfill_tex.xshift) + tx];

			tu += pgrad.dudx;
			tv += pgrad.dvdx;

			cr = varr[0].r;
			cg = varr[0].g;
			cb = varr[0].b;

			/* This is not correct, should be /255, but it's much faster
			 * to shift by 8 (/256), and won't make a huge difference
			 */
			cr = (cr * G3D_UNPACK_R(texel)) >> 8;
			cg = (cg * G3D_UNPACK_G(texel)) >> 8;
			cb = (cb * G3D_UNPACK_B(texel)) >> 8;

			if(cr >= 255) cr = 255;
			if(cg >= 255) cg = 255;
			if(cb >= 255) cb = 255;
			color = G3D_PACK_RGB(cr, cg, cb);
			*pptr++ = color;
		}
		fbptr += pfill_fb.width;
	}
}
#endif
