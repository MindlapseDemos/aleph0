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
static void left_side(float tint);
static void right_side(float tint);
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

static float dbg_num;
static unsigned int dbg_toggle;
static struct image testimg;

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

	if(load_image(&testimg, "data/blendtst.png") == -1) {
		fprintf(stderr, "failed to load blendtst image\n");
		return -1;
	}
	if(conv_rle_alpha(&testimg) == -1) {
		fprintf(stderr, "failed to convert blendtst image\n");
		return -1;
	}

	return 0;
}

static void credits_destroy(void)
{
}

static void credits_start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(VFOV, 1.3333333, 0.5, 100.0);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();

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
}

#define NUM_TENT	1
#define TENT_NODES	12
#define TENT_DIST	0.7

static char dbgtext[TENT_NODES][64];

static void credits_draw(void)
{
	int i;
	float clip_plane[] = {0, 1, 0, 0};

	credits_update();

	g3d_clear(G3D_DEPTH_BUFFER_BIT);

	backdrop();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, -2, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);

	g3d_clip_plane(0, clip_plane);
	g3d_enable(G3D_CLIP_PLANE0);
	g3d_disable(G3D_LIGHTING);
	g3d_polygon_mode(G3D_FLAT);

	left_side(1);
	right_side(1);

	g3d_scale(1, -1, 1);
	g3d_clip_plane(0, clip_plane);

	g3d_front_face(G3D_CW);
	left_side(0.8);
	right_side(0.8);
	g3d_front_face(G3D_CCW);

	g3d_disable(G3D_CLIP_PLANE0);

	/*for(i=0; i<TENT_NODES; i++) {
		cs_cputs(fb_pixels, 100, 10 + i * 10, dbgtext[i]);
	}*/

	if(dbg_toggle) {
		blendfb_rle(fb_pixels, -50, 0, &testimg);
	}

	swap_buffers(fb_pixels);
}

static void left_side(float tint)
{
	int i, j, otherm, curm = 0;
	float mat[2][16], angle, t, f, a;
	cgm_vec3 pos[TENT_NODES], norm[TENT_NODES], pprev, nprev;
	cgm_vec3 tang;

	for(j=0; j<NUM_TENT; j++) {
		g3d_push_matrix();
		g3d_translate(-3, 0, 0);

		t = (float)time_msec * 0.001 + j * (59.0 + dbg_num);

		cgm_midentity(mat[1]);

		for(i=0; i<TENT_NODES; i++) {
			otherm = (curm + 1) & 1;
			f = 1.0 + (float)i * 0.4;
			a = 0.2 + (float)(i + 1) * 0.06;
			cgm_midentity(mat[curm]);
			cgm_mtranslate(mat[curm], 0, TENT_DIST, 0);
			angle = sin(t + f) * a;
			if(j == 0) {
				sprintf(dbgtext[i], "%+.2f (t:%.2f|f:%.2f|a:%.2f)", angle, t, f, a);
			}
			cgm_mrotate_z(mat[curm], angle);
			cgm_mmul(mat[curm], mat[otherm]);

			cgm_vcons(pos + i, 0, 0, 0);
			cgm_vmul_m4v3(pos + i, mat[curm]);
			curm = otherm;
		}

		cgm_vcons(&pprev, 0, 0, 0);
		for(i=0; i<TENT_NODES; i++) {
			float s = (1.0 - (float)i / (float)TENT_NODES) * 0.5 + 0.05;
			cgm_vcsub(&tang, pos + i, &pprev);
			cgm_vcons(norm + i, -tang.y * s, tang.x * s, 0);
			pprev = pos[i];
		}


		g3d_disable(G3D_LIGHTING);

		cgm_vcons(&pprev, 0, 0, 0);
		cgm_vcons(&nprev, -TENT_DIST * 0.5, 0, 0);
		g3d_begin(G3D_QUADS);
		g3d_color3f(0.15 * tint, 0.85 * tint, 0.15 * tint);
		for(i=0; i<TENT_NODES; i++) {
			g3d_vertex(pprev.x + nprev.x, pprev.y + nprev.y, 0);
			g3d_vertex(pprev.x - nprev.x, pprev.y - nprev.y, 0);
			g3d_vertex(pos[i].x - norm[i].x, pos[i].y - norm[i].y, 0);
			g3d_vertex(pos[i].x + norm[i].x, pos[i].y + norm[i].y, 0);
			pprev = pos[i];
			nprev = norm[i];
		}
		g3d_end();

		g3d_pop_matrix();
	}
}

static void right_side(float tint)
{
}

static void credits_keyb(int key)
{
	switch(key) {
	case '=':
		dbg_num += 0.5;
		printf("dbg: %f\n", dbg_num);
		break;
	case '-':
		dbg_num -= 0.5;
		printf("dbg: %f\n", dbg_num);
		break;

	case 'd':
		dbg_toggle ^= 1;
		break;
	}
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
