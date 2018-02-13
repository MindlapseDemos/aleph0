#include <stdio.h>
#include <math.h>
#include "demo.h"
#include "3dgfx.h"
#include "screen.h"
#include "cfgopt.h"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void draw_cube(void);

static struct screen scr = {
	"infcubes",
	init,
	destroy,
	start, 0,
	draw
};

static float cam_theta, cam_phi;
static float cam_dist = 5;

struct screen *infcubes_screen(void)
{
	return &scr;
}


static int init(void)
{
	return 0;
}

static void destroy(void)
{
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.3333333, 0.5, 100.0);

	g3d_enable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);
}

static void update(void)
{
	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

static void draw(void)
{
	update();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	}

	memset(fb_pixels, 0, fb_width * fb_height * 2);

	draw_cube();

	swap_buffers(fb_pixels);
}

static void draw_cube(void)
{
	g3d_begin(G3D_QUADS);
	g3d_normal(0, 0, 1);
	g3d_vertex(-1, -1, 1);
	g3d_vertex(1, -1, 1);
	g3d_vertex(1, 1, 1);
	g3d_vertex(-1, 1, 1);
	g3d_normal(1, 0, 0);
	g3d_vertex(1, -1, 1);
	g3d_vertex(1, -1, -1);
	g3d_vertex(1, 1, -1);
	g3d_vertex(1, 1, 1);
	g3d_normal(0, 0, -1);
	g3d_vertex(1, -1, -1);
	g3d_vertex(-1, -1, -1);
	g3d_vertex(-1, 1, -1);
	g3d_vertex(1, 1, -1);
	g3d_normal(-1, 0, 0);
	g3d_vertex(-1, -1, -1);
	g3d_vertex(-1, -1, 1);
	g3d_vertex(-1, 1, 1);
	g3d_vertex(-1, 1, -1);
	g3d_normal(0, 1, 0);
	g3d_vertex(-1, 1, 1);
	g3d_vertex(1, 1, 1);
	g3d_vertex(1, 1, -1);
	g3d_vertex(-1, 1, -1);
	g3d_normal(0, -1, 0);
	g3d_vertex(1, -1, 1);
	g3d_vertex(-1, -1, 1);
	g3d_vertex(-1, -1, -1);
	g3d_vertex(1, -1, -1);
	g3d_end();
}
