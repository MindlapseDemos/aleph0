#ifndef POLYFILL_H_
#define POLYFILL_H_

#include "inttypes.h"
#include "3dgfx.h"

#define POLYFILL_MODE_MASK	0x03
#define POLYFILL_TEX_MASK	0x0c
#define POLYFILL_TEXMODE_SHIFT	2
#define POLYFILL_ALPHA_BIT	0x10
#define POLYFILL_ADD_BIT	0x20
#define POLYFILL_ZBUF_BIT	0x40

enum {
	POLYFILL_WIRE,
	POLYFILL_FLAT,
	POLYFILL_GOURAUD
};

enum {
	POLYFILL_NOTEX,
	POLYFILL_TEXMOD,
	POLYFILL_TEXADD,
	POLYFILL_TEXREPL
};

/* projected vertices for the rasterizer */
struct pvertex {
	int32_t x, y; /* 24.8 fixed point */
	int32_t u, v; /* 16.16 fixed point */
	int32_t r, g, b, a;  /* int 0-255 */
	int32_t z;	/* 0-(2^24-1) */
};

struct pgradient {
	int32_t dudx, dudy, dvdx, dvdy;
	int32_t drdx, drdy, dgdx, dgdy, dbdx, dbdy, dadx, dady;
	int32_t dzdx, dzdy;
};

struct pimage {
	g3d_pixel *pixels;
	int width, height, scanlen;

	int xshift, yshift;
	unsigned int xmask, ymask;
};

extern struct pimage pfill_fb;
extern struct pimage pfill_tex;
extern uint32_t *pfill_zbuf;
extern struct pgradient pgrad;

void polyfill_fbheight(int height);

void polyfill(int mode, struct pvertex *verts);

void polyfill_wire(struct pvertex *verts);
void polyfill_flat(struct pvertex *verts);
void polyfill_gour(struct pvertex *verts);
void polyfill_tex_wire(struct pvertex *verts);
void polyfill_tex_flat(struct pvertex *verts);
void polyfill_tex_gour(struct pvertex *verts);
void polyfill_alpha_wire(struct pvertex *verts);
void polyfill_alpha_flat(struct pvertex *verts);
void polyfill_alpha_gour(struct pvertex *verts);
void polyfill_alpha_tex_wire(struct pvertex *verts);
void polyfill_alpha_tex_flat(struct pvertex *verts);
void polyfill_alpha_tex_gour(struct pvertex *verts);
void polyfill_add_wire(struct pvertex *verts);
void polyfill_add_flat(struct pvertex *verts);
void polyfill_add_gour(struct pvertex *verts);
void polyfill_add_tex_wire(struct pvertex *verts);
void polyfill_add_tex_flat(struct pvertex *verts);
void polyfill_add_tex_gour(struct pvertex *verts);
void polyfill_flat_zbuf(struct pvertex *verts);
void polyfill_gour_zbuf(struct pvertex *verts);
void polyfill_tex_flat_zbuf(struct pvertex *verts);
void polyfill_tex_gour_zbuf(struct pvertex *verts);
void polyfill_alpha_flat_zbuf(struct pvertex *verts);
void polyfill_alpha_gour_zbuf(struct pvertex *verts);
void polyfill_alpha_tex_flat_zbuf(struct pvertex *verts);
void polyfill_alpha_tex_gour_zbuf(struct pvertex *verts);
void polyfill_add_flat_zbuf(struct pvertex *verts);
void polyfill_add_gour_zbuf(struct pvertex *verts);
void polyfill_add_tex_flat_zbuf(struct pvertex *verts);
void polyfill_add_tex_gour_zbuf(struct pvertex *verts);

void polyfill_addtex_flat(struct pvertex *verts);
void polyfill_addtex_gour(struct pvertex *verts);
void polyfill_addtex_flat_zbuf(struct pvertex *verts);
void polyfill_addtex_gour_zbuf(struct pvertex *verts);

void polyfill_repltex_flat(struct pvertex *verts);
void polyfill_repltex_flat_zbuf(struct pvertex *verts);

#endif	/* POLYFILL_H_ */
