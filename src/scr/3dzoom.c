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

#define VFOV	80.0f


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void keypress(int key);


static struct screen scr = {
	"reactor",
	init,
	destroy,
	start,
	0,
	draw,
	keypress
};

static float cam_theta, cam_phi;
static float cam_dist = 80;

static struct g3d_scene *scn;
static struct g3d_mesh *poolmesh;

static struct curve campath;

static int dbgfreelook;

struct screen *zoom3d_screen(void)
{
	return &scr;
}


static int init(void)
{
	int i, num_cp;
	struct goat3d *g;

	if(!(g = goat3d_create()) || goat3d_load(g, "data/3dzoom/reactor.g3d") == -1) {
		goat3d_free(g);
		return -1;
	}
	scn = scn_create();
	scn->texpath = "data/3dzoom";

	conv_goat3d_scene(scn, g);
	goat3d_free(g);

	poolmesh = scn_find_mesh(scn, "Plane");
	poolmesh->flags |= G3DMESH_SKIP;

	if(crv_load(&campath, "data/rcampath.crv") == -1) {
		fprintf(stderr, "reactor: failed to load camera path spline\n");
		goat3d_free(g);
		return -1;
	}
	num_cp = crv_num_points(&campath);
	for(i=0; i<num_cp; i++) {
		cgm_vscale(campath.cp + i, 0.036f);
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
	g3d_perspective(VFOV, 1.3333333, 15.0, 300.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_enable(G3D_DEPTH_TEST);
	g3d_disable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);
	g3d_polygon_mode(G3D_FLAT);

	g3d_clear_color(0, 0, 0);
}

static cgm_vec3 campos, camtarg = {0};
static float cam_t;

static void update(void)
{
	cam_t = fmod((float)time_msec * 0.00005f, 1.0f);
	crv_eval(&campath, cam_t, &campos);
	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

static void draw(void)
{
	int i;
	float viewmat[16];
	float texanim = (float)time_msec / 8192.0f;
	cgm_vec3 up = {0, 1, 0};
	char buf[64];
	cgm_vec3 pos, prev;
	float t;
	int num_samples;

	update();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	if(!dbgfreelook) {
		g3d_rotate_z(0.4);
		cgm_minv_lookat(viewmat, &campos, &camtarg, &up);
		g3d_mult_matrix(viewmat);
	} else {
		g3d_translate(0, -0.5, -cam_dist);
		g3d_rotate(cam_phi, 1, 0, 0);
		g3d_rotate(cam_theta, 0, 1, 0);
		if(opt.sball) {
			g3d_mult_matrix(sball_matrix);
		}
	}

	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);

	scn_draw(scn);

	/* draw animated energy "pool" */
	g3d_enable(G3D_TEXTURE_MAT);
	g3d_matrix_mode(G3D_TEXTURE);
	g3d_load_identity();
	g3d_translate(texanim, texanim, 0);
	draw_mesh(poolmesh);
	g3d_disable(G3D_TEXTURE_MAT);
	g3d_matrix_mode(G3D_MODELVIEW);

	swap_buffers(fb_pixels);
}

static void keypress(int key)
{
	switch(key) {
	case ' ':
		dbgfreelook ^= 1;
		break;
	}
}
