#include <stdio.h>
#include <stdlib.h>
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
static int gen_phong_tex(struct pimage *img, int xsz, int ysz, float sexp,
		float offx, float offy,	int dr, int dg, int db, int sr, int sg, int sb);

static struct screen scr = {
	"infcubes",
	init,
	destroy,
	start, 0,
	draw
};

static float cam_theta = -29, cam_phi = 35;
static float cam_dist = 6;
static struct pimage tex_inner, tex_outer;
static struct g3d_mesh mesh_cube, mesh_cube2;

struct screen *infcubes_screen(void)
{
	return &scr;
}

#define PHONG_TEX_SZ	128

static int init(void)
{
	static const float scalemat[16] = {-6, 0, 0, 0, 0, -6, 0, 0, 0, 0, -6, 0, 0, 0, 0, 1};
	/*
	if(!(tex_inner.pixels = img_load_pixels("data/crate.jpg", &tex_inner.width,
					&tex_inner.height, IMG_FMT_RGB24))) {
		fprintf(stderr, "infcubes: failed to load crate texture\n");
		return -1;
	}
	convimg_rgb24_rgb16(tex_inner.pixels, (unsigned char*)tex_inner.pixels, tex_inner.width, tex_inner.height);
	*/
	gen_phong_tex(&tex_inner, PHONG_TEX_SZ, PHONG_TEX_SZ, 5.0f, 0, 0, 10, 50, 92, 192, 192, 192);

	if(!(tex_outer.pixels = img_load_pixels("data/refmap1.jpg", &tex_outer.width,
					&tex_outer.height, IMG_FMT_RGB24))) {
		fprintf(stderr, "infcubes: failed to load outer texture\n");
		return -1;
	}
	convimg_rgb24_rgb16(tex_outer.pixels, (unsigned char*)tex_outer.pixels, tex_outer.width, tex_outer.height);
	/*gen_phong_tex(&tex_outer, PHONG_TEX_SZ, PHONG_TEX_SZ, 5.0f, 50, 50, 50, 255, 255, 255);*/

	/*
	if(gen_cube_mesh(&mesh_cube, 1.0f, 3) == -1) {
		return -1;
	}
	*/
	if(load_mesh(&mesh_cube, "data/bevelbox.obj") == -1) {
		return -1;
	}
	if(copy_mesh(&mesh_cube2, &mesh_cube) == -1) {
		return -1;
	}
	apply_mesh_xform(&mesh_cube2, scalemat);
	normalize_mesh_normals(&mesh_cube2);
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
	g3d_perspective(70.0, 1.3333333, 0.5, 100.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_disable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);
	g3d_disable(G3D_DEPTH_TEST);
}

static void update(void)
{
	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

static void draw(void)
{
	float t = (float)time_msec / 16.0f;
	update();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	}

	/*memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * 2);*/

	g3d_polygon_mode(G3D_FLAT);
	g3d_enable(G3D_TEXTURE_2D);
	g3d_enable(G3D_TEXTURE_GEN);

	g3d_push_matrix();
	g3d_rotate(t, 1, 0, 0);
	g3d_rotate(t, 0, 1, 0);
	g3d_set_texture(tex_outer.width, tex_outer.height, tex_outer.pixels);
	draw_mesh(&mesh_cube2);
	g3d_pop_matrix();

	g3d_set_texture(tex_inner.width, tex_inner.height, tex_inner.pixels);
	draw_mesh(&mesh_cube);
	g3d_disable(G3D_TEXTURE_GEN);

	swap_buffers(fb_pixels);
}

static int gen_phong_tex(struct pimage *img, int xsz, int ysz, float sexp,
		float offx, float offy,	int dr, int dg, int db, int sr, int sg, int sb)
{
	int i, j;
	float u, v, du, dv;
	uint16_t *pix;

	if(!(img->pixels = malloc(xsz * ysz * sizeof *pix))) {
		return -1;
	}
	pix = img->pixels;

	du = 2.0f / (float)(xsz - 1);
	dv = 2.0f / (float)(ysz - 1);

	v = -1.0f - offy;
	for(i=0; i<ysz; i++) {
		u = -1.0f - offx;
		for(j=0; j<xsz; j++) {
			float d = sqrt(u * u + v * v);
			float val = pow(cos(d * M_PI / 2.0f), sexp);
			int ival = abs(val * 255.0f);

			int r = dr + ival * sr / 256;
			int g = dg + ival * sg / 256;
			int b = db + ival * sb / 256;

			if(r > 255) r = 255;
			if(g > 255) g = 255;
			if(b > 255) b = 255;

			*pix++ = PACK_RGB16(r, g, b);

			u += du;
		}
		v += dv;
	}

	img->width = xsz;
	img->height = ysz;
	return 0;
}
