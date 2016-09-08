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
	unsigned char r, g, b;
};

void polyfill(int mode, struct pvertex *verts, int nverts);
void polyfill_wire(struct pvertex *verts, int nverts);
void polyfill_flat(struct pvertex *verts, int nverts);

#endif	/* POLYFILL_H_ */
