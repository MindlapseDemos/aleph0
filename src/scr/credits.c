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
#include "font.h"

#define VFOV	60

static int credits_init(void);
static void credits_destroy(void);
static void credits_start(long trans_time);
static void credits_draw(void);
static void left_side(float tint);
static void right_side(float tint);
static void credits_keyb(int key);
static void backdrop(float theta, float phi);

static struct screen scr = {
	"credits",
	credits_init,
	credits_destroy,
	credits_start, 0,
	credits_draw,
	credits_keyb
};

static float cam_theta, cam_phi;
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

static struct image envmap;

static float dbg_num;
static unsigned int dbg_toggle;

#define CRD_LINES	16
static const char *crd_text[CRD_LINES] = {
	"CODE",
	"  NUCLEAR",
	"  OPTIMUS",
	"  SAMURAI",
	"",
	"GRAPHICS",
	"  DSTAR",
	"  JADE",
	"  LUTHER",
	"",
	"MUSIC",
	"  NO@XS",
	"  MAZE",
	"",
	"ORGANIZER",
	"  RAMON"
};
static float crd_texv[CRD_LINES + 1];
static struct image ctex;

static dseq_event *ev_emerge, *ev_text, *ev_fade, *ev_ampl;


struct screen *credits_screen(void)
{
	return &scr;
}

static int credits_init(void)
{
	int i, y, adv;
	int col[3];
	float xform[16];
	uint16_t *src, *dst;
	/*struct font demofont;*/

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

	/*if(load_font(&demofont, "data/aleph0.gmp", PACK_RGB16(255, 220, 192)) == -1) {
		return -1;
	}*/

	/* prepare credits texture */
	init_image(&ctex, 256, 512, 0, 256);
	if(!(ctex.pixels = calloc(256 * 300, 2))) {
		fprintf(stderr, "failed to allocate credits texture\n");
		return -1;
	}

	adv = demofont.advance * 3 / 2;
	y = 0;
	for(i=0; i<CRD_LINES; i++) {
		crd_texv[i] = (float)y / 512.0f;
		draw_text_img(&demofont, &ctex, 0, y + adv / 5, crd_text[i]);
		y += adv;
	}
	crd_texv[CRD_LINES] = (float)y / 512.0f;
	/*destroy_font(&demofont);*/

	/* load envmap */
	if(load_image(&envmap, "data/crenv.jpg") == -1) {
		fprintf(stderr, "failed to load credits envmap\n");
		return -1;
	}

	ev_emerge = dseq_lookup("credits.emerge");
	ev_text = dseq_lookup("credits.text");
	ev_fade = dseq_lookup("credits.fade");
	ev_ampl = dseq_lookup("credits.ampl");

	return 0;
}

static void credits_destroy(void)
{
	destroy_image(&envmap);
}

static void credits_start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(VFOV, 1.3333333, 0.5, 100.0);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();

	g3d_disable(G3D_DEPTH_TEST);
	g3d_enable(G3D_CULL_FACE);
	g3d_disable(G3D_LIGHTING);

	g3d_texture_mode(G3D_TEX_MODULATE);

	g3d_polygon_mode(G3D_GOURAUD);
	g3d_light_ambient(0, 0, 0);
}

static void credits_update(void)
{
	int i;
	float rot;

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

#define NUM_TENT	3
#define TENT_NODES	12
static const float tent_dist[] = {0.9, 0.7, 0.5};

static void credits_draw(void)
{
	int i;
	float clip_plane[] = {0, 1, 0, 0};
	float t = (float)time_msec / 2000.0f;
	float theta = cam_theta + cos(t) * 8.0f;
	float phi = cam_phi + sin(t * 1.2f) * 2.0f;
	float dist;

	dist = cam_dist/* + dseq_value(ev_zoom)*/;

	credits_update();

	/*g3d_clear(G3D_DEPTH_BUFFER_BIT);*/

	backdrop(theta, phi);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, -2.5, -dist);
	g3d_rotate(phi, 1, 0, 0);
	g3d_rotate(theta, 0, 1, 0);

	g3d_clip_plane(0, clip_plane);
	g3d_enable(G3D_CLIP_PLANE0);
	g3d_polygon_mode(G3D_FLAT);

	left_side(1);
	right_side(1);

	g3d_scale(1, -1, 1);
	g3d_clip_plane(0, clip_plane);

	g3d_front_face(G3D_CW);
	left_side(0.5);
	right_side(0.5);
	g3d_front_face(G3D_CCW);

	g3d_disable(G3D_CLIP_PLANE0);

	if(dseq_isactive(ev_fade)) {
		unsigned int i, r, g, b, pcol;
		unsigned int val = 256 - cround64(dseq_param(ev_fade) * 256.0f);
		uint16_t *ptr = fb_pixels;
		for(i=0; i<320 * 240; i++) {
			pcol = *ptr;
			r = (UNPACK_R16(pcol) * val) >> 8;
			g = (UNPACK_G16(pcol) * val) >> 8;
			b = (UNPACK_B16(pcol) * val) >> 8;
			*ptr++ = PACK_RGB16(r, g, b);
		}
	}

	swap_buffers(0);
}

