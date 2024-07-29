#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "cgmath/cgmath.h"
#include "screen.h"
#include "demo.h"
#include "3dgfx.h"
#include "gfxutil.h"
#include "util.h"
#include "mesh.h"
#include "imago2.h"
#include "noise.h"

#define VFOV	60.0f

static int credits_init(void);
static void credits_destroy(void);
static void credits_start(long trans_time);
static void credits_draw(void);
static void credits_keyb(int key);
static void backdrop(void);

static struct screen scr = {
	"credits",
	credits_init,
	credits_destroy,
	credits_start, 0,
	credits_draw,
	credits_keyb
};

static float cam_theta, cam_phi = 15;
static float cam_dist = 10;

#define BGCOL_SIZE	128
#define BGOFFS_SIZE	1024
static uint16_t bgcol[BGCOL_SIZE];
static uint16_t bgcol_mir[BGCOL_SIZE];
static int bgoffs[BGOFFS_SIZE];
static const int colzen[] = {98, 64, 192};
static const int colhor[] = {128, 80, 64};
static const int colmnt[] = {16, 9, 24};
static uint16_t mountcol, mountcol_mir;

#define NUM_TORI	4
#define TORUS_SIZE	4.0f
#define TORUS_XPOS	-3.0f
#define TORUS_YPOS	3.2f
struct g3d_mesh tori[NUM_TORI];
static float tormat[NUM_TORI][16];


struct screen *credits_screen(void)
{
	return &scr;
}

static int credits_init(void)
{
	int i;
	int col[3];
	float xform[16];

	mountcol = PACK_RGB16(colmnt[0], colmnt[1], colmnt[2]);
	mountcol_mir = PACK_RGB16(colmnt[0] / 2, colmnt[1] / 2, colmnt[2] / 2);

	for(i=0; i<BGCOL_SIZE; i++) {
		int32_t t = (i << 8) / BGCOL_SIZE;
		col[0] = colhor[0] + ((colzen[0] - colhor[0]) * t >> 8);
		col[1] = colhor[1] + ((colzen[1] - colhor[1]) * t >> 8);
		col[2] = colhor[2] + ((colzen[2] - colhor[2]) * t >> 8);
		bgcol[i] = PACK_RGB16(col[0], col[1], col[2]);
		bgcol_mir[i] = PACK_RGB16(col[0] / 2, col[1] / 2, col[2] / 2);
	}

	for(i=0; i<BGOFFS_SIZE; i++) {
		float x = 8.0f * (float)i / (float)BGOFFS_SIZE;

		bgoffs[i] = pfbm1(x, 8.0f, 5) * 32 + 16;
	}

	for(i=0; i<NUM_TORI; i++) {
		float rad = TORUS_SIZE * (i + 1) * 0.65f / NUM_TORI;
		gen_torus_mesh(tori + i, rad, rad * 0.15, 16, 6);

		cgm_mrotation_x(xform, (float)rand() * CGM_PI / (float)RAND_MAX);
		apply_mesh_xform(tori + i, xform);
	}

	return 0;
}

static void credits_destroy(void)
{
	int i;

	for(i=0; i<NUM_TORI; i++) {
		destroy_mesh(tori + i);
	}
}

static void credits_start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(VFOV, 1.3333333, 0.5, 100.0);

	g3d_enable(G3D_DEPTH_TEST);
	g3d_enable(G3D_CULL_FACE);
	g3d_enable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);

	g3d_polygon_mode(G3D_GOURAUD);
	g3d_light_ambient(0, 0, 0);
}

static void credits_update(void)
{
	int i;
	float rot;

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);

	rot = (float)time_msec / 500.0f;
	for(i=0; i<NUM_TORI; i++) {
		cgm_mrotation_x(tormat[i], rot);
		cgm_mrotate_y(tormat[i], rot);
		rot *= 0.8f;
	}
}

static void credits_draw(void)
{
	int i;

	credits_update();

	g3d_clear(G3D_DEPTH_BUFFER_BIT);

	backdrop();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, -2, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);

	g3d_light_color(0, 0.3, 0.3, 0.3);
	g3d_push_matrix();
	g3d_translate(TORUS_XPOS, -TORUS_YPOS, 0);
	g3d_scale(1, -1, 1);
	g3d_front_face(G3D_CW);

	g3d_light_dir(0, 0, -1, -1);

	for(i=0; i<NUM_TORI; i++) {
		g3d_push_matrix();
		g3d_mult_matrix(tormat[i]);
		draw_mesh(tori + i);
		g3d_pop_matrix();
	}
	g3d_front_face(G3D_CCW);
	g3d_pop_matrix();

	g3d_translate(TORUS_XPOS, TORUS_YPOS, 0);
	g3d_light_color(0, 1, 1, 1);
	for(i=0; i<NUM_TORI; i++) {
		g3d_push_matrix();
		g3d_mult_matrix(tormat[i]);
		draw_mesh(tori + i);
		g3d_pop_matrix();
	}

	swap_buffers(fb_pixels);
}

static void credits_keyb(int key)
{
}

static void backdrop(void)
{
	int i, j, hory;
	uint16_t *fbptr, pcol;
	int cidx, offs = -10;
	int startidx;

	startidx = cround64(cam_theta * (float)BGOFFS_SIZE) / 360;

	hory = (fb_height - 2 * fb_height * cround64(cam_phi) / VFOV) / 2;
	if(hory > fb_height) hory = fb_height;

	if(hory > 0) {
		fbptr = fb_pixels + (hory - 1) * fb_width;
		cidx = offs;
		i = 0;
		while(fbptr >= fb_pixels) {
			pcol = bgcol[cidx < 0 ? 0 : (cidx >= BGCOL_SIZE ? BGCOL_SIZE - 1 : cidx)];
			for(j=0; j<fb_width; j++) {
				if(cidx < bgoffs[(startidx + j) & (BGOFFS_SIZE - 1)]) {
					fbptr[j] = mountcol;
				} else {
					fbptr[j] = pcol;
				}
			}
			fbptr -= fb_width;
			cidx++;
		}
		cidx = offs;
	} else {
		cidx = offs - hory;
		hory = 0;
	}

	fbptr = fb_pixels + hory * fb_width;
	for(i=hory; i<fb_height; i++) {
		pcol = bgcol_mir[cidx < 0 ? 0 : (cidx >= BGCOL_SIZE ? BGCOL_SIZE - 1 : cidx)];
		for(j=0; j<fb_width; j++) {
			if(cidx < bgoffs[(startidx + j) & (BGOFFS_SIZE - 1)]) {
				*fbptr++ = mountcol_mir;
			} else {
				*fbptr++ = pcol;
			}
		}
		cidx++;
	}
}
