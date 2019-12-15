#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "screen.h"
#include "demo.h"
#include "3dgfx.h"
#include "gfxutil.h"
#include "polyfill.h"	/* just for struct pimage */
#include "cfgopt.h"
#include "mesh.h"
#include "bsptree.h"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void draw_lowres_raster(void);
static void keypress(int key);
static int gen_texture(struct pimage *img, int xsz, int ysz);

static struct screen scr = {
	"polytest",
	init,
	destroy,
	start, 0,
	draw,
	keypress
};

static float cam_theta, cam_phi = 25;
static float cam_dist = 3;
static struct g3d_mesh cube, torus;
static struct bsptree torus_bsp;

static struct pimage tex;

static int use_bsp = 0;

#define LOWRES_SCALE	10
static uint16_t *lowres_pixels;
static int lowres_width, lowres_height;

struct screen *polytest_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i;

	gen_cube_mesh(&cube, 1.0, 0);
	gen_torus_mesh(&torus, 1.0, 0.25, 24, 12);
	/* scale texcoords */
	for(i=0; i<torus.vcount; i++) {
		torus.varr[i].u *= 4.0;
		torus.varr[i].v *= 2.0;
	}

	/*
	init_bsp(&torus_bsp);
	if(bsp_add_mesh(&torus_bsp, &torus) == -1) {
		fprintf(stderr, "failed to construct torus BSP tree\n");
		return -1;
	}
	*/

	gen_texture(&tex, 128, 128);

#ifdef DEBUG_POLYFILL
	lowres_width = fb_width / LOWRES_SCALE;
	lowres_height = fb_height / LOWRES_SCALE;
	lowres_pixels = malloc(lowres_width * lowres_height * 2);
	scr.draw = draw_debug;
#endif

	return 0;
}

static void destroy(void)
{
	free(lowres_pixels);
	free(cube.varr);
	free(torus.varr);
	free(torus.iarr);
	/*
	destroy_bsp(&torus_bsp);
	*/
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
	g3d_enable(G3D_TEXTURE_2D);
}

static void update(void)
{
	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}


static void draw(void)
{
	float vdir[3];
	const float *mat;

	update();

	memset(fb_pixels, 0, fb_width * fb_height * 2);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	} else {
		g3d_rotate(cam_phi, 1, 0, 0);
		g3d_rotate(cam_theta, 0, 1, 0);
	}

	g3d_light_pos(0, -10, 10, 20);

	g3d_mtl_diffuse(0.4, 0.7, 1.0);
	g3d_set_texture(tex.width, tex.height, tex.pixels);

	if(use_bsp) {
		/* calc world-space view direction */
		mat = g3d_get_matrix(G3D_MODELVIEW, 0);
		/* transform (0, 0, -1) with transpose(mat3x3) */
		vdir[0] = -mat[2];
		vdir[1] = -mat[6];
		vdir[2] = -mat[10];

		draw_bsp(&torus_bsp, vdir[0], vdir[1], vdir[2]);
	} else {
		zsort_mesh(&torus);
		draw_mesh(&torus);
	}

	/*draw_mesh(&cube);*/
	swap_buffers(fb_pixels);
}

static void draw_debug(void)
{
	update();

	memset(lowres_pixels, 0, lowres_width * lowres_height * 2);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);

	g3d_framebuffer(lowres_width, lowres_height, lowres_pixels);
	/*zsort(&torus);*/
	draw_mesh(&cube);

	draw_lowres_raster();


	g3d_framebuffer(fb_width, fb_height, fb_pixels);

	g3d_polygon_mode(G3D_WIRE);
	draw_mesh(&cube);
	g3d_polygon_mode(G3D_FLAT);

	swap_buffers(fb_pixels);
}


static void draw_huge_pixel(uint16_t *dest, int dest_width, uint16_t color)
{
	int i, j;
	uint16_t grid_color = PACK_RGB16(127, 127, 127);

	for(i=0; i<LOWRES_SCALE; i++) {
		for(j=0; j<LOWRES_SCALE; j++) {
			dest[j] = i == 0 || j == 0 ? grid_color : color;
		}
		dest += dest_width;
	}
}

static void draw_lowres_raster(void)
{
	int i, j;
	uint16_t *sptr = lowres_pixels;
	uint16_t *dptr = fb_pixels;

	for(i=0; i<lowres_height; i++) {
		for(j=0; j<lowres_width; j++) {
			draw_huge_pixel(dptr, fb_width, *sptr++);
			dptr += LOWRES_SCALE;
		}
		dptr += fb_width * LOWRES_SCALE - fb_width;
	}
}

static void keypress(int key)
{
	switch(key) {
	case 'b':
		use_bsp = !use_bsp;
		printf("drawing with %s\n", use_bsp ? "BSP tree" : "z-sorting");
		break;
	}
}

static int gen_texture(struct pimage *img, int xsz, int ysz)
{
	int i, j;
	uint16_t *pix;

	if(!(img->pixels = malloc(xsz * ysz * sizeof *pix))) {
		return -1;
	}
	pix = img->pixels;

	for(i=0; i<ysz; i++) {
		for(j=0; j<xsz; j++) {
			int val = i ^ j;

			*pix++ = PACK_RGB16(val, val, val);
		}
	}

	img->width = xsz;
	img->height = ysz;
	return 0;
}
