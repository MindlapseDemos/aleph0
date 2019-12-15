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
#include "mesh.h"

struct metaball {
	float energy;
	float pos[3];
};

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

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
static struct g3d_mesh mmesh;

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
	int i, j;
	float tsec = time_msec / 1000.0f;
	static float phase[] = {0.0, M_PI / 3.0, M_PI * 0.8};
	static float speed[] = {0.8, 1.4, 1.0};
	static float scale[][3] = {{1, 2, 0.8}, {0.5, 1.6, 0.6}, {1.5, 0.7, 0.5}};
	static float offset[][3] = {{0, 0, 0}, {0.25, 0, 0}, {-0.2, 0.15, 0.2}};

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);

	for(i=0; i<NUM_MBALLS; i++) {
		float t = (tsec + phase[i]) * speed[i];

		for(j=0; j<3; j++) {
			float x = sin(t + j * M_PI / 2.0);
			mball[i].pos[j] = offset[i][j] + x * scale[i][j];
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

	zsort_mesh(&mmesh);

	g3d_mtl_diffuse(0.6, 0.6, 0.6);

	draw_mesh(&mmesh);

	g3d_viewport(0, 0, fb_width, fb_height);

	swap_buffers(fb_pixels);
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
