#ifndef POLYFILL_H_
#define POLYFILL_H_

#include "inttypes.h"

enum {
	POLYFILL_WIRE,
	POLYFILL_FLAT,
	POLYFILL_GOURAUD,
	POLYFILL_TEX,
	POLYFILL_TEX_GOURAUD
};

/* projected vertices for the rasterizer */
struct pvertex {
	int32_t x, y; /* 24.8 fixed point */
	int32_t u, v; /* 16.16 fixed point */
	int32_t r, g, b, a;  /* int 0-255 */
};

struct pimage {
	uint16_t *pixels;
	int width, height;

	int xshift, yshift;
	unsigned int xmask, ymask;
};

extern struct pimage pfill_fb;
extern struct pimage pfill_tex;

void polyfill(int mode, struct pvertex *verts, int nverts);
void polyfill_wire(struct pvertex *verts, int nverts);
void polyfill_flat(struct pvertex *verts, int nverts);
void polyfill_gouraud(struct pvertex *verts, int nverts);
void polyfill_tex(struct pvertex *verts, int nverts);
void polyfill_tex_gouraud(struct pvertex *verts, int nverts);

#endif	/* POLYFILL_H_ */
