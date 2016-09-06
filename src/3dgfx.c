#include <stdio.h>
#include <math.h>
#include <string.h>
#include "3dgfx.h"
#include "polyfill.h"

#define STACK_SIZE	8
typedef float g3d_matrix[16];

struct g3d_state {
	unsigned int opt;

	g3d_matrix mat[G3D_NUM_MATRICES][STACK_SIZE];
	int mtop[G3D_NUM_MATRICES];

	int width, height;
	void *pixels;
};

static void xform4_vec3(const float *mat, float *vec);
static void xform3_vec3(const float *mat, float *vec);
static void proc_vertex(struct pvertex *res, const struct g3d_vertex *vert, int space);

static struct g3d_state st;
static const float idmat[] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

void g3d_init(void)
{
	int i;

	memset(&st, 0, sizeof st);

	for(i=0; i<G3D_NUM_MATRICES; i++) {
		g3d_set_matrix(i, 0);
	}
}

void g3d_framebuffer(int width, int height, void *pixels)
{
	st.width = width;
	st.height = height;
	st.pixels = pixels;
}

void g3d_enable(unsigned int opt)
{
	st.opt |= opt;
}

void g3d_disable(unsigned int opt)
{
	st.opt &= ~opt;
}

void g3d_setopt(unsigned int opt, unsigned int mask)
{
	st.opt = (st.opt & ~mask) | (opt & mask);
}

unsigned int g3d_getopt(unsigned int mask)
{
	return st.opt & mask;
}

void g3d_set_matrix(int which, const float *m)
{
	int top = st.mtop[which];

	if(!m) m = idmat;
	memcpy(st.mat[which][top], m, 16 * sizeof(float));
}

#define M(i,j)	(((i) << 2) + (j))
void g3d_mult_matrix(int which, const float *m2)
{
	int i, j, top = st.mtop[which];
	float m1[16];
	float *dest = st.mat[which][top];

	memcpy(m1, dest, sizeof m1);

	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			*dest++ = m1[M(0,j)] * m2[M(i,0)] +
				m1[M(1,j)] * m2[M(i,1)] +
				m1[M(2,j)] * m2[M(i,2)] +
				m1[M(3,j)] * m2[M(i,3)];
		}
	}
}

void g3d_push_matrix(int which)
{
	int top = st.mtop[which];
	if(top >= G3D_NUM_MATRICES) {
		fprintf(stderr, "g3d_push_matrix overflow\n");
		return;
	}
	memcpy(st.mat[which][top + 1], st.mat[which][top], 16 * sizeof(float));
	st.mtop[which] = top + 1;;
}

void g3d_pop_matrix(int which)
{
	if(st.mtop[which] <= 0) {
		fprintf(stderr, "g3d_pop_matrix underflow\n");
		return;
	}
	--st.mtop[which];
}

