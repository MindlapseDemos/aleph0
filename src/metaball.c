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

static struct screen scr = {
	"metaballs",
	init,
	destroy,
	start, 0,
	draw
};

static float cam_theta, cam_phi = 25;
static float cam_dist = 10;
static struct mesh mmesh;

static struct metasurface *msurf;

#define VOL_SZ	32
#define VOL_SCALE	10.0f
#define VOX_DIST	(VOL_SCALE / VOL_SZ)
#define VOL_HALF_SCALE	(VOL_SCALE * 0.5f)

#define NUM_MBALLS	3
static struct metaball mball[NUM_MBALLS];

static int dbg;

struct screen *metaballs_screen(void)
{
	return &scr;
}

static int init(void)
{
	mball[0].energy = 1.2;
	mball[1].energy = 0.8;
	mball[2].energy = 1.0;

	if(!(msurf = msurf_create())) {
		fprintf(stderr, "failed to initialize metasurf\n");
		return -1;
	}
	msurf_set_resolution(msurf, VOL_SZ, VOL_SZ, VOL_SZ);
	msurf_set_bounds(msurf, -VOL_HALF_SCALE, -VOL_HALF_SCALE, -VOL_HALF_SCALE,
			VOL_HALF_SCALE, VOL_HALF_SCALE, VOL_HALF_SCALE);
	msurf_set_threshold(msurf, 1.7);
	msurf_set_inside(msurf, MSURF_GREATER);

	mmesh.prim = G3D_TRIANGLES;
	mmesh.varr = 0;
	mmesh.iarr = 0;
	mmesh.vcount = mmesh.icount = 0;

	return 0;
}

static void destroy(void)
{
	msurf_free(msurf);
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.3333333, 0.5, 100.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_enable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);

	g3d_polygon_mode(G3D_GOURAUD);
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
	msurf_polygonize(msurf);

	mmesh.vcount = msurf_vertex_count(msurf);
	mmesh.varr = msurf_vertices(msurf);
}

static void draw(void)
{
	int i, j;

	update();

	memset(fb_pixels, 0, fb_width * fb_height * 2);

	for(i=0; i<120; i++) {
		for(j=0; j<160; j++) {
			fb_pixels[(i + 60) * 320 + (j + 80)] = 0x1e7;
		}
	}
	g3d_viewport(80, 60, 160, 120);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);

	g3d_light_pos(0, -10, 10, 20);

	zsort(&mmesh);

	g3d_mtl_diffuse(0.6, 0.6, 0.6);

	draw_mesh(&mmesh);

	g3d_viewport(0, 0, fb_width, fb_height);

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
	const float *m = zsort_cls.xform;

	const struct g3d_vertex *va = (const struct g3d_vertex*)aptr;
	const struct g3d_vertex *vb = (const struct g3d_vertex*)bptr;

	float za = m[2] * va->x + m[6] * va->y + m[10] * va->z + m[14];
	float zb = m[2] * vb->x + m[6] * vb->y + m[10] * vb->z + m[14];

	++va;
	++vb;

	za += m[2] * va->x + m[6] * va->y + m[10] * va->z + m[14];
	zb += m[2] * vb->x + m[6] * vb->y + m[10] * vb->z + m[14];

	return za - zb;
}

static void zsort(struct mesh *m)
{
	int nfaces = m->vcount / m->prim;

	zsort_cls.varr = m->varr;
	zsort_cls.xform = g3d_get_matrix(G3D_MODELVIEW, 0);

	qsort(m->varr, nfaces, m->prim * sizeof *m->varr, zsort_cmp);
}

static void calc_voxel_field(void)
{
	int i, j, k, b;
	float *voxptr;

	if(!(voxptr = msurf_voxels(msurf))) {
		fprintf(stderr, "failed to allocate voxel field\n");
		abort();
	}

	for(i=0; i<VOL_SZ; i++) {
		float z = -VOL_HALF_SCALE + i * VOX_DIST;

		for(j=0; j<VOL_SZ; j++) {
			float y = -VOL_HALF_SCALE + j * VOX_DIST;

			for(k=0; k<VOL_SZ; k++) {
				float x = -VOL_HALF_SCALE + k * VOX_DIST;

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
	++dbg;
}
