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
#include "curve.h"

#define VFOV	60.0f


static int space_init(void);
static void space_destroy(void);
static void space_start(long trans_time);
static void space_draw(void);
static void space_keypress(int key);


static struct screen scr = {
	"space",
	space_init,
	space_destroy,
	space_start,
	0,
	space_draw,
	space_keypress
};

static float cam_theta, cam_phi;
static float cam_dist = 10;

static struct g3d_scene *scn;
static struct g3d_mesh *mesh_hull, *mesh_lit;

static dseq_event *ev_space;

struct screen *space_screen(void)
{
	return &scr;
}


static int space_init(void)
{
	int i, num_cp;
	struct goat3d *g;

	if(!(g = goat3d_create()) || goat3d_load(g, "data/frig8.g3d") == -1) {
		goat3d_free(g);
		return -1;
	}
	scn = scn_create();
	scn->texpath = "data";

	conv_goat3d_scene(scn, g);
	goat3d_free(g);

	mesh_hull = scn_find_mesh(scn, "hull");
	mesh_lit = scn_find_mesh(scn, "emissive");

	ev_space = dseq_lookup("space");
	return 0;
}

static void space_destroy(void)
{
}

static void space_start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(VFOV, 1.3333333, 1.0, 200.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_enable(G3D_DEPTH_TEST);
	g3d_enable(G3D_LIGHT0);
	g3d_polygon_mode(G3D_FLAT);

	g3d_clear_color(0, 0, 0);
}

static void update(void)
{
	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

static void space_draw(void)
{
	unsigned i, nmeshes;

	update();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, -0.5, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	}

	g3d_light_dir(0, 1, 0.5, 0.5);

	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);

	g3d_translate(0, 0, 3);

	g3d_enable(G3D_LIGHTING);
	draw_mesh(mesh_hull);
	g3d_disable(G3D_LIGHTING);
	draw_mesh(mesh_lit);

	swap_buffers(0);
}

static void space_keypress(int key)
{
}
