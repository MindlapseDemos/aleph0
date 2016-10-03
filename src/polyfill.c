#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(__WATCOMC__) || defined(_MSC_VER)
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
	0, 0, 0
};

struct pimage pimg_fb, pimg_texture;

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
		if(clip_line(&x0, &y0, &x1, &y1, 0, 0, pimg_fb.width, pimg_fb.height)) {
			draw_line(x0, y0, x1, y1, color);
		}
	}
	x0 = verts[0].x >> 8;
	y0 = verts[0].y >> 8;
	if(clip_line(&x1, &y1, &x0, &y0, 0, 0, pimg_fb.width, pimg_fb.height)) {
		draw_line(x1, y1, x0, y0, color);
	}
}

static uint32_t scan_edge(struct pvertex *v0, struct pvertex *v1, struct pvertex *edge);

#define NEXTIDX(x) (((x) - 1 + nverts) % nverts)
#define PREVIDX(x) (((x) + 1) % nverts)

void polyfill_flat(struct pvertex *pv, int nverts)
{
	int i;
	int topidx = 0, botidx = 0, sltop = pimg_fb.height, slbot = 0;
	struct pvertex *left, *right;
	uint16_t color = PACK_RGB16(pv[0].r, pv[0].g, pv[0].b);

	for(i=1; i<nverts; i++) {
		if(pv[i].y < pv[topidx].y) topidx = i;
		if(pv[i].y > pv[botidx].y) botidx = i;
	}

	left = (struct pvertex*)alloca(pimg_fb.height * sizeof *left);
	right = (struct pvertex*)alloca(pimg_fb.height * sizeof *right);
	memset(left, 0, pimg_fb.height * sizeof *left);
	memset(right, 0, pimg_fb.height * sizeof *right);

	for(i=0; i<nverts; i++) {
		int next = NEXTIDX(i);
		int32_t y0 = pv[i].y;
		int32_t y1 = pv[next].y;

		if((y0 >> 8) == (y1 >> 8)) {
			if(y0 > y1) {
				int idx = y0 >> 8;
				left[idx].x = pv[i].x < pv[next].x ? pv[i].x : pv[next].x;
				right[idx].x = pv[i].x < pv[next].x ? pv[next].x : pv[i].x;
			}
		} else {
			struct pvertex *edge = y0 > y1 ? left : right;
			uint32_t res = scan_edge(pv + i, pv + next, edge);
			uint32_t tmp = (res >> 16) & 0xffff;
			if(tmp > slbot) slbot = tmp;
			if((tmp = res & 0xffff) < sltop) {
				sltop = tmp;
			}
		}
	}

	for(i=sltop; i<=slbot; i++) {
		int32_t x;
		uint16_t *pixptr;

		x = left[i].x;
		pixptr = pimg_fb.pixels + i * pimg_fb.width + (x >> 8);

		while(x <= right[i].x) {
#ifdef DEBUG_POLYFILL
			*pixptr++ += 15;
#else
			*pixptr++ = color;
#endif
			x += 256;
		}
	}
}

static uint32_t scan_edge(struct pvertex *v0, struct pvertex *v1, struct pvertex *edge)
{
	int i;
	int32_t x, dx, dy, slope;
	int32_t start_idx, end_idx;

	if(v0->y > v1->y) {
		struct pvertex *tmp = v0;
		v0 = v1;
		v1 = tmp;
	}

	dy = v1->y - v0->y;
	dx = v1->x - v0->x;
	slope = (dx << 8) / dy;

	start_idx = v0->y >> 8;
	end_idx = v1->y >> 8;

	x = v0->x;
	for(i=start_idx; i<end_idx; i++) {
		edge[i].x = x;
		x += slope;
	}

	return (uint32_t)start_idx | ((uint32_t)(end_idx - 1) << 16);
}
