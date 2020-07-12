#include <stdio.h>
#include "demo.h"
#include "3dgfx.h"
#include "screen.h"
#include "gfxutil.h"
#include "mesh.h"
#include "image.h"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"cybersun",
	init,
	destroy,
	start,
	0,
	draw
};

static float cam_theta = 0, cam_phi = 10;
static float cam_dist = 6;

static struct g3d_mesh gmesh;
#define GMESH_GRIDSZ	20
#define GMESH_SIZE		50
static struct image gtex;

struct screen *cybersun_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j;

	if(gen_plane_mesh(&gmesh, GMESH_SIZE, GMESH_SIZE, GMESH_GRIDSZ, GMESH_GRIDSZ) == -1) {
		return -1;
	}
	for(i=0; i<gmesh.vcount; i++) {
		gmesh.varr[i].u *= GMESH_GRIDSZ;
		gmesh.varr[i].v *= GMESH_GRIDSZ;
	}
	if(load_image(&gtex, "data/pgrid.png") == -1) {
		return -1;
	}

	return 0;
}

static void destroy(void)
{
	destroy_mesh(&gmesh);
	destroy_image(&gtex);
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.3333333, 0.5, 500.0);

	g3d_enable(G3D_CULL_FACE);

	g3d_clear_color(0, 0, 0);
}

static void draw(void)
{
	int i;

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	}

	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);

	g3d_set_texture(gtex.width, gtex.height, gtex.pixels);
	g3d_enable(G3D_TEXTURE_2D);

	g3d_push_matrix();
	g3d_rotate(-90, 1, 0, 0);
	draw_mesh(&gmesh);
	g3d_pop_matrix();

	g3d_disable(G3D_TEXTURE_2D);

	swap_buffers(fb_pixels);
}
