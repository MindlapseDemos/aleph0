#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(__WATCOMC__) || defined(_MSC_VER) || defined(__DJGPP__)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "polyfill.h"
#include "gfxutil.h"
#include "demo.h"

void (*fillfunc[])(struct pvertex*, int) = {
	polyfill_wire,
	polyfill_flat,
	polyfill_gouraud,
	polyfill_tex,
	polyfill_tex_gouraud
};

struct pimage pfill_fb, pfill_tex;

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
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_gouraud
#define SCANEDGE scanedge_gouraud
#define GOURAUD
#undef TEXMAP
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_tex
#define SCANEDGE scanedge_tex
#undef GOURAUD
#define TEXMAP
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE

#define POLYFILL polyfill_tex_gouraud
#define SCANEDGE scanedge_tex_gouraud
#define GOURAUD
#define TEXMAP
#include "polytmpl.h"
#undef POLYFILL
#undef SCANEDGE