static void left_side(float tint)
{
	int i, j, otherm, curm = 0;
	float mat[2][16], angle, t, f, a, tampl, y;
	cgm_vec3 pos[TENT_NODES], norm[TENT_NODES], pprev, nprev, nvec, pvec;
	cgm_vec3 tang;
	float rad[TENT_NODES], sprev;

	if(dseq_started()) {
		tampl = dseq_value(ev_ampl);
		y = (cgm_logerp(1, 9, dseq_value(ev_emerge)) - 9);
	} else {
		tampl = 1.0f;
		y = 0;
	}

	for(j=0; j<NUM_TENT; j++) {
		g3d_push_matrix();
		g3d_translate(-3, y, 0);

		t = (float)time_msec * 0.001 + j * (59.0 + dbg_num);

		cgm_midentity(mat[1]);

		for(i=0; i<TENT_NODES; i++) {
			otherm = (curm + 1) & 1;
			f = 1.0 + (float)i * 0.4;
			a = cgm_lerp(0.15f, 0.2 + (float)(i + 1) * 0.06, tampl);
			cgm_midentity(mat[curm]);
			cgm_mtranslate(mat[curm], 0, tent_dist[j], 0);
			angle = sin(t + f) * a;
			cgm_mrotate_z(mat[curm], angle);
			cgm_mmul(mat[curm], mat[otherm]);

			cgm_vcons(pos + i, 0, 0, 0);
			cgm_vmul_m4v3(pos + i, mat[curm]);
			curm = otherm;
		}

		cgm_vcons(&pprev, 0, 0, 0);
		for(i=0; i<TENT_NODES; i++) {
			rad[i] = (1.0 - (float)i / (float)TENT_NODES) * 0.5 + 0.05;
			cgm_vcsub(&tang, pos + i, &pprev);
			cgm_vcons(norm + i, -tang.y, tang.x, 0);
			pprev = pos[i];
		}


		g3d_enable(G3D_TEXTURE_2D);
		g3d_set_texture(envmap.width, envmap.height, envmap.pixels);
		g3d_enable(G3D_TEXTURE_GEN);

		sprev = tent_dist[j] * 0.5f;
		cgm_vcons(&pprev, 0, 0, 0);
		cgm_vcons(&nprev, -sprev, 0, 0);
		pvec = nprev;
		g3d_begin(G3D_QUADS);
		g3d_color3f(tint, tint, tint);
		for(i=0; i<TENT_NODES; i++) {
			nvec.x = norm[i].x * rad[i];
			nvec.y = norm[i].y * rad[i];

			g3d_normal(nprev.x, nprev.y, 0);
			g3d_vertex(pprev.x + pvec.x, pprev.y + pvec.y, 0);
			g3d_normal(0, 0, 1);
			g3d_vertex(pprev.x, pprev.y, sprev);
			g3d_vertex(pos[i].x, pos[i].y, rad[i]);
			g3d_normal(norm[i].x, norm[i].y, 0);
			g3d_vertex(pos[i].x + nvec.x, pos[i].y + nvec.y, 0);

			g3d_normal(0, 0, 1);
			g3d_vertex(pprev.x, pprev.y, sprev);
			g3d_normal(-nprev.x, -nprev.y, 0);
			g3d_vertex(pprev.x - pvec.x, pprev.y - pvec.y, 0);
			g3d_normal(-norm[i].x, -norm[i].y, 0);
			g3d_vertex(pos[i].x - nvec.x, pos[i].y - nvec.y, 0);
			g3d_normal(0, 0, 1);
			g3d_vertex(pos[i].x, pos[i].y, rad[i]);

			pprev = pos[i];
			nprev = norm[i];
			pvec = nvec;
			sprev = rad[i];
		}
		g3d_end();

		g3d_disable(G3D_TEXTURE_GEN);
		g3d_disable(G3D_TEXTURE_2D);

		/*
		g3d_begin(G3D_LINES);
		g3d_color3f(0.15 * tint, 0.15 * tint, 0.65 * tint);
		for(i=0; i<TENT_NODES; i++) {
			nvec.x = norm[i].x * rad[i];
			nvec.y = norm[i].y * rad[i];
			g3d_vertex(pos[i].x + nvec.x, pos[i].y + nvec.y, 0);
			g3d_vertex(pos[i].x + nvec.x + norm[i].x, pos[i].y + nvec.y + norm[i].y, 0);

			g3d_vertex(pos[i].x, pos[i].y, rad[i]);
			g3d_vertex(pos[i].x, pos[i].y, rad[i] + 1);

			g3d_vertex(pos[i].x - nvec.x, pos[i].y - nvec.y, 0);
			g3d_vertex(pos[i].x - nvec.x - norm[i].x, pos[i].y - nvec.y - norm[i].y, 0);
		}
		g3d_end();
		*/

		g3d_pop_matrix();
	}
}

