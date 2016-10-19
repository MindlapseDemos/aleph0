#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "screen.h"
#include "demo.h"
#include "3dgfx.h"
#include "gfxutil.h"
#include "util.h"
#include "metasurf.h"
#include "dynarr.h"

struct mesh {
	int prim;
	struct g3d_vertex *varr;
	int16_t *iarr;
	int vcount, icount;
};

struct metaball {
	float energy;
	float pos[3];
};

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void draw_mesh(struct mesh *mesh);
static void zsort(struct mesh *m);

static void calc_voxel_field(void);
static float eval(struct metasurface *ms, float x, float y, float z);
static void emit_vertex(struct metasurface *ms, float x, float y, float z);

static struct screen scr = {
	"metaballs",
	init,
	destroy,
	start, 0,
	draw
};

static float cam_theta, cam_phi = 25;
static float cam_dist = 3;
static struct mesh mmesh;

static struct metasurface *msurf;

#define VOL_SZ	32
#define VOL_SCALE	10.0f
#define VOL_HALF_SCALE	(VOL_SCALE * 0.5f)
static float *volume;
#define VOXEL(x, y, z)	(volume[(z) * VOL_SZ * VOL_SZ + (y) * VOL_SZ + (x)])

#define NUM_MBALLS	3
static struct metaball mball[NUM_MBALLS];


struct screen *metaballs_screen(void)
{
	return &scr;
}

static int init(void)
{
	if(!(volume = malloc(VOL_SZ * VOL_SZ * VOL_SZ * sizeof *volume))) {
		fprintf(stderr, "failed to allocate %dx%dx%d voxel field\n", VOL_SZ, VOL_SZ, VOL_SZ);
		return -1;
	}

	mball[0].energy = 1.2;
	mball[1].energy = 0.8;
	mball[2].energy = 1.0;

	if(!(msurf = msurf_create())) {
		fprintf(stderr, "failed to initialize metasurf\n");
		return -1;
	}
	msurf_set_resolution(msurf, VOL_SZ, VOL_SZ, VOL_SZ);
	msurf_set_bounds(msurf, 0, 0, 0, VOL_SCALE, VOL_SCALE, VOL_SCALE);
	msurf_eval_func(msurf, eval);
	msurf_set_threshold(msurf, 0.5);
	msurf_set_inside(msurf, MSURF_GREATER);
	msurf_vertex_func(msurf, emit_vertex);

	mmesh.prim = G3D_TRIANGLES;
	mmesh.varr = 0;
	mmesh.iarr = 0;
	mmesh.vcount = mmesh.icount = 0;

	return 0;
}

static void destroy(void)
{
	dynarr_free(mmesh.varr);
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.3333333, 0.5, 100.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_enable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);

	g3d_polygon_mode(G3D_WIRE);
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
				if(mouse_bmask & 1) {
					cam_theta += dx * 1.0;
					cam_phi += dy * 1.0;

					if(cam_phi < -90) cam_phi = -90;
					if(cam_phi > 90) cam_phi = 90;
				}
				if(mouse_bmask & 4) {
					cam_dist += dy * 0.5;

					if(cam_dist < 0) cam_dist = 0;
				}
			}
		}
	}
	prev_mx = mouse_x;
	prev_my = mouse_y;
	prev_bmask = mouse_bmask;

	{
		int i, j;
		float tsec = time_msec / 1000.0f;
		static float phase[] = {0.0, M_PI / 3.0, M_PI * 0.8};
		static float speed[] = {0.8, 1.4, 1.0};
		static float scale[][3] = {{1, 2, 0.8}, {0.5, 1.6, 0.6}, {1.5, 0.7, 0.5}};
		static float offset[][3] = {{0, 0, 0}, {0.25, 0, 0}, {-0.2, 0.15, 0.2}};

		for(i=0; i<NUM_MBALLS; i++) {
			float t = (tsec + phase[i]) * speed[i];

			for(j=0; j<3; j++) {
				float x = sin(t + j * M_PI / 2.0);
				mball[i].pos[j] = offset[i][j] + x * scale[i][j];
			}
		}
	}

	calc_voxel_field();

	dynarr_free(mmesh.varr);
	mmesh.varr = dynarr_alloc(0, sizeof *mmesh.varr);
	msurf_polygonize(msurf);
}

static void draw(void)
{
	update();

	memset(fb_pixels, 0, fb_width * fb_height * 2);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);

	/*g3d_light_pos(0, -10, 10, 20);*/

	/*zsort(&metamesh);*/

	/*g3d_mtl_diffuse(0.6, 0.6, 0.6);*/

	draw_mesh(&mmesh);

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

static void calc_voxel_field(void)
{
	int i, j, k, b;
	float *voxptr = volume;

	for(i=0; i<VOL_SZ; i++) {
		float z = VOL_SCALE * ((float)i / (float)VOL_SZ - 0.5);

		for(j=0; j<VOL_SZ; j++) {
			float y = VOL_SCALE * ((float)j / (float)VOL_SZ - 0.5);

			for(k=0; k<VOL_SZ; k++) {
				float x = VOL_SCALE * ((float)k / (float)VOL_SZ - 0.5);

				float val = 0.0f;
				for(b=0; b<NUM_MBALLS; b++) {
					float dx = mball[b].pos[0] - x;
					float dy = mball[b].pos[1] - y;
					float dz = mball[b].pos[2] - z;

					float lensq = dx * dx + dy * dy + dz * dz;

					val += lensq == 0.0f ? 1024.0f : mball[b].energy / lensq;
				}

				*voxptr++ = val;
			}
		}
	}
}

static float eval(struct metasurface *ms, float x, float y, float z)
{
	int xidx = cround64(x);
	int yidx = cround64(y);
	int zidx = cround64(z);

	assert(xidx >= 0 && xidx < VOL_SZ);
	assert(yidx >= 0 && yidx < VOL_SZ);
	assert(zidx >= 0 && zidx < VOL_SZ);

	return VOXEL(xidx, yidx, zidx);
}

static void emit_vertex(struct metasurface *ms, float x, float y, float z)
{
	struct g3d_vertex v;

	v.x = x - VOL_HALF_SCALE;
	v.y = y - VOL_HALF_SCALE;
	v.z = z - VOL_HALF_SCALE;
	v.r = cround64(255.0 * x / VOL_SCALE);
	v.g = cround64(255.0 * y / VOL_SCALE);
	v.b = cround64(255.0 * z / VOL_SCALE);

	mmesh.varr = dynarr_push(mmesh.varr, &v);
	assert(mmesh.varr);
	++mmesh.vcount;
}
