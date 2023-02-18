#include <stdlib.h>
#include <math.h>
#include "demo.h"
#include "3dgfx.h"
#include "mesh.h"
#include "screen.h"
#include "util.h"

#define VFOV	50.0f


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);


static struct screen scr = {
	"break",
	init,
	destroy,
	start,
	0,
	draw
};

static float cam_theta = 0, cam_phi = 0;
static float cam_dist = 10;

static long part_start;

static struct g3d_mesh foomesh;


struct screen *break_screen(void)
{
	return &scr;
}


static int init(void)
{
	int i, j;

	if(load_mesh(&foomesh, "data/fracscn.obj") == -1) {
		return -1;
	}

	return 0;
}

static void destroy(void)
{
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(VFOV, 1.3333333, 0.5, 500.0);

	g3d_enable(G3D_CULL_FACE);
	//g3d_enable(G3D_DEPTH_TEST);
	g3d_enable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);
	g3d_polygon_mode(G3D_FLAT);

	g3d_clear_color(85, 70, 136);

	part_start = time_msec;
}

static void update(void)
{
	int i;

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

static void draw(void)
{
	int i;
	float tm = (float)(time_msec - part_start) / 100.0f;

	update();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, -0.5, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	}

	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);

	zsort_mesh(&foomesh);
	draw_mesh(&foomesh);

	swap_buffers(fb_pixels);
}
