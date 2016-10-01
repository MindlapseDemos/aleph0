#include <stdio.h>
#include <stdlib.h>
#include "polyfill.h"
#include "gfxutil.h"
#include "demo.h"

void (*fillfunc[])(struct pvertex*, int) = {
	polyfill_wire,
	polyfill_flat,
	0, 0, 0
};

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
		if(clip_line(&x0, &y0, &x1, &y1, 0, 0, fb_width, fb_height)) {
			draw_line(x0, y0, x1, y1, color);
		}
	}
	x0 = verts[0].x >> 8;
	y0 = verts[0].y >> 8;
	if(clip_line(&x1, &y1, &x0, &y0, 0, 0, fb_width, fb_height)) {
		draw_line(x1, y1, x0, y0, color);
	}
}

#define NEXTIDX(x) (((x) - 1 + nverts) % nverts)
#define PREVIDX(x) (((x) + 1) % nverts)

#define CALC_EDGE(which) \
	do { \
		which##_x = pv[which##_beg].x; \
		which##_dx = pv[which##_end].x - pv[which##_beg].x; \
		which##_slope = (which##_dx << 8) / which##_dy; \
	} while(0)

void polyfill_flat(struct pvertex *pv, int nverts)
{
	int i, sline, x, slen, top = 0;
	int left_beg, left_end, right_beg, right_end;
	int32_t left_dy, left_dx, right_dy, right_dx;
	int32_t left_slope, right_slope;
	int32_t left_x, right_x, y;
	uint16_t color = ((pv->r << 8) & 0xf800) | ((pv->g << 3) & 0x7e0) |
		((pv->b >> 3) & 0x1f);
	uint16_t *pixptr;

	/* find topmost */
	for(i=1; i<nverts; i++) {
		if(pv[i].y < pv[top].y) {
			top = i;
		}
	}
	left_beg = right_beg = top;
	left_end = PREVIDX(left_beg);
	right_end = NEXTIDX(right_beg);

	if((left_dy = pv[left_end].y - pv[left_beg].y)) {
		CALC_EDGE(left);
	}

	if((right_dy = pv[right_end].y - pv[right_beg].y)) {
		CALC_EDGE(right);
	}

	y = pv[top].y;
	sline = pv[top].y >> 8;

	for(;;) {
		if(y >= pv[left_end].y) {
			while(y >= pv[left_end].y) {
				left_beg = left_end;
				if(left_beg == right_beg) return;
				left_end = PREVIDX(left_end);
			}

			left_dy = pv[left_end].y - pv[left_beg].y;
			CALC_EDGE(left);
		}

		if(y >= pv[right_end].y) {
			while(y >= pv[right_end].y) {
				right_beg = right_end;
				if(left_beg == right_beg) return;
				right_end = NEXTIDX(right_end);
			}

			right_dy = pv[right_end].y - pv[right_beg].y;
			CALC_EDGE(right);
		}

		x = left_x >> 8;
		slen = (right_x >> 8) - (left_x >> 8);

		pixptr = (uint16_t*)fb_pixels + sline * fb_width + x;
		for(i=0; i<slen; i++) {
			*pixptr++ += 10;
		}

		++sline;
		y += 256;
		left_x += left_slope;
		right_x += right_slope;
	}
}
