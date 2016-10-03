#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "screen.h"
#include "demo.h"
#include "3dgfx.h"
#include "gfxutil.h"

struct mesh {
	int prim;
	struct g3d_vertex *varr;
	int16_t *iarr;
	int vcount, icount;
};

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void draw_mesh(struct mesh *mesh);
static int gen_cube(struct mesh *mesh, float sz);
static int gen_torus(struct mesh *mesh, float rad, float ringrad, int usub, int vsub);
static void zsort(struct mesh *m);
static void draw_lowres_raster(void);

static struct screen scr = {
	"polytest",
	init,
	destroy,
	start, 0,
	draw
};

static float theta, phi = 25;
static struct mesh cube, torus;

#define LOWRES_SCALE	10
static uint16_t *lowres_pixels;
static int lowres_width, lowres_height;

struct screen *polytest_screen(void)
{
	return &scr;
}

static int init(void)
{
	gen_cube(&cube, 1.0);
	gen_torus(&torus, 1.0, 0.25, 24, 12);

#ifdef DEBUG_POLYFILL
	lowres_width = fb_width / LOWRES_SCALE;
	lowres_height = fb_height / LOWRES_SCALE;
	lowres_pixels = malloc(lowres_width * lowres_height * 2);
	scr.draw = draw_debug;
#endif

	return 0;
}

static void destroy(void)
{
	free(lowres_pixels);
	free(cube.varr);
	free(torus.varr);
	free(torus.iarr);
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.3333333, 0.5, 100.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_enable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);
}

static void update(void)
{
	static int prev_mx, prev_my;
	static unsigned int prev_bmask;

	if(mouse_bmask) {
		if((mouse_bmask ^ prev_bmask) == 0) {
			int dx = mouse_x - prev_mx;
			int dy = mouse_y - prev_my;

			if(dx || dy) {
				theta += dx * 2.0;
				phi += dy * 2.0;

				if(phi < -90) phi = -90;
				if(phi > 90) phi = 90;
			}
		}
	}
	prev_mx = mouse_x;
	prev_my = mouse_y;
	prev_bmask = mouse_bmask;
}


static void draw(void)
{
	update();

	memset(fb_pixels, 0, fb_width * fb_height * 2);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -3);
	g3d_rotate(phi, 1, 0, 0);
	g3d_rotate(theta, 0, 1, 0);

	g3d_light_pos(0, -10, 10, 20);

	zsort(&torus);

	g3d_mtl_diffuse(0.3, 0.6, 1.0);
	draw_mesh(&torus);

	/*draw_mesh(&cube);*/
	swap_buffers(fb_pixels);
}

static void draw_debug(void)
{
	update();

	memset(lowres_pixels, 0, lowres_width * lowres_height * 2);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -3);
	g3d_rotate(phi, 1, 0, 0);
	g3d_rotate(theta, 0, 1, 0);

	g3d_framebuffer(lowres_width, lowres_height, lowres_pixels);
	/*zsort(&torus);*/
	draw_mesh(&cube);

	draw_lowres_raster();


	g3d_framebuffer(fb_width, fb_height, fb_pixels);

	g3d_polygon_mode(G3D_WIRE);
	draw_mesh(&cube);
	g3d_polygon_mode(G3D_FLAT);

	swap_buffers(fb_pixels);
}

static void draw_mesh(struct mesh *mesh)
{
	if(mesh->iarr) {
		g3d_draw_indexed(mesh->prim, mesh->varr, mesh->vcount, mesh->iarr, mesh->icount);
	} else {
		g3d_draw(mesh->prim, mesh->varr, mesh->vcount);
	}
}

#define NORMAL(vp, x, y, z) do { vp->nx = x; vp->ny = y; vp->nz = z; } while(0)
#define COLOR(vp, cr, cg, cb) do { vp->r = cr; vp->g = cg; vp->b = cb; } while(0)
#define TEXCOORD(vp, tu, tv) do { vp->u = tu; vp->v = tv; } while(0)
#define VERTEX(vp, vx, vy, vz) \
	do { \
		vp->x = vx; vp->y = vy; vp->z = vz; vp->w = 1.0f; \
		++vp; \
	} while(0)

static int gen_cube(struct mesh *mesh, float sz)
{
	struct g3d_vertex *vptr;
	float hsz = sz / 2.0;

	mesh->prim = G3D_QUADS;
	mesh->iarr = 0;
	mesh->icount = 0;

	mesh->vcount = 24;
	if(!(mesh->varr = malloc(mesh->vcount * sizeof *mesh->varr))) {
		return -1;
	}
	vptr = mesh->varr;

	/* -Z */
	NORMAL(vptr, 0, 0, -1);
	COLOR(vptr, 255, 0, 255);
	VERTEX(vptr, hsz, -hsz, -hsz);
	VERTEX(vptr, -hsz, -hsz, -hsz);
	VERTEX(vptr, -hsz, hsz, -hsz);
	VERTEX(vptr, hsz, hsz, -hsz);
	/* -Y */
	NORMAL(vptr, 0, -1, 0);
	COLOR(vptr, 0, 255, 255);
	VERTEX(vptr, -hsz, -hsz, -hsz);
	VERTEX(vptr, hsz, -hsz, -hsz);
	VERTEX(vptr, hsz, -hsz, hsz);
	VERTEX(vptr, -hsz, -hsz, hsz);
	/* -X */
	NORMAL(vptr, -1, 0, 0);
	COLOR(vptr, 255, 255, 0);
	VERTEX(vptr, -hsz, -hsz, -hsz);
	VERTEX(vptr, -hsz, -hsz, hsz);
	VERTEX(vptr, -hsz, hsz, hsz);
	VERTEX(vptr, -hsz, hsz, -hsz);
	/* +X */
	NORMAL(vptr, 1, 0, 0);
	COLOR(vptr, 255, 0, 0);
	VERTEX(vptr, hsz, -hsz, hsz);
	VERTEX(vptr, hsz, -hsz, -hsz);
	VERTEX(vptr, hsz, hsz, -hsz);
	VERTEX(vptr, hsz, hsz, hsz);
	/* +Y */
	NORMAL(vptr, 0, 1, 0);
	COLOR(vptr, 0, 255, 0);
	VERTEX(vptr, -hsz, hsz, hsz);
	VERTEX(vptr, hsz, hsz, hsz);
	VERTEX(vptr, hsz, hsz, -hsz);
	VERTEX(vptr, -hsz, hsz, -hsz);
	/* +Z */
	NORMAL(vptr, 0, 0, 1);
	COLOR(vptr, 0, 0, 255);
	VERTEX(vptr, -hsz, -hsz, hsz);
	VERTEX(vptr, hsz, -hsz, hsz);
	VERTEX(vptr, hsz, hsz, hsz);
	VERTEX(vptr, -hsz, hsz, hsz);

	return 0;
}