void g3d_translate(int which, float x, float y, float z)
{
	float m[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
	m[12] = x;
	m[13] = y;
	m[14] = z;
	g3d_mult_matrix(which, m);
}

void g3d_rotate(int which, float deg, float x, float y, float z)
{
	float m[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	float angle = M_PI * deg / 180.0f;
	float sina = sin(angle);
	float cosa = cos(angle);
	float one_minus_cosa = 1.0f - cosa;
	float nxsq = x * x;
	float nysq = y * y;
	float nzsq = z * z;

	m[0] = nxsq + (1.0f - nxsq) * cosa;
	m[4] = x * y * one_minus_cosa - z * sina;
	m[8] = x * z * one_minus_cosa + y * sina;
	m[1] = x * y * one_minus_cosa + z * sina;
	m[5] = nysq + (1.0 - nysq) * cosa;
	m[9] = y * z * one_minus_cosa - x * sina;
	m[2] = x * z * one_minus_cosa - y * sina;
	m[6] = y * z * one_minus_cosa + x * sina;
	m[10] = nzsq + (1.0 - nzsq) * cosa;

	g3d_mult_matrix(which, m);
}

void g3d_scale(int which, float x, float y, float z)
{
	float m[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	m[0] = x;
	m[5] = y;
	m[10] = z;
	g3d_mult_matrix(which, m);
}

void g3d_ortho(int which, float left, float right, float bottom, float top, float znear, float zfar)
{
	float m[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	float dx = right - left;
	float dy = top - bottom;
	float dz = zfar - znear;

	m[0] = 2.0 / dx;
	m[5] = 2.0 / dy;
	m[10] = -2.0 / dz;
	m[12] = -(right + left) / dx;
	m[13] = -(top + bottom) / dy;
	m[14] = -(zfar + znear) / dz;

	g3d_mult_matrix(which, m);
}

void g3d_frustum(int which, float left, float right, float bottom, float top, float nr, float fr)
{
	float m[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	float dx = right - left;
	float dy = top - bottom;
	float dz = fr - nr;

	float a = (right + left) / dx;
	float b = (top + bottom) / dy;
	float c = -(fr + nr) / dz;
	float d = -2.0 * fr * nr / dz;

	m[0] = 2.0 * nr / dx;
	m[5] = 2.0 * nr / dy;
	m[8] = a;
	m[9] = b;
	m[10] = c;
	m[11] = -1.0f;
	m[14] = d;

	g3d_mult_matrix(which, m);
}

void g3d_perspective(int which, float vfov_deg, float aspect, float znear, float zfar)
{
	float m[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	float vfov = M_PI * vfov_deg / 180.0f;
	float s = 1.0f / tan(vfov * 0.5f);
	float range = znear - zfar;

	m[0] = s / aspect;
	m[5] = s;
	m[10] = (znear + zfar) / range;
	m[11] = -1.0f;
	m[14] = 2.0f * znear * zfar / range;

	g3d_mult_matrix(which, m);
}

void g3d_draw(int prim, int space, const struct g3d_vertex *varr, int varr_size)
{
	int i;
	int vnum = prim; /* primitive vertex counts correspond to the enum values */

	while(varr_size >= vnum) {
		struct pvertex pv[4];

		for(i=0; i<vnum; i++) {
			proc_vertex(pv + i, varr++, space);
		}

		polyfill_wire(pv, vnum);

		varr_size -= vnum;
	}
}

static void xform4_vec3(const float *mat, float *vec)
{
	float x = mat[0] * vec[0] + mat[4] * vec[1] + mat[8] * vec[2] + mat[12];
	float y = mat[1] * vec[0] + mat[5] * vec[1] + mat[9] * vec[2] + mat[13];
	float z = mat[2] * vec[0] + mat[6] * vec[1] + mat[10] * vec[2] + mat[14];
	float w = mat[3] * vec[0] + mat[7] * vec[1] + mat[11] * vec[2] + mat[15];

	vec[0] = x;
	vec[1] = y;
	vec[2] = z;
	vec[3] = w;
}

static void xform3_vec3(const float *mat, float *vec)
{
	float x = mat[0] * vec[0] + mat[4] * vec[1] + mat[8] * vec[2];
	float y = mat[1] * vec[0] + mat[5] * vec[1] + mat[9] * vec[2];
	float z = mat[2] * vec[0] + mat[6] * vec[1] + mat[10] * vec[2];

	vec[0] = x;
	vec[1] = y;
	vec[2] = z;
}

#define VEC3(v, x, y, z) do { v[0] = x; v[1] = y; v[2] = z; } while(0)
#define VEC4(v, x, y, z, w) do { v[0] = x; v[1] = y; v[2] = z; v[3] = w; } while(0)

static void proc_vertex(struct pvertex *res, const struct g3d_vertex *vert, int space)
{
	float pos[4];
	float norm[3];
	float color[3];
	int mvtop = st.mtop[G3D_MODELVIEW];
	int ptop = st.mtop[G3D_PROJECTION];
	g3d_matrix norm_mat;

	VEC4(pos, vert->x, vert->y, vert->z, 1.0f);
	VEC3(norm, vert->nx, vert->ny, vert->nz);

	switch(space) {
	case G3D_LOCAL_SPACE:
		memcpy(norm_mat, st.mat[G3D_MODELVIEW][mvtop], 16 * sizeof(float));
		norm_mat[12] = norm_mat[13] = norm_mat[14] = 0.0f;

		xform4_vec3(pos, st.mat[G3D_MODELVIEW][mvtop]);
		xform3_vec3(norm, norm_mat);

	case G3D_VIEW_SPACE:
		if(st.opt & G3D_LIGHTING) {
			/* TODO lighting */
			color[0] = vert->r;
			color[1] = vert->g;
			color[2] = vert->b;
		}
		xform4_vec3(pos, st.mat[G3D_PROJECTION][ptop]);

	case G3D_CLIP_SPACE:
		/* TODO clipping */
		if(pos[3] != 0.0f) {
			pos[0] /= pos[3];
			pos[1] /= pos[3];
			pos[2] /= pos[3];
		}

	case G3D_SCREEN_SPACE:
		pos[0] = (pos[0] * 0.5 + 0.5) * (float)st.width;
		pos[1] = (0.5 - pos[0] * 0.5) * (float)st.height;

	case G3D_PIXEL_SPACE:
		break;
	}

	/* convert pos to 24.8 fixed point */
	res->x = (int32_t)(pos[0] * 256.0f);
	res->y = (int32_t)(pos[1] * 256.0f);

	/* convert tex coords to 16.16 fixed point */
	res->u = (int32_t)(vert->u * 65536.0f);
	res->v = (int32_t)(vert->v * 65536.0f);

	/* pass color through as is */
	res->r = color[0];
	res->g = color[1];
	res->b = color[2];
}
