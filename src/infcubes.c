#include <stdio.h>
#include <string.h>
#include <math.h>
#include "demo.h"
#include "3dgfx.h"
#include "screen.h"
#include "cfgopt.h"
#include "polyfill.h"
#include "imago2.h"
#include "gfxutil.h"
#include "mesh.h"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void draw_cube(float sz);

static struct screen scr = {
	"infcubes",
	init,
	destroy,
	start, 0,
	draw
};

static float cam_theta = -29, cam_phi = 35;
static float cam_dist = 5;
static struct pimage tex_inner, tex_outer;
static struct g3d_mesh mesh_cube;

struct screen *infcubes_screen(void)
{
	return &scr;
}


static int init(void)
{
	if(!(tex_inner.pixels = img_load_pixels("data/crate.jpg", &tex_inner.width,
					&tex_inner.height, IMG_FMT_RGB24))) {
		fprintf(stderr, "infcubes: failed to load crate texture\n");
		return -1;
	}
	convimg_rgb24_rgb16(tex_inner.pixels, (unsigned char*)tex_inner.pixels, tex_inner.width, tex_inner.height);

	if(!(tex_outer.pixels = img_load_pixels("data/steelfrm.jpg", &tex_outer.width,
					&tex_outer.height, IMG_FMT_RGB24))) {
		fprintf(stderr, "infcubes: failed to load ornamental texture\n");
		return -1;
	}
	convimg_rgb24_rgb16(tex_outer.pixels, (unsigned char*)tex_outer.pixels, tex_outer.width, tex_outer.height);

	if(gen_cube_mesh(&mesh_cube, 1.0f, 3) == -1) {
		return -1;
	}
	return 0;
}

static void destroy(void)
{
	img_free_pixels(tex_inner.pixels);
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.3333333, 0.5, 100.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_disable(G3D_LIGHTING);
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

	g3d_polygon_mode(G3D_TEX);

	g3d_push_matrix();
	g3d_scale(-6, -6, -6);
	g3d_set_texture(tex_outer.width, tex_outer.height, tex_outer.pixels);
	draw_mesh(&mesh_cube);
	g3d_pop_matrix();

	g3d_set_texture(tex_inner.width, tex_inner.height, tex_inner.pixels);
	draw_mesh(&mesh_cube);

	swap_buffers(fb_pixels);
}
