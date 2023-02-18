#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "polyfill.h"
#include "gfxutil.h"
#include "util.h"

/*#define DEBUG_OVERDRAW	G3D_PACK_RGB(10, 10, 10)*/

#define FILL_POLY_BITS	0x03

void polyfill_flat_new(struct pvertex *varr, int vnum);

/* mode bits: 00-wire 01-flat 10-gouraud 11-reserved
 *     bit 2: texture
 *     bit 3-4: blend mode: 00-none 01-alpha 10-additive 11-reserved
 *     bit 5: zbuffering
 */
void (*fillfunc[])(struct pvertex*, int) = {
	polyfill_wire,
	polyfill_flat_new,
	polyfill_gouraud,
	0,
	polyfill_tex_wire,
	polyfill_tex_flat,
	polyfill_tex_gouraud,
	0,
	polyfill_alpha_wire,
	polyfill_alpha_flat,
	polyfill_alpha_gouraud,
	0,
	polyfill_alpha_tex_wire,
	polyfill_alpha_tex_flat,
	polyfill_alpha_tex_gouraud,
	0,
	polyfill_add_wire,
	polyfill_add_flat,
	polyfill_add_gouraud,
	0,
	polyfill_add_tex_wire,
	polyfill_add_tex_flat,
	polyfill_add_tex_gouraud,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	polyfill_wire,
	polyfill_flat_zbuf,
	polyfill_gouraud_zbuf,
	0,
	polyfill_tex_wire,
	polyfill_tex_flat_zbuf,
	polyfill_tex_gouraud_zbuf,
	0,
	polyfill_alpha_wire,
	polyfill_alpha_flat_zbuf,
	polyfill_alpha_gouraud_zbuf,
	0,
	polyfill_alpha_tex_wire,
	polyfill_alpha_tex_flat_zbuf,
	polyfill_alpha_tex_gouraud_zbuf,
	0,
	polyfill_add_wire,
	polyfill_add_flat_zbuf,
	polyfill_add_gouraud_zbuf,
	0,
	polyfill_add_tex_wire,
	polyfill_add_tex_flat_zbuf,
	polyfill_add_tex_gouraud_zbuf,
	0, 0, 0, 0, 0, 0, 0, 0, 0
};

struct pimage pfill_fb, pfill_tex;
uint16_t *pfill_zbuf;

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

void polyfill(int mode, struct pvertex *verts, int nverts)
{
#ifndef NDEBUG
	if(!fillfunc[mode]) {
		fprintf(stderr, "polyfill mode %d not implemented\n", mode);
		abort();
	}
#endif

	fillfunc[mode](verts, nverts);
}

