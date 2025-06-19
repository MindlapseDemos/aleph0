#include <stdlib.h>
#include <math.h>
#include "demo.h"
#include "3dgfx.h"
#include "mesh.h"
#include "scene.h"
#include "screen.h"
#include "util.h"
#include "gfxutil.h"
#include "dynarr.h"

#define VFOV	60.0f


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);


static struct screen scr = {
	"reactor",
	init,
	destroy,
	start,
	0,
	draw
};

static float cam_theta, cam_phi;
static float cam_dist = 800;

static struct g3d_scene *scn;


struct screen *zoom3d_screen(void)
{
	return &scr;
}


static int init(void)
{
	struct goat3d *g;

	if(!(g = goat3d_create()) || goat3d_load(g, "data/3dzoom/reactor.g3d") == -1) {
		goat3d_free(g);
		return -1;
	}
	scn = scn_create();
	scn->texpath = "data/3dzoom";

	conv_goat3d_scene(scn, g);
	goat3d_free(g);

	return 0;
}

static void destroy(void)
{
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(VFOV, 1.3333333, 10.0, 2000.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_enable(G3D_DEPTH_TEST);
	g3d_disable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);
	g3d_polygon_mode(G3D_FLAT);

	g3d_clear_color(0, 0, 0);
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
	g3d_translate(0, -0.5, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	}

	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);

	scn_draw(scn);

	swap_buffers(fb_pixels);
}
