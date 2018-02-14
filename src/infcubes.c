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
static struct pimage tex_crate;

struct screen *infcubes_screen(void)
{
	return &scr;
}


static int init(void)
{
	int i, npixels;
	unsigned char *src;
	uint16_t *dst;

	if(!(tex_crate.pixels = img_load_pixels("data/crate.jpg", &tex_crate.width,
					&tex_crate.height, IMG_FMT_RGB24))) {
		fprintf(stderr, "infcubes: failed to load crate texture\n");
		return -1;
	}

	npixels = tex_crate.width * tex_crate.height;
	src = (unsigned char*)tex_crate.pixels;
	dst = tex_crate.pixels;
	for(i=0; i<npixels; i++) {
		int r = *src++;
		int g = *src++;
		int b = *src++;
		*dst++ = PACK_RGB16(r, g, b);
	}
	return 0;
}

static void destroy(void)
{
	img_free_pixels(tex_crate.pixels);
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.3333333, 0.5, 100.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_disable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);

	g3d_set_texture(tex_crate.width, tex_crate.height, tex_crate.pixels);
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

	g3d_polygon_mode(G3D_FLAT);
	draw_cube(-6);

	g3d_polygon_mode(G3D_TEX);
	draw_cube(1);

	swap_buffers(fb_pixels);
}

static void draw_cube(float sz)
{
	float hsz = sz * 0.5f;
	g3d_begin(G3D_QUADS);
	g3d_color3b(255, 0, 0);
	g3d_normal(0, 0, 1);
	g3d_texcoord(0, 0); g3d_vertex(-hsz, -hsz, hsz);
	g3d_texcoord(1, 0); g3d_vertex(hsz, -hsz, hsz);
	g3d_texcoord(1, 1); g3d_vertex(hsz, hsz, hsz);
	g3d_texcoord(0, 1); g3d_vertex(-hsz, hsz, hsz);
	g3d_color3b(0, 255, 0);
	g3d_normal(1, 0, 0);
	g3d_texcoord(0, 0); g3d_vertex(hsz, -hsz, hsz);
	g3d_texcoord(1, 0); g3d_vertex(hsz, -hsz, -hsz);
	g3d_texcoord(1, 1); g3d_vertex(hsz, hsz, -hsz);
	g3d_texcoord(0, 1); g3d_vertex(hsz, hsz, hsz);
	g3d_color3b(0, 0, 255);
	g3d_normal(0, 0, -1);
	g3d_texcoord(0, 0); g3d_vertex(hsz, -hsz, -hsz);
	g3d_texcoord(1, 0); g3d_vertex(-hsz, -hsz, -hsz);
	g3d_texcoord(1, 1); g3d_vertex(-hsz, hsz, -hsz);
	g3d_texcoord(0, 1); g3d_vertex(hsz, hsz, -hsz);
	g3d_color3b(255, 0, 255);
	g3d_normal(-1, 0, 0);
	g3d_texcoord(0, 0); g3d_vertex(-hsz, -hsz, -hsz);
	g3d_texcoord(1, 0); g3d_vertex(-hsz, -hsz, hsz);
	g3d_texcoord(1, 1); g3d_vertex(-hsz, hsz, hsz);
	g3d_texcoord(0, 1); g3d_vertex(-hsz, hsz, -hsz);
	g3d_color3b(255, 255, 0);
	g3d_normal(0, 1, 0);
	g3d_texcoord(0, 0); g3d_vertex(-hsz, hsz, hsz);
	g3d_texcoord(1, 0); g3d_vertex(hsz, hsz, hsz);
	g3d_texcoord(1, 1); g3d_vertex(hsz, hsz, -hsz);
	g3d_texcoord(0, 1); g3d_vertex(-hsz, hsz, -hsz);
	g3d_color3b(0, 255, 255);
	g3d_normal(0, -1, 0);
	g3d_texcoord(0, 0); g3d_vertex(hsz, -hsz, hsz);
	g3d_texcoord(1, 0); g3d_vertex(-hsz, -hsz, hsz);
	g3d_texcoord(1, 1); g3d_vertex(-hsz, -hsz, -hsz);
	g3d_texcoord(0, 1); g3d_vertex(hsz, -hsz, -hsz);
	g3d_end();
}