void polyfill_wire(struct pvertex *verts, int nverts)
{
	int i, x0, y0, x1, y1;
	struct pvertex *v = verts;
	unsigned short color = ((v->r << 8) & 0xf800) |
		((v->g << 3) & 0x7e0) | ((v->b >> 3) & 0x1f);

	for(i=0; i<nverts - 1; i++) {
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

void polyfill_tex_wire(struct pvertex *verts, int nverts)
{
	polyfill_wire(verts, nverts);	/* TODO */
}

void polyfill_alpha_wire(struct pvertex *verts, int nverts)
{
	polyfill_wire(verts, nverts);	/* TODO */
}

void polyfill_alpha_tex_wire(struct pvertex *verts, int nverts)
{
	polyfill_wire(verts, nverts);	/* TODO */
}

void polyfill_add_wire(struct pvertex *verts, int nverts)
{
	polyfill_wire(verts, nverts);	/* TODO */
}

void polyfill_add_tex_wire(struct pvertex *verts, int nverts)
{
	polyfill_wire(verts, nverts);	/* TODO */
}

#define NEXTIDX(x) (((x) - 1 + nverts) % nverts)
#define PREVIDX(x) (((x) + 1) % nverts)

/* XXX
 * When HIGH_QUALITY is defined, the rasterizer calculates slopes for attribute
 * interpolation on each scanline separately; otherwise the slope for each
 * attribute would be calculated once for the whole polygon, which is faster,
 * but produces some slight quantization artifacts, due to the limited precision
 * of fixed-point calculations.
 */
#define HIGH_QUALITY

/* extra bits of precision to use when interpolating colors.
 * try tweaking this if you notice strange quantization artifacts.
 */
#define COLOR_SHIFT	12


#define POLYFILL polyfill_flat
#define SCANEDGE scanedge_flat
#undef GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_gouraud
#define SCANEDGE scanedge_gouraud
#define GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_tex_flat
#define SCANEDGE scanedge_tex_flat
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_tex_gouraud
#define SCANEDGE scanedge_tex_gouraud
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_alpha_flat
#define SCANEDGE scanedge_alpha_flat
#undef GOURAUD
#undef TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_alpha_gouraud
#define SCANEDGE scanedge_alpha_gouraud
#define GOURAUD
#undef TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_alpha_tex_flat
#define SCANEDGE scanedge_alpha_tex_flat
#undef GOURAUD
#define TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_alpha_tex_gouraud
#define SCANEDGE scanedge_alpha_tex_gouraud
#define GOURAUD
#define TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_add_flat
#define SCANEDGE scanedge_add_flat
#undef GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_add_gouraud
#define SCANEDGE scanedge_add_gouraud
#define GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_add_tex_flat
#define SCANEDGE scanedge_add_tex_flat
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_add_tex_gouraud
#define SCANEDGE scanedge_add_tex_gouraud
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

/* ---- zbuffer variants ----- */

#define POLYFILL polyfill_flat_zbuf
#define SCANEDGE scanedge_flat_zbuf
#undef GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_gouraud_zbuf
#define SCANEDGE scanedge_gouraud_zbuf
#define GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_tex_flat_zbuf
#define SCANEDGE scanedge_tex_flat_zbuf
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_tex_gouraud_zbuf
#define SCANEDGE scanedge_tex_gouraud_zbuf
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_alpha_flat_zbuf
#define SCANEDGE scanedge_alpha_flat_zbuf
#undef GOURAUD
#undef TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_alpha_gouraud_zbuf
#define SCANEDGE scanedge_alpha_gouraud_zbuf
#define GOURAUD
#undef TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_alpha_tex_flat_zbuf
#define SCANEDGE scanedge_alpha_tex_flat_zbuf
#undef GOURAUD
#define TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_alpha_tex_gouraud_zbuf
#define SCANEDGE scanedge_alpha_tex_gouraud_zbuf
#define GOURAUD
#define TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_add_flat_zbuf
#define SCANEDGE scanedge_add_flat_zbuf
#undef GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_add_gouraud_zbuf
#define SCANEDGE scanedge_add_gouraud_zbuf
#define GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_add_tex_flat_zbuf
#define SCANEDGE scanedge_add_tex_flat_zbuf
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_add_tex_gouraud_zbuf
#define SCANEDGE scanedge_add_tex_gouraud_zbuf
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE


#define VNEXT(p)	(((p) == vlast) ? varr : (p) + 1)
#define VPREV(p)	((p) == varr ? vlast : (p) - 1)
#define VSUCC(p, side)	((side) == 0 ? VNEXT(p) : VPREV(p))

void polyfill_flat_new(struct pvertex *varr, int vnum)
{
	int i, line, top, bot;
	struct pvertex *vlast, *v, *vn, *tab;
	int32_t x, y0, y1, dx, dy, slope, fx, fy;
	int start, len;
	g3d_pixel *fbptr, *pptr;

	g3d_pixel color = G3D_PACK_RGB(varr[0].r, varr[0].g, varr[0].b);

	vlast = varr + vnum - 1;
	top = pfill_fb.height;
	bot = 0;

	for(i=0; i<vnum; i++) {
		v = varr + i;
		vn = VNEXT(v);

		if(vn->y == v->y) continue;

		if(vn->y > v->y) {
			tab = left;
		} else {
			tab = right;
			v = vn;
			vn = varr + i;
		}

		dx = vn->x - v->x;
		dy = vn->y - v->y;
		slope = (dx << 8) / dy;

		y0 = (v->y + 0x100) & 0xffffff00;	/* start from the next scanline */
		fy = y0 - v->y;						/* fractional part before the next scanline */
		fx = (fy * slope) >> 8;				/* X adjust for the step to the next scanline */
		x = v->x + fx;						/* adjust X */
		y1 = vn->y & 0xffffff00;			/* last scanline of the edge <= vn->y */

		line = y0 >> 8;
		if(line < top) top = line;
		if((y1 >> 8) > bot) bot = y1 >> 8;

		if(line > 0) tab += line;

		while(line <= (y1 >> 8) && line < pfill_fb.height) {
			if(line >= 0) {
				int val = x < 0 ? 0 : x >> 8;
				tab->x = val < pfill_fb.width ? val : pfill_fb.width - 1;
				tab++;
			}
			x += slope;
			line++;
		}
	}

	if(top < 0) top = 0;
	if(bot >= pfill_fb.height) bot = pfill_fb.height - 1;

	fbptr = pfill_fb.pixels + top * pfill_fb.width;
	for(i=top; i<=bot; i++) {
		start = left[i].x;
		len = right[i].x - start;

		pptr = fbptr + start;
		while(len-- > 0) {
#ifdef DEBUG_OVERDRAW
			*pptr++ += DEBUG_OVERDRAW;
#else
			*pptr++ = color;
#endif
		}
		fbptr += pfill_fb.width;
	}
}
