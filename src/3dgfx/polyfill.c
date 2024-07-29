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
 *     bit 2-3: texture mode: 00-none 01-modulate 10-add 11-replace
 *     bit 4-5: blend mode: 00-none 01-alpha 10-additive 11-reserved
 *     bit 6: zbuffering
 */
void (*fillfunc[])(struct pvertex*) = {											/* zbbttpp */
	polyfill_wire, polyfill_flat, polyfill_gour, 0,								/* 00000xx */
	polyfill_tex_wire, polyfill_tex_flat, polyfill_tex_gour, 0,					/* 00001xx */
	0, polyfill_addtex_flat, polyfill_addtex_gour, 0,							/* 00010xx */
	0, polyfill_repltex_flat, polyfill_repltex_flat, 0,							/* 00011xx */

	polyfill_alpha_wire, polyfill_alpha_flat, polyfill_alpha_gour, 0,			/* 00100xx */
	polyfill_alpha_tex_wire, polyfill_alpha_tex_flat, polyfill_alpha_tex_gour,0,/* 00101xx */
	0, 0, 0, 0,																	/* 00110xx */
	0, 0, 0, 0,																	/* 00111xx */

	polyfill_add_wire, polyfill_add_flat, polyfill_add_gour, 0,					/* 01000xx */
	polyfill_add_tex_wire, polyfill_add_tex_flat, polyfill_add_tex_gour, 0,		/* 01001xx */
	0, 0, 0, 0,																	/* 01010xx */
	0, 0, 0, 0,																	/* 01011xx */

	0, 0, 0, 0,																	/* 01100xx */
	0, 0, 0, 0,																	/* 01101xx */
	0, 0, 0, 0,																	/* 01110xx */
	0, 0, 0, 0,																	/* 01111xx */

	polyfill_wire, polyfill_flat_zbuf, polyfill_gour_zbuf, 0,					/* 10000xx */
	polyfill_tex_wire, polyfill_tex_flat_zbuf, polyfill_tex_gour_zbuf, 0,		/* 10001xx */
	0, polyfill_addtex_flat_zbuf, polyfill_addtex_gour_zbuf, 0,					/* 10010xx */
	0, polyfill_repltex_flat_zbuf, polyfill_repltex_flat_zbuf, 0,				/* 10011xx */

	polyfill_alpha_wire, polyfill_alpha_flat_zbuf, polyfill_alpha_gour_zbuf, 0,	/* 10100xx */
	polyfill_alpha_tex_wire,polyfill_alpha_tex_flat_zbuf,polyfill_alpha_tex_gour_zbuf,0,/* 10101xx */
	0, 0, 0, 0,																	/* 10110xx */
	0, 0, 0, 0,																	/* 10111xx */

	polyfill_add_wire, polyfill_add_flat_zbuf, polyfill_add_gour_zbuf, 0,		/* 11000xx */
	polyfill_add_tex_wire, polyfill_add_tex_flat_zbuf, polyfill_add_tex_gour_zbuf, 0,	/* 11001xx */
	0, 0, 0, 0,																	/* 11010xx */
	0, 0, 0, 0,																	/* 11011xx */

	0, 0, 0, 0,																	/* 11100xx */
	0, 0, 0, 0,																	/* 11101xx */
	0, 0, 0, 0,																	/* 11110xx */
	0, 0, 0, 0,																	/* 11111xx */
};

struct pimage pfill_fb, pfill_tex;
uint32_t *pfill_zbuf;
struct pgradient pgrad;

#define EDGEPAD	8
static struct pvertex *edgebuf, *left, *right;
static int edgebuf_size;
/*static int fbheight;*/

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

	/*fbheight = height;*/
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

#define POLYFILL polyfill_gour
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

#define POLYFILL polyfill_tex_gour
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

#define POLYFILL polyfill_alpha_gour
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

#define POLYFILL polyfill_alpha_tex_gour
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

#define POLYFILL polyfill_add_gour
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

#define POLYFILL polyfill_add_tex_gour
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

#define POLYFILL polyfill_gour_zbuf
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

#define POLYFILL polyfill_tex_gour_zbuf
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

#define POLYFILL polyfill_alpha_gour_zbuf
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

#define POLYFILL polyfill_alpha_tex_gour_zbuf
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

#define POLYFILL polyfill_add_gour_zbuf
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

#define POLYFILL polyfill_add_tex_gour_zbuf
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

#define POLYFILL polyfill_addtex_gour
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

#define POLYFILL polyfill_addtex_gour_zbuf
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#define TEX_ADD
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_repltex_flat
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#undef TEX_ADD
#define TEX_REPL
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_repltex_flat_zbuf
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#undef TEX_ADD
#define TEX_REPL
#include "polytmpl.h"
#undef POLYFILL
