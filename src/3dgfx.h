#ifndef THREEDGFX_H_
#define THREEDGFX_H_

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

/* 2nd arg to g3d_draw: which space are input verts in. skips parts of the pipeline */
enum {
	G3D_LOCAL_SPACE, /* this being 0 makes a nice default arg. */
	G3D_WORLD_SPACE, /* ignore world matrix */
	G3D_VIEW_SPACE,  /* ignore view matrix */
	G3D_CLIP_SPACE,  /* ignore projection matrix */
	G3D_SCREEN_SPACE,/* 2D verts, don't divide by w */
	G3D_PIXEL_SPACE  /* in pixel units, ignore viewport */
};

/* matrix stacks */
enum {
	G3D_WORLD,
	G3D_VIEW,
	G3D_PROJECTION,

	G3D_NUM_MATRICES
};

void g3d_init(void);

void g3d_framebuffer(int width, int height, void *pixels);

void g3d_enable(unsigned int opt);
void g3d_disable(unsigned int opt);
void g3d_setopt(unsigned int opt, unsigned int mask);
unsigned int g3d_getopt(unsigned int mask);

void g3d_set_matrix(int which, const float *m); /* null ptr for identity */
void g3d_mult_matrix(int which, const float *m);
void g3d_push_matrix(int which);
void g3d_pop_matrix(int which);

void g3d_translate(float x, float y, float z);
void g3d_rotate(float angle, float x, float y, float z);
void g3d_scale(float x, float y, float z);
void g3d_ortho(float left, float right, float bottom, float top, float znear, float zfar);
void g3d_frustum(float left, float right, float bottom, float top, float znear, float zfar);
void g3d_perspective(float vfov, float aspect, float znear, float zfar);

void g3d_draw(int prim, int space, const struct g3d_vertex *varr, int num);

#endif	/* THREEDGFX_H_ */
