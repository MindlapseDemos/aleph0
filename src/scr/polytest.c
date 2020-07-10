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
#include "util.h"

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

static int use_zbuf;


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

	gen_texture(&tex, 64, 64);

#ifdef DEBUG_POLYFILL
	lowres_width = FB_WIDTH / LOWRES_SCALE;
	lowres_height = FB_HEIGHT / LOWRES_SCALE;
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
	g3d_perspective(50.0, 1.3333333, 1.0, 10.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_enable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);

	g3d_polygon_mode(G3D_GOURAUD);
	g3d_enable(G3D_TEXTURE_2D);

	g3d_clear_color(20, 30, 50);
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

	if(use_zbuf) {
		g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);
		g3d_enable(G3D_DEPTH_TEST);
	} else {
		g3d_clear(G3D_COLOR_BUFFER_BIT);
		g3d_disable(G3D_DEPTH_TEST);
	}


	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	} else {
		g3d_rotate(cam_phi, 1, 0, 0);
		g3d_rotate(cam_theta, 0, 1, 0);
	}

	g3d_light_dir(0, -10, 10, 10);
	g3d_mtl_diffuse(1.0, 1.0, 1.0);
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
		if(!use_zbuf) {
			zsort_mesh(&torus);
		}
		draw_mesh(&torus);
	}

	/* show zbuffer */
	/*{
		int i;
		for(i=0; i<FB_WIDTH*FB_HEIGHT; i++) {
			unsigned int z = pfill_zbuf[i];
			fb_pixels[i] = (z >> 5) & 0x7e0;
		}
	}*/

	cs_dputs(fb_pixels, 140, 0, use_zbuf ? "zbuffer" : "zsort");

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


	g3d_framebuffer(FB_WIDTH, FB_HEIGHT, fb_pixels);

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
			draw_huge_pixel(dptr, FB_WIDTH, *sptr++);
			dptr += LOWRES_SCALE;
		}
		dptr += FB_WIDTH * LOWRES_SCALE - FB_WIDTH;
	}
}

static void keypress(int key)
{
	switch(key) {
	case 'b':
		use_bsp = !use_bsp;
		printf("drawing with %s\n", use_bsp ? "BSP tree" : "z-sorting");
		break;

	case 'z':
		use_zbuf = !use_zbuf;
		printf("Z-buffer %s\n", use_zbuf ? "on" : "off");
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
			int val = (i << 2) ^ (j << 2);
			uint16_t r = val;
			uint16_t g = val << 1;
			uint16_t b = val << 2;

			*pix++ = PACK_RGB16(r, g, b);
		}
	}

	img->width = xsz;
	img->height = ysz;
	return 0;
}
