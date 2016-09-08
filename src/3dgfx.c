#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "3dgfx.h"
#include "polyfill.h"
#include "inttypes.h"

#define STACK_SIZE	8
typedef float g3d_matrix[16];

#define MAX_VBUF_SIZE	256

struct g3d_state {
	unsigned int opt;
	int frontface;

	g3d_matrix mat[G3D_NUM_MATRICES][STACK_SIZE];
	int mtop[G3D_NUM_MATRICES];
	int mmode;

	g3d_matrix norm_mat;

	int width, height;
	void *pixels;
};

static void xform4_vec3(const float *mat, float *vec);
static void xform3_vec3(const float *mat, float *vec);
static void shade(struct g3d_vertex *v);

static struct g3d_state *st;
static const float idmat[] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

int g3d_init(void)
{
	int i;

	if(!(st = calloc(1, sizeof *st))) {
		fprintf(stderr, "failed to allocate G3D context\n");
		return -1;
	}

	for(i=0; i<G3D_NUM_MATRICES; i++) {
		g3d_matrix_mode(i);
		g3d_load_identity();
	}
	return 0;
}

void g3d_destroy(void)
{
	free(st);
}

void g3d_framebuffer(int width, int height, void *pixels)
{
	st->width = width;
	st->height = height;
	st->pixels = pixels;
}

void g3d_enable(unsigned int opt)
{
	st->opt |= opt;
}

void g3d_disable(unsigned int opt)
{
	st->opt &= ~opt;
}

void g3d_setopt(unsigned int opt, unsigned int mask)
{
	st->opt = (st->opt & ~mask) | (opt & mask);
}

unsigned int g3d_getopt(unsigned int mask)
{
	return st->opt & mask;
}

void g3d_front_face(unsigned int order)
{
	st->frontface = order;
}

void g3d_matrix_mode(int mmode)
{
	st->mmode = mmode;
}

void g3d_load_identity(void)
{
	int top = st->mtop[st->mmode];
	memcpy(st->mat[st->mmode][top], idmat, 16 * sizeof(float));
}

void g3d_load_matrix(const float *m)
{
	int top = st->mtop[st->mmode];
	memcpy(st->mat[st->mmode][top], m, 16 * sizeof(float));
}

#define M(i,j)	(((i) << 2) + (j))
void g3d_mult_matrix(const float *m2)
{
	int i, j, top = st->mtop[st->mmode];
	float m1[16];
	float *dest = st->mat[st->mmode][top];

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

void g3d_push_matrix(void)
{
	int top = st->mtop[st->mmode];
	if(top >= G3D_NUM_MATRICES) {
		fprintf(stderr, "g3d_push_matrix overflow\n");
		return;
	}
	memcpy(st->mat[st->mmode][top + 1], st->mat[st->mmode][top], 16 * sizeof(float));
	st->mtop[st->mmode] = top + 1;
}

void g3d_pop_matrix(void)
{
	if(st->mtop[st->mmode] <= 0) {
		fprintf(stderr, "g3d_pop_matrix underflow\n");
		return;
	}
	--st->mtop[st->mmode];
}

void g3d_translate(float x, float y, float z)
{
	float m[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
	m[12] = x;
	m[13] = y;
	m[14] = z;
	g3d_mult_matrix(m);
}

void g3d_rotate(float deg, float x, float y, float z)
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
	m[15] = 1.0f;

	g3d_mult_matrix(m);
}

void g3d_scale(float x, float y, float z)
{
	float m[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	m[0] = x;
	m[5] = y;
	m[10] = z;
	m[15] = 1.0f;
	g3d_mult_matrix(m);
}

void g3d_ortho(float left, float right, float bottom, float top, float znear, float zfar)
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

	g3d_mult_matrix(m);
}

void g3d_frustum(float left, float right, float bottom, float top, float nr, float fr)
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

	g3d_mult_matrix(m);
}

void g3d_perspective(float vfov_deg, float aspect, float znear, float zfar)
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

	g3d_mult_matrix(m);
}

#define CROSS(res, a, b) \
	do { \
		(res)[0] = (a)[1] * (b)[2] - (a)[2] * (b)[1]; \
		(res)[1] = (a)[2] * (b)[0] - (a)[0] * (b)[2]; \
		(res)[2] = (a)[0] * (b)[1] - (a)[1] * (b)[0]; \
	} while(0)

void g3d_draw(int prim, const struct g3d_vertex *varr, int varr_size)
{
	int i;
	struct pvertex pv[4];
	struct g3d_vertex v[4];
	int vnum = prim; /* primitive vertex counts correspond to enum values */
	int mvtop = st->mtop[G3D_MODELVIEW];
	int ptop = st->mtop[G3D_PROJECTION];

	/* calc the normal matrix */
	memcpy(st->norm_mat, st->mat[G3D_MODELVIEW][mvtop], 16 * sizeof(float));
	st->norm_mat[12] = st->norm_mat[13] = st->norm_mat[14] = 0.0f;

	while(varr_size >= vnum) {
		varr_size -= vnum;

		for(i=0; i<vnum; i++) {
			v[i] = *varr++;

			xform4_vec3(st->mat[G3D_MODELVIEW][mvtop], &v[i].x);
			xform3_vec3(st->norm_mat, &v[i].nx);

			if(st->opt & G3D_LIGHTING) {
				shade(v + i);
			}
			xform4_vec3(st->mat[G3D_PROJECTION][ptop], &v[i].x);
		}

		/* TODO clipping */

		for(i=0; i<vnum; i++) {
			if(v[i].w != 0.0f) {
				v[i].x /= v[i].w;
				v[i].y /= v[i].w;
				/*v[i].z /= v[i].w;*/
			}

			/* viewport transformation */
			v[i].x = (v[i].x * 0.5f + 0.5f) * (float)st->width;
			v[i].y = (0.5f - v[i].y * 0.5f) * (float)st->height;

			/* convert pos to 24.8 fixed point */
			pv[i].x = (int32_t)(v[i].x * 256.0f);
			pv[i].y = (int32_t)(v[i].y * 256.0f);
			/* convert tex coords to 16.16 fixed point */
			pv[i].u = (int32_t)(v[i].u * 65536.0f);
			pv[i].v = (int32_t)(v[i].v * 65536.0f);
			/* pass the color through as is */
			pv[i].r = v[i].r;
			pv[i].g = v[i].g;
			pv[i].b = v[i].b;
		}

		/* backface culling */
		if(vnum > 2 && st->opt & G3D_CULL_FACE) {
			int32_t ax = pv[1].x - pv[0].x;
			int32_t ay = pv[1].y - pv[0].y;
			int32_t bx = pv[2].x - pv[0].x;
			int32_t by = pv[2].y - pv[0].y;
			int32_t cross_z = ax * (by >> 8) - ay * (bx >> 8);
			int sign = (cross_z >> 31) & 1;

			if(!(sign ^ st->frontface)) {
				continue;	/* back-facing */
			}
		}

		polyfill_flat(pv, vnum);
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

static void shade(struct g3d_vertex *v)
{
	v->r = v->g = v->b = 255;
}
