#ifndef BUILD_OPENGL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "3dgfx.h"
#include "gfxutil.h"
#include "polyfill.h"
#include "polyclip.h"
#include "inttypes.h"
#include "demo.h"
#include "util.h"


#undef CORRECT_NORMAL_MATRIX
#ifdef CORRECT_NORMAL_MATRIX
#include <cgmath/cgmath.h>
#endif

#define ENABLE_ZBUFFER

#define STACK_SIZE	16
typedef float g3d_matrix[16];

#define MAX_LIGHTS		4

#define IMM_VBUF_SIZE	256

#define NORMALIZE(v) \
	do { \
		float len = sqrt((v)[0] * (v)[0] + (v)[1] * (v)[1] + (v)[2] * (v)[2]); \
		if(len != 0.0) { \
			float s = 1.0 / len; \
			(v)[0] *= s; \
			(v)[1] *= s; \
			(v)[2] *= s; \
		} \
	} while(0)

enum {LT_POS, LT_DIR};
struct light {
	int type;
	float x, y, z;
	float r, g, b;
};

struct material {
	float kd[3];
	float ks[3];
	float shin;
};

struct g3d_state {
	unsigned int opt;
	int frontface;
	int polymode, texmode;

	g3d_matrix mat[G3D_NUM_MATRICES][STACK_SIZE];
	int mtop[G3D_NUM_MATRICES];
	int mmode;

	g3d_matrix norm_mat;

	float ambient[3];
	struct light lt[MAX_LIGHTS];
	struct material mtl;

	int width, height;
	g3d_pixel *pixels;

	int vport[4];

	struct cplane clip[4];

	uint16_t clear_color;
	uint32_t clear_depth;

	/* immediate mode */
	int imm_prim;
	int imm_numv, imm_pcount;
	struct g3d_vertex imm_curv;
	struct g3d_vertex imm_vbuf[IMM_VBUF_SIZE];
};

static void calc_grad(struct g3d_vertex *v);

static void imm_flush(void);
static __inline void xform4_vec3(const float *mat, float *vec);
static __inline void xform3_vec3(const float *mat, float *vec);
static __inline void xform4_vec4(const float *mat, float *vec);

static struct g3d_state *st;
static const float idmat[] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

int g3d_init(void)
{
	if(!(st = calloc(1, sizeof *st))) {
		fprintf(stderr, "failed to allocate G3D context\n");
		return -1;
	}
	g3d_reset();

	return 0;
}

void g3d_destroy(void)
{
#ifdef ENABLE_ZBUFFER
	free(pfill_zbuf);
#endif
	free(st);
}

void g3d_reset(void)
{
	int i;

#ifdef ENABLE_ZBUFFER
	free(pfill_zbuf);
#endif
	memset(st, 0, sizeof *st);

	st->opt = G3D_CLIP_FRUSTUM;
	st->polymode = POLYFILL_FLAT;
	st->texmode = POLYFILL_TEXMOD;

	for(i=0; i<G3D_NUM_MATRICES; i++) {
		g3d_matrix_mode(i);
		g3d_load_identity();
	}

	for(i=0; i<MAX_LIGHTS; i++) {
		g3d_light_dir(i, 0, 0, 1);
		g3d_light_color(i, 1, 1, 1);
	}
	g3d_light_ambient(0.1, 0.1, 0.1);

	g3d_mtl_diffuse(1, 1, 1);

	st->clear_depth = 0xffffff;
}

void g3d_framebuffer(int width, int height, void *pixels, int scanlen)
{
	static int max_height;

#ifdef ENABLE_ZBUFFER
	static int max_npixels;
	int npixels = width * height;

	if(npixels > max_npixels) {
		free(pfill_zbuf);
		pfill_zbuf = malloc(npixels * sizeof *pfill_zbuf);
		max_npixels = npixels;
	}
#endif

	if(height > max_height) {
		polyfill_fbheight(height);
		max_height = height;
	}

	st->width = width;
	st->height = height;

	pfill_fb.pixels = pixels;
	pfill_fb.width = width;
	pfill_fb.height = height;
	pfill_fb.scanlen = scanlen;

	g3d_viewport(0, 0, width, height);
}

