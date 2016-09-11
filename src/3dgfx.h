#ifndef THREEDGFX_H_
#define THREEDGFX_H_

#include "inttypes.h"

struct g3d_vertex {
	float x, y, z, w;
	float nx, ny, nz;
	float u, v;
	unsigned char r, g, b;
};

enum {
	G3D_POINTS = 1,
	G3D_LINES = 2,
	G3D_TRIANGLES = 3,
	G3D_QUADS = 4
};

/* g3d_enable/g3d_disable bits */
enum {
	G3D_CULL_FACE = 1,
	G3D_DEPTH_TEST = 2,	/* XXX not implemented */
	G3D_LIGHTING = 4,
	G3D_TEXTURE = 8,

	G3D_ALL = 0x7fffffff
};

/* arg to g3d_front_face */
enum { G3D_CCW, G3D_CW };

/* matrix stacks */
enum {
	G3D_MODELVIEW,
	G3D_PROJECTION,

	G3D_NUM_MATRICES
};

int g3d_init(void);
void g3d_destroy(void);

void g3d_framebuffer(int width, int height, void *pixels);

void g3d_enable(unsigned int opt);
void g3d_disable(unsigned int opt);
void g3d_setopt(unsigned int opt, unsigned int mask);
unsigned int g3d_getopt(unsigned int mask);

void g3d_front_face(unsigned int order);

void g3d_matrix_mode(int mmode);

void g3d_load_identity(void);
void g3d_load_matrix(const float *m);
void g3d_mult_matrix(const float *m);
void g3d_push_matrix(void);
void g3d_pop_matrix(void);

void g3d_translate(float x, float y, float z);
void g3d_rotate(float angle, float x, float y, float z);
void g3d_scale(float x, float y, float z);
void g3d_ortho(float left, float right, float bottom, float top, float znear, float zfar);
void g3d_frustum(float left, float right, float bottom, float top, float znear, float zfar);
void g3d_perspective(float vfov, float aspect, float znear, float zfar);

const float *g3d_get_matrix(int which, float *m);

void g3d_draw(int prim, const struct g3d_vertex *varr, int varr_size);
void g3d_draw_indexed(int prim, const struct g3d_vertex *varr, int varr_size,
		const int16_t *iarr, int iarr_size);

#endif	/* THREEDGFX_H_ */