#define TXQUAD_SUB	4
#define DX			(5.0f / TXQUAD_SUB)
#define DU			(0.5f / TXQUAD_SUB)
#define DY			1.0f

static void right_side(float tint)
{
	int i, j;
	float x, y, u, v0, v1, fade, t;

	g3d_push_matrix();
	g3d_translate(0, 2, 0);

	g3d_polygon_mode(G3D_FLAT);
	g3d_enable(G3D_TEXTURE_2D);
	g3d_enable(G3D_ADD_BLEND);
	/*g3d_disable(G3D_DEPTH_TEST);*/
	g3d_set_texture(ctex.width, ctex.height, ctex.pixels);

	y = dseq_param(ev_text) * 21.0f;
	//y = (time_msec & 0x7fff) / 1024.0f - 2.0f;
	g3d_begin(G3D_QUADS);
	for(i=0; i<CRD_LINES; i++) {
		if(crd_text[i][0] == 0) {
			y -= DY;
			continue;
		}
		t = y / 5.8f;
		if(t < 0.0f) t = 0.0f; else if(t > 1.0f) t = 1.0f;
		fade = sin(t * CGM_PI);
		if(fade < 0.01) {
			y -= DY;
			continue;
		}
		u = 0;
		x = fade * 0.5f + 1;//-1;
		v0 = crd_texv[i];
		v1 = crd_texv[i + 1];
		g3d_color3f(fade * tint, fade * tint, fade * tint);
		for(j=0; j<TXQUAD_SUB; j++) {
			g3d_texcoord(u, v1);
			g3d_vertex(x, y - 2, 0.1);
			g3d_texcoord(u + DU, v1);
			g3d_vertex(x + DX, y - 2, 0.1);
			g3d_texcoord(u + DU, v0);
			g3d_vertex(x + DX, y - 1, 0.1);
			g3d_texcoord(u, v0);
			g3d_vertex(x, y - 1, 0.1);
			x += DX;
			u += DU;
		}
		y -= DY;
	}
	g3d_end();

	g3d_disable(G3D_ADD_BLEND);
	g3d_disable(G3D_TEXTURE_2D);
	/*g3d_enable(G3D_DEPTH_TEST);*/

	g3d_pop_matrix();
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

static void backdrop(float theta, float phi)
{
	int i, j, hory;
	uint16_t *fbptr, pcol;
	int cidx, offs = -10;
	int startidx;

	startidx = cround64(theta * (float)BGOFFS_SIZE) / 360;

	hory = (fb_height - ((2 * fb_height * cround64((phi * 256.0f)) / VFOV) >> 8)) / 2;
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