/* set the framebuffer pointer, without resetting the size */
void g3d_framebuffer_addr(void *pixels)
{
	pfill_fb.pixels = pixels;
}

void g3d_viewport(int x, int y, int w, int h)
{
	st->vport[0] = x;
	st->vport[1] = y;
	st->vport[2] = w;
	st->vport[3] = h;
}

void g3d_clear_color(unsigned char r, unsigned char g, unsigned char b)
{
	st->clear_color = PACK_RGB16(r, g, b);
}

void g3d_clear_depth(float z)
{
	int iz = (int)(z * (float)0xffffff);
	if(iz < 0) iz = 0;
	if(iz > 0xffffff) iz = 0xffffff;
	st->clear_depth = iz;
}

void g3d_clear(unsigned int mask)
{
	if(mask & G3D_COLOR_BUFFER_BIT) {
		memset16(pfill_fb.pixels, st->clear_color, pfill_fb.scanlen * pfill_fb.height);
	}
	if(mask & G3D_DEPTH_BUFFER_BIT) {
		memset16(pfill_zbuf, st->clear_depth, pfill_fb.width * pfill_fb.height * sizeof *pfill_zbuf / 2);
	}
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

void g3d_polygon_mode(int pmode)
{
	st->polymode = pmode;
}

int g3d_get_polygon_mode(void)
{
	return st->polymode;
}

void g3d_texture_mode(int tmode)
{
	st->texmode = tmode;
}

int g3d_get_texture_mode(void)
{
	return st->texmode;
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
	if(top >= STACK_SIZE) {
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
	float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
	m[12] = x;
	m[13] = y;
	m[14] = z;
	g3d_mult_matrix(m);
}

void g3d_rotate(float deg, float x, float y, float z)
{
	float m[16] = {0};

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

void g3d_rotate_x(float angle)
{
	float m[16] = {0};
	float sa = sin(angle);
	float ca = cos(angle);

	m[0] = m[15] = 1;
	m[5] = ca;
	m[6] = sa;
	m[9] = -sa;
	m[10] = ca;
	g3d_mult_matrix(m);
}

void g3d_rotate_y(float angle)
{
	float m[16] = {0};
	float sa = sin(angle);
	float ca = cos(angle);

	m[5] = m[15] = 1;
	m[0] = ca;
	m[2] = -sa;
	m[8] = sa;
	m[10] = ca;
	g3d_mult_matrix(m);
}

void g3d_rotate_z(float angle)
{
	float m[16] = {0};
	float sa = sin(angle);
	float ca = cos(angle);

	m[10] = m[15] = 1;
	m[0] = ca;
	m[1] = sa;
	m[4] = -sa;
	m[5] = ca;
	g3d_mult_matrix(m);
}

void g3d_scale(float x, float y, float z)
{
	float m[16] = {0};
	m[0] = x;
	m[5] = y;
	m[10] = z;
	m[15] = 1.0f;
	g3d_mult_matrix(m);
}

void g3d_ortho(float left, float right, float bottom, float top, float znear, float zfar)
{
	float m[16] = {0};

	float dx = right - left;
	float dy = top - bottom;
	float dz = zfar - znear;

	m[0] = 2.0 / dx;
	m[5] = 2.0 / dy;
	m[10] = -2.0 / dz;
	m[12] = -(right + left) / dx;
	m[13] = -(top + bottom) / dy;
	m[14] = -(zfar + znear) / dz;
	m[15] = 1.0f;

	g3d_mult_matrix(m);
}

void g3d_frustum(float left, float right, float bottom, float top, float nr, float fr)
{
	float m[16] = {0};

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
	float m[16] = {0};

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

const float *g3d_get_matrix(int which, float *m)
{
	int top = st->mtop[which];

	if(m) {
		memcpy(m, st->mat[which][top], 16 * sizeof(float));
	}
	return st->mat[which][top];
}

void g3d_light_pos(int idx, float x, float y, float z)
{
	int mvtop = st->mtop[G3D_MODELVIEW];

	st->lt[idx].type = LT_POS;
	st->lt[idx].x = x;
	st->lt[idx].y = y;
	st->lt[idx].z = z;

	xform4_vec3(st->mat[G3D_MODELVIEW][mvtop], &st->lt[idx].x);
}

void g3d_light_dir(int idx, float x, float y, float z)
{
	int mvtop = st->mtop[G3D_MODELVIEW];

	st->lt[idx].type = LT_DIR;
	st->lt[idx].x = x;
	st->lt[idx].y = y;
	st->lt[idx].z = z;

	/* calc the normal matrix */
#ifdef CORRECT_NORMAL_MATRIX
	memcpy(st->norm_mat, st->mat[G3D_MODELVIEW][mvtop], 16 * sizeof(float));
	cgm_minverse(st->norm_mat);
	cgm_mtranspose(st->norm_mat);
#else
	memcpy(st->norm_mat, st->mat[G3D_MODELVIEW][mvtop], 16 * sizeof(float));
	st->norm_mat[12] = st->norm_mat[13] = st->norm_mat[14] = 0.0f;
#endif

	xform4_vec3(st->norm_mat, &st->lt[idx].x);

	NORMALIZE(&st->lt[idx].x);
}

void g3d_light_color(int idx, float r, float g, float b)
{
	st->lt[idx].r = r;
	st->lt[idx].g = g;
	st->lt[idx].b = b;
}

void g3d_light_ambient(float r, float g, float b)
{
	st->ambient[0] = r;
	st->ambient[1] = g;
	st->ambient[2] = b;
}

void g3d_mtl_diffuse(float r, float g, float b)
{
	st->mtl.kd[0] = r;
	st->mtl.kd[1] = g;
	st->mtl.kd[2] = b;
}

void g3d_mtl_specular(float r, float g, float b)
{
	st->mtl.ks[0] = r;
	st->mtl.ks[1] = g;
	st->mtl.ks[2] = b;
}

void g3d_mtl_shininess(float shin)
{
	st->mtl.shin = shin;
}

static INLINE int calc_mask(unsigned int x)
{
	return x - 1;
}

void g3d_set_texture(int xsz, int ysz, void *pixels)
{
	pfill_tex.pixels = pixels;
	pfill_tex.width = pfill_tex.scanlen = xsz;
	pfill_tex.height = ysz;

	pfill_tex.xshift = calc_shift(xsz);
	pfill_tex.yshift = calc_shift(ysz);
	pfill_tex.xmask = calc_mask(xsz);
	pfill_tex.ymask = calc_mask(ysz);
}

void g3d_draw(int prim, const struct g3d_vertex *varr, int varr_size)
{
	g3d_draw_indexed(prim, varr, varr_size, 0, 0);
}

#define NEED_NORMALS	(st->opt & (G3D_LIGHTING | G3D_TEXTURE_GEN))

void g3d_draw_indexed(int prim, const struct g3d_vertex *varr, int varr_size,
		const uint16_t *iarr, int iarr_size)
{
	int i, j, vnum, nfaces, fill_mode, num_tri;
	struct pvertex pv[16], *pvtri;
	struct g3d_vertex v[16], *vtri;
	int mvtop = st->mtop[G3D_MODELVIEW];
	int ptop = st->mtop[G3D_PROJECTION];
	struct g3d_vertex *tmpv;

	tmpv = alloca(prim * 6 * sizeof *tmpv);

	/* calc the normal matrix */
	if(NEED_NORMALS) {
#ifdef CORRECT_NORMAL_MATRIX
		memcpy(st->norm_mat, st->mat[G3D_MODELVIEW][mvtop], 16 * sizeof(float));
		cgm_minverse(st->norm_mat);
		cgm_mtranspose(st->norm_mat);
#else
		memcpy(st->norm_mat, st->mat[G3D_MODELVIEW][mvtop], 16 * sizeof(float));
		st->norm_mat[12] = st->norm_mat[13] = st->norm_mat[14] = 0.0f;
#endif
	}

	nfaces = (iarr ? iarr_size : varr_size) / prim;

	for(j=0; j<nfaces; j++) {
		vnum = prim;	/* reset vnum for each iteration */

		for(i=0; i<vnum; i++) {
			v[i] = iarr ? varr[*iarr++] : *varr++;
			xform4_vec3(st->mat[G3D_MODELVIEW][mvtop], &v[i].x);
		}

		if(st->opt & G3D_CLIP_PLANE0) {
			int cnum;
			int res = clip_poly(tmpv, &cnum, v, vnum, st->clip);
			if(res == -1) continue;	/* all outside */
			if(res == 0) {
				/* clipped, replace with the clipped polygon */
				memcpy(v, tmpv, cnum * sizeof *v);
				vnum = cnum;
			}
		}

		for(i=0; i<vnum; i++) {
			if(NEED_NORMALS) {
				xform3_vec3(st->norm_mat, &v[i].nx);
				if(st->opt & G3D_LIGHTING) {
					g3d_shade(v + i);
				}
				if(st->opt & G3D_TEXTURE_GEN) {
					v[i].u = v[i].nx * 0.5 + 0.5;
					v[i].v = 0.5 - v[i].ny * 0.5;
				}
			}
			if(st->opt & G3D_TEXTURE_MAT) {
				float *mat = st->mat[G3D_TEXTURE][st->mtop[G3D_TEXTURE]];
				float x = mat[0] * v[i].u + mat[4] * v[i].v + mat[12];
				float y = mat[1] * v[i].u + mat[5] * v[i].v + mat[13];
				float w = mat[3] * v[i].u + mat[7] * v[i].v + mat[15];
				if(w != 0) {
					float rcp_w = 1.0 / w;
					v[i].u = x * rcp_w;
					v[i].v = y * rcp_w;
				}
			}
			xform4_vec3(st->mat[G3D_PROJECTION][ptop], &v[i].x);
		}

		/* clipping */
		if(st->opt & G3D_CLIP_FRUSTUM) {
			for(i=0; i<6; i++) {
				memcpy(tmpv, v, vnum * sizeof *v);

				if(clip_frustum(v, &vnum, tmpv, vnum, i) < 0) {
					/* polygon completely outside of view volume. discard */
					vnum = 0;
					break;
				}
			}

			if(!vnum) continue;
		}

		for(i=0; i<vnum; i++) {
			if(v[i].w != 0.0f) {
				float rcp_w = 1.0f / v[i].w;
				v[i].x *= rcp_w;
				v[i].y *= rcp_w;
#ifdef ENABLE_ZBUFFER
				if(st->opt & G3D_DEPTH_TEST) {
					v[i].z *= rcp_w;
				}
#endif
			}

			/* viewport transformation */
			v[i].x = (v[i].x * 0.5f + 0.5f) * (float)st->vport[2] + st->vport[0];
			v[i].y = (0.5f - v[i].y * 0.5f) * (float)st->vport[3] + st->vport[1];

			/* convert pos to 24.8 fixed point */
			pv[i].x = cround64(v[i].x * 256.0f);
			pv[i].y = cround64(v[i].y * 256.0f);
#ifdef ENABLE_ZBUFFER
			if(st->opt & G3D_DEPTH_TEST) {
				/* after div/w z is in [-1, 1], remap it to [0, 0xffffff] */
				pv[i].z = cround64(v[i].z * 8388607.5f + 8388607.5f);
			}
#endif
			/* convert tex coords to 16.16 fixed point */
			pv[i].u = cround64(v[i].u * 65536.0f);
			pv[i].v = cround64(v[i].v * 65536.0f);
			/* pass the color through as is */
			pv[i].r = v[i].r;
			pv[i].g = v[i].g;
			pv[i].b = v[i].b;
			pv[i].a = v[i].a;
		}

		switch(vnum) {
		case 1:
			if(st->opt & (G3D_ALPHA_BLEND | G3D_ADD_BLEND)) {
				int r, g, b, inv_alpha;
				g3d_pixel *dest = pfill_fb.pixels + (pv[0].y >> 8) * pfill_fb.scanlen + (pv[0].x >> 8);
				if(st->opt & G3D_ALPHA_BLEND) {
					inv_alpha = 255 - pv[0].a;
					r = ((int)pv[0].r * pv[0].a + G3D_UNPACK_R(*dest) * inv_alpha) >> 8;
					g = ((int)pv[0].g * pv[0].a + G3D_UNPACK_G(*dest) * inv_alpha) >> 8;
					b = ((int)pv[0].b * pv[0].a + G3D_UNPACK_B(*dest) * inv_alpha) >> 8;
				} else {
					r = (int)pv[0].r + G3D_UNPACK_R(*dest);
					g = (int)pv[0].g + G3D_UNPACK_G(*dest);
					b = (int)pv[0].b + G3D_UNPACK_B(*dest);
					if(r > 255) r = 255;
					if(g > 255) g = 255;
					if(b > 255) b = 255;
				}
				*dest++ = G3D_PACK_RGB(r, g, b);
			} else {
				g3d_pixel *dest = pfill_fb.pixels + (pv[0].y >> 8) * pfill_fb.scanlen + (pv[0].x >> 8);
				*dest = G3D_PACK_RGB(pv[0].r, pv[0].g, pv[0].b);
			}
			break;

		case 2:
			{
				g3d_pixel col = G3D_PACK_RGB(pv[0].r, pv[0].g, pv[0].b);
				draw_line(pv[0].x >> 8, pv[0].y >> 8, pv[1].x >> 8, pv[1].y >> 8, col);
			}
			break;

		default:
			fill_mode = st->polymode;
			if(st->opt & G3D_TEXTURE_2D) {
				fill_mode |= st->texmode << POLYFILL_TEXMODE_SHIFT;
			}
			if(st->opt & G3D_ALPHA_BLEND) {
				fill_mode |= POLYFILL_ALPHA_BIT;
			} else if(st->opt & G3D_ADD_BLEND) {
				fill_mode |= POLYFILL_ADD_BIT;
			}
#ifdef ENABLE_ZBUFFER
			if(st->opt & G3D_DEPTH_TEST) {
				fill_mode |= POLYFILL_ZBUF_BIT;
			}
#endif
			num_tri = vnum - 2;
			vtri = v;
			pvtri = pv;
			for(;;) {
				/* backface culling */
				if(st->opt & G3D_CULL_FACE) {
					int32_t ax = pvtri[1].x - pvtri[0].x;
					int32_t ay = pvtri[1].y - pvtri[0].y;
					int32_t bx = pvtri[2].x - pvtri[0].x;
					int32_t by = pvtri[2].y - pvtri[0].y;
					int32_t cross_z = (ax >> 4) * (by >> 4) - (ay >> 4) * (bx >> 4);
					int sign = (cross_z >> 31) & 1;

					if(!(sign ^ st->frontface)) {
						goto skip_triangle;	/* back-facing */
					}

					if(st->frontface != G3D_CCW) {
						struct pvertex revtri[3];
						revtri[0] = pvtri[0];
						revtri[1] = pvtri[2];
						revtri[2] = pvtri[1];

						calc_grad(vtri);
						polyfill(fill_mode, revtri);
						goto skip_triangle;
					}
				}

				calc_grad(vtri);
				polyfill(fill_mode, pvtri);
skip_triangle:
				if(--num_tri == 0) break;
				vtri[1] = vtri[0];
				pvtri[1] = pvtri[0];
				vtri++;
				pvtri++;
			}
		}
	}
}

#define ATTR_DELTAS(attr) \
	float d##attr##02 = v[0].attr - v[2].attr; \
	float d##attr##12 = v[1].attr - v[2].attr

#define DFDX(attr)	\
	(dx ? (d##attr##12 * dy02 - d##attr##02 * dy12) / dx : 0)
#define DFDY(attr)	\
	(dy ? (d##attr##12 * dx02 - d##attr##02 * dx12) / dy : 0)

static void calc_grad(struct g3d_vertex *v)
{
	float dx02 = v[0].x - v[2].x;
	float dx12 = v[1].x - v[2].x;
	float dy02 = v[0].y - v[2].y;
	float dy12 = v[1].y - v[2].y;

	float dx = dx12 * dy02 - dx02 * dy12;
	float dy = dx02 * dy12 - dx12 * dy02;

	if(st->polymode == POLYFILL_GOURAUD) {
		ATTR_DELTAS(r);
		ATTR_DELTAS(g);
		ATTR_DELTAS(b);
		pgrad.drdx = cround64(DFDX(r) * 4096.0f);
		pgrad.drdy = cround64(DFDY(r) * 4096.0f);
		pgrad.dgdx = cround64(DFDX(g) * 4096.0f);
		pgrad.dgdy = cround64(DFDY(g) * 4096.0f);
		pgrad.dbdx = cround64(DFDX(b) * 4096.0f);
		pgrad.dbdy = cround64(DFDY(b) * 4096.0f);
		if(st->opt & G3D_ALPHA_BLEND) {
			ATTR_DELTAS(a);
			pgrad.dadx = cround64(DFDX(a) * 4096.0f);
			pgrad.dady = cround64(DFDY(a) * 4096.0f);
		}
	}

	if(st->opt & G3D_DEPTH_TEST) {
		ATTR_DELTAS(z);
		pgrad.dzdx = cround64(DFDX(z) * 8388607.5f);
		pgrad.dzdy = cround64(DFDY(z) * 8388607.5f);
	}

	if(st->opt & G3D_TEXTURE_2D) {
		ATTR_DELTAS(u);
		ATTR_DELTAS(v);
		pgrad.dudx = cround64(DFDX(u) * 65536.0f);
		pgrad.dudy = cround64(DFDY(u) * 65536.0f);
		pgrad.dvdx = cround64(DFDX(v) * 65536.0f);
		pgrad.dvdy = cround64(DFDY(v) * 65536.0f);
	}
}

void g3d_begin(int prim)
{
	st->imm_prim = prim;
	st->imm_pcount = prim;
	st->imm_numv = 0;
}

void g3d_end(void)
{
	imm_flush();
}

static void imm_flush(void)
{
	int numv = st->imm_numv;
	st->imm_numv = 0;
	g3d_draw_indexed(st->imm_prim, st->imm_vbuf, numv, 0, 0);
}

void g3d_vertex(float x, float y, float z)
{
	struct g3d_vertex *vptr = st->imm_vbuf + st->imm_numv++;
	*vptr = st->imm_curv;
	vptr->x = x;
	vptr->y = y;
	vptr->z = z;
	vptr->w = 1.0f;

	if(!--st->imm_pcount) {
		if(st->imm_numv >= IMM_VBUF_SIZE - st->imm_prim) {
			imm_flush();
		}
		st->imm_pcount = st->imm_prim;
	}
}

void g3d_normal(float x, float y, float z)
{
	st->imm_curv.nx = x;
	st->imm_curv.ny = y;
	st->imm_curv.nz = z;
}

#define CLAMP(x, a, b)	((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#define MIN(a, b)		((a) < (b) ? (a) : (b))

void g3d_color3b(unsigned char r, unsigned char g, unsigned char b)
{
	st->imm_curv.r = MIN(r, 255);
	st->imm_curv.g = MIN(g, 255);
	st->imm_curv.b = MIN(b, 255);
	st->imm_curv.a = 255;
}

void g3d_color4b(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	st->imm_curv.r = MIN(r, 255);
	st->imm_curv.g = MIN(g, 255);
	st->imm_curv.b = MIN(b, 255);
	st->imm_curv.a = MIN(a, 255);
}

void g3d_color3f(float r, float g, float b)
{
	int ir = r * 255.0f;
	int ig = g * 255.0f;
	int ib = b * 255.0f;
	st->imm_curv.r = CLAMP(ir, 0, 255);
	st->imm_curv.g = CLAMP(ig, 0, 255);
	st->imm_curv.b = CLAMP(ib, 0, 255);
	st->imm_curv.a = 255;
}

void g3d_color4f(float r, float g, float b, float a)
{
	int ir = r * 255.0f;
	int ig = g * 255.0f;
	int ib = b * 255.0f;
	int ia = a * 255.0f;
	st->imm_curv.r = CLAMP(ir, 0, 255);
	st->imm_curv.g = CLAMP(ig, 0, 255);
	st->imm_curv.b = CLAMP(ib, 0, 255);
	st->imm_curv.a = CLAMP(ia, 0, 255);
}

void g3d_texcoord(float u, float v)
{
	st->imm_curv.u = u;
	st->imm_curv.v = v;
}

void g3d_clip_plane(int idx, const float *eqn)
{
	int mvtop = st->mtop[G3D_MODELVIEW];
	float mat[16];
	struct cplane *cp;
	cgm_vec4 veqn;

	if(idx < 0 || idx >= 4) return;

	memcpy(mat, st->mat[G3D_MODELVIEW][mvtop], sizeof mat);
	cgm_minverse(mat);
	memcpy(&veqn, eqn, sizeof veqn);
	cgm_wmul_v4m4(&veqn, mat);

	cp = &st->clip[idx];
	cp->nx = veqn.x;
	cp->ny = veqn.y;
	cp->nz = veqn.z;
	cp->x = -veqn.x * veqn.w;
	cp->y = -veqn.y * veqn.w;
	cp->z = -veqn.z * veqn.w;
}

int g3d_xform_point(float *vec)
{
	int mvtop = st->mtop[G3D_MODELVIEW];
	int ptop = st->mtop[G3D_PROJECTION];
	int inside;
	float rcp_w;

	xform4_vec3(st->mat[G3D_MODELVIEW][mvtop], vec);
	xform4_vec3(st->mat[G3D_PROJECTION][ptop], vec);

	inside = vec[0] > -vec[3] && vec[0] < vec[3] && vec[1] > -vec[3] && vec[1] < vec[3];

	rcp_w = vec[3] == 0.0f ? 1.0f : 1.0f / vec[3];

	vec[0] *= rcp_w;
	vec[1] *= rcp_w;
	vec[2] *= rcp_w;

	vec[0] = (vec[0] * 0.5f + 0.5f) * (float)st->vport[2] + st->vport[0];
	vec[1] = (0.5f - vec[1] * 0.5f) * (float)st->vport[3] + st->vport[1];
	return inside;
}

void g3d_viewspace_vertex(struct g3d_vertex *v)
{
	int mvtop = st->mtop[G3D_MODELVIEW];

	xform4_vec3(st->mat[G3D_MODELVIEW][mvtop], &v->x);
	xform3_vec3(st->mat[G3D_MODELVIEW][mvtop], &v->nx);
}

void g3d_shade(struct g3d_vertex *v)
{
	int i, r, g, b;
	float color[3], kd[3];

	if(st->opt & G3D_COLOR_MATERIAL) {
		kd[0] = (float)v->r / 255.0f;
		kd[1] = (float)v->g / 255.0f;
		kd[2] = (float)v->b / 255.0f;
	} else {
		kd[0] = st->mtl.kd[0];
		kd[1] = st->mtl.kd[1];
		kd[2] = st->mtl.kd[2];
	}

	color[0] = st->ambient[0] * kd[0];
	color[1] = st->ambient[1] * kd[1];
	color[2] = st->ambient[2] * kd[2];

	for(i=0; i<MAX_LIGHTS; i++) {
		float ldir[3];
		float ndotl;

		if(!(st->opt & (G3D_LIGHT0 << i))) {
			continue;
		}

		ldir[0] = st->lt[i].x;
		ldir[1] = st->lt[i].y;
		ldir[2] = st->lt[i].z;

		if(st->lt[i].type != LT_DIR) {
			ldir[0] -= v->x;
			ldir[1] -= v->y;
			ldir[2] -= v->z;
			NORMALIZE(ldir);
		}

		if((ndotl = v->nx * ldir[0] + v->ny * ldir[1] + v->nz * ldir[2]) < 0.0f) {
			ndotl = 0.0f;
		}

		color[0] += kd[0] * st->lt[i].r * ndotl;
		color[1] += kd[1] * st->lt[i].g * ndotl;
		color[2] += kd[2] * st->lt[i].b * ndotl;

		if(st->opt & G3D_SPECULAR) {
			float ndoth;
			ldir[2] += 1.0f;
			NORMALIZE(ldir);
			if((ndoth = v->nx * ldir[0] + v->ny * ldir[1] + v->nz * ldir[2]) < 0.0f) {
				ndoth = 0.0f;
			}
			ndoth = pow(ndoth, st->mtl.shin);

			color[0] += st->mtl.ks[0] * st->lt[i].r * ndoth;
			color[1] += st->mtl.ks[1] * st->lt[i].g * ndoth;
			color[2] += st->mtl.ks[2] * st->lt[i].b * ndoth;
		}
	}

	r = cround64(color[0] * 255.0);
	g = cround64(color[1] * 255.0);
	b = cround64(color[2] * 255.0);

	v->r = r > 255 ? 255 : r;
	v->g = g > 255 ? 255 : g;
	v->b = b > 255 ? 255 : b;
}

static __inline void xform4_vec3(const float *mat, float *vec)
{
	float x = mat[0] * vec[0] + mat[4] * vec[1] + mat[8] * vec[2] + mat[12];
	float y = mat[1] * vec[0] + mat[5] * vec[1] + mat[9] * vec[2] + mat[13];
	float z = mat[2] * vec[0] + mat[6] * vec[1] + mat[10] * vec[2] + mat[14];
	vec[3] = mat[3] * vec[0] + mat[7] * vec[1] + mat[11] * vec[2] + mat[15];
	vec[2] = z;
	vec[1] = y;
	vec[0] = x;
}

static __inline void xform3_vec3(const float *mat, float *vec)
{
	float x = mat[0] * vec[0] + mat[4] * vec[1] + mat[8] * vec[2];
	float y = mat[1] * vec[0] + mat[5] * vec[1] + mat[9] * vec[2];
	vec[2] = mat[2] * vec[0] + mat[6] * vec[1] + mat[10] * vec[2];
	vec[1] = y;
	vec[0] = x;
}

static __inline void xform4_vec4(const float *mat, float *vec)
{
	float x = mat[0] * vec[0] + mat[4] * vec[1] + mat[8] * vec[2] + mat[12] * vec[3];
	float y = mat[1] * vec[0] + mat[5] * vec[1] + mat[9] * vec[2] + mat[13] * vec[3];
	float z = mat[2] * vec[0] + mat[6] * vec[1] + mat[10] * vec[2] + mat[14] * vec[3];
	vec[3] = mat[3] * vec[0] + mat[7] * vec[1] + mat[11] * vec[2] + mat[15] * vec[3];
	vec[2] = z;
	vec[1] = y;
	vec[0] = x;
}

#endif	/* !def BUILD_OPENGL */