static void torusvec(float *res, float theta, float phi, float mr, float rr)
{
	float rx, ry, rz;
	theta = -theta;

	rx = -cos(phi) * rr + mr;
	ry = sin(phi) * rr;
	rz = 0.0f;

	res[0] = rx * sin(theta) + rz * cos(theta);
	res[1] = ry;
	res[2] = -rx * cos(theta) + rz * sin(theta);
}

static int gen_torus(struct mesh *mesh, float rad, float ringrad, int usub, int vsub)
{
	int i, j;
	int nfaces, uverts, vverts;
	struct g3d_vertex *vptr;
	int16_t *iptr;

	mesh->prim = G3D_QUADS;

	if(usub < 4) usub = 4;
	if(vsub < 2) vsub = 2;

	uverts = usub + 1;
	vverts = vsub + 1;

	mesh->vcount = uverts * vverts;
	nfaces = usub * vsub;
	mesh->icount = nfaces * 4;

	if(!(mesh->varr = malloc(mesh->vcount * sizeof *mesh->varr))) {
		return -1;
	}
	if(!(mesh->iarr = malloc(mesh->icount * sizeof *mesh->iarr))) {
		return -1;
	}
	vptr = mesh->varr;
	iptr = mesh->iarr;

	for(i=0; i<uverts; i++) {
		float u = (float)i / (float)(uverts - 1);
		float theta = u * 2.0 * M_PI;
		float rcent[3];

		torusvec(rcent, theta, 0, rad, 0);

		for(j=0; j<vverts; j++) {
			float v = (float)j / (float)(vverts - 1);
			float phi = v * 2.0 * M_PI;
			int chess = (i & 1) == (j & 1);

			torusvec(&vptr->x, theta, phi, rad, ringrad);

			vptr->nx = (vptr->x - rcent[0]) / ringrad;
			vptr->ny = (vptr->y - rcent[1]) / ringrad;
			vptr->nz = (vptr->z - rcent[2]) / ringrad;
			vptr->u = u;
			vptr->v = v;
			vptr->r = chess ? 255 : 64;
			vptr->g = 128;
			vptr->b = chess ? 64 : 255;
			++vptr;

			if(i < usub && j < vsub) {
				int idx = i * vverts + j;
				*iptr++ = idx;
				*iptr++ = idx + 1;
				*iptr++ = idx + vverts + 1;
				*iptr++ = idx + vverts;
			}
		}
	}
	return 0;
}


static struct {
	struct g3d_vertex *varr;
	const float *xform;
} zsort_cls;

static int zsort_cmp(const void *aptr, const void *bptr)
{
	const int16_t *a = (const int16_t*)aptr;
	const int16_t *b = (const int16_t*)bptr;

	const float *m = zsort_cls.xform;

	const struct g3d_vertex *va = zsort_cls.varr + a[0];
	const struct g3d_vertex *vb = zsort_cls.varr + b[0];

	float za = m[2] * va->x + m[6] * va->y + m[10] * va->z + m[14];
	float zb = m[2] * vb->x + m[6] * vb->y + m[10] * vb->z + m[14];

	va = zsort_cls.varr + a[2];
	vb = zsort_cls.varr + b[2];

	za += m[2] * va->x + m[6] * va->y + m[10] * va->z + m[14];
	zb += m[2] * vb->x + m[6] * vb->y + m[10] * vb->z + m[14];

	return za - zb;
}

static void zsort(struct mesh *m)
{
	int nfaces = m->icount / m->prim;

	zsort_cls.varr = m->varr;
	zsort_cls.xform = g3d_get_matrix(G3D_MODELVIEW, 0);

	qsort(m->iarr, nfaces, m->prim * sizeof *m->iarr, zsort_cmp);
}

static void draw_huge_pixel(uint16_t *dest, int dest_width, uint16_t color)
{
	int i, j;
	uint16_t grid_color = PACK_RGB16(127, 127, 127);

	for(i=0; i<LOWRES_SCALE; i++) {
		for(j=0; j<LOWRES_SCALE; j++) {
			dest[j] = i == 0 || j == 0 ? grid_color : color;
		}
		dest += dest_width;
	}
}

static void draw_lowres_raster(void)
{
	int i, j;
	uint16_t *sptr = lowres_pixels;
	uint16_t *dptr = fb_pixels;

	for(i=0; i<lowres_height; i++) {
		for(j=0; j<lowres_width; j++) {
			draw_huge_pixel(dptr, fb_width, *sptr++);
			dptr += LOWRES_SCALE;
		}
		dptr += fb_width * LOWRES_SCALE - fb_width;
	}
}
