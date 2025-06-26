#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cgmath/cgmath.h"
#include "demo.h"
#include "3dgfx.h"
#include "mesh.h"
#include "screen.h"
#include "util.h"
#include "gfxutil.h"

#define VFOV	50.0f
#define HFOV	(VFOV * 1.333333f)


struct hextile {
	float x, y, height;
	unsigned char r, g, b;
	int hidden;
};

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void stop(long trans_time);
static void draw(void);
static void draw_hextile(struct hextile *tile, float t);
static void keypress(int key);


static struct screen scr = {
	"hexfloor",
	init,
	destroy,
	start,
	stop,
	draw,
	keypress
};

static float cam_theta = 0, cam_phi = 20;
static float cam_dist = 12;

static long part_start;

static struct g3d_vertex hextop[6];
static struct g3d_vertex hexsides[24];
static uint16_t hextop_idx[12] = {
	0, 5, 4, 0, 4, 3, 0, 3, 1, 1, 3, 2
};
static uint16_t hexsides_idx[] = {
	13, 12, 0, 1,
	15, 14, 2, 3,
	17, 16, 4, 5,
	19, 18, 6, 7,
	21, 20, 8, 9,
	23, 22, 10, 11
};

/* orig: 8/32, radsq: 140 */
#define MAX_RAD_SQ	180.0f
#define TILES_XSZ	9
#define TILES_YSZ	32
#define NUM_TILES	(TILES_XSZ * TILES_YSZ)
static struct hextile tiles[NUM_TILES];

#define OBJ_HEIGHT	5.0f


struct screen *hexfloor_screen(void)
{
	return &scr;
}

#define HEX_SCALE		0.9f
#define HALF_HEX_ANGLE	(M_PI / 6.0f)

static int init(void)
{
	int i, j;
	float angle[6];
	struct hextile *tile;
	struct g3d_vertex *vptr;
	float nx, nz;

	for(i=0; i<6; i++) {
		angle[i] = (float)i / 3.0f * M_PI;
		hextop[i].x = cos(angle[i]) * HEX_SCALE;
		hextop[i].z = sin(angle[i]) * HEX_SCALE;
		hextop[i].y = 0;

		hextop[i].nx = hextop[i].nz = 0;
		hextop[i].ny = 1;

		hextop[i].r = 255;
		hextop[i].g = 255;
		hextop[i].b = 255;

		hexsides[i * 2] = hextop[i];
		hexsides[(i * 2 + 11) % 12] = hextop[i];
	}

	for(i=0; i<6; i++) {
		nx = cos(angle[i] + HALF_HEX_ANGLE);
		nz = sin(angle[i] + HALF_HEX_ANGLE);
		hexsides[i * 2].nx = nx;
		hexsides[i * 2].nz = nz;
		hexsides[i * 2].ny = 0;

		hexsides[i * 2 + 1].nx = nx;
		hexsides[i * 2 + 1].nz = nz;
		hexsides[i * 2 + 1].ny = 0;
	}

	vptr = hexsides + 12;
	for(i=0; i<12; i++) {
		*vptr = hexsides[i];
		vptr->r = vptr->g = vptr->b = 64;
		vptr++;
	}


	tile = tiles;
	for(i=0; i<TILES_YSZ; i++) {
		for(j=0; j<TILES_XSZ; j++) {
			float rad, dx, dy;
			float offs = (float)(i & 1) * 1.5f;
			tile->x = (j - TILES_XSZ / 2) * 3 + offs;
			tile->y = (i - TILES_YSZ / 2) * 0.866;
			tile->height = 0;

			tile->r = rand() & 0xff;
			tile->g = rand() & 0xff;
			tile->b = rand() & 0xff;

			dx = tile->x;
			dy = tile->y;
			tile->hidden = (dx * dx + dy * dy) > MAX_RAD_SQ;
			tile++;
		}
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
	g3d_perspective(VFOV, 1.3333333, 0.5, 500.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_enable(G3D_DEPTH_TEST);
	g3d_enable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);
	g3d_disable(G3D_TEXTURE_2D);

	g3d_enable(G3D_COLOR_MATERIAL);

	g3d_polygon_mode(G3D_GOURAUD);

	g3d_clear_color(0, 0, 0);

	part_start = time_msec;
}

static void stop(long trans_time)
{
	g3d_disable(G3D_COLOR_MATERIAL);
}

static void update(void)
{
	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}


static void draw(void)
{
	int i;
	struct hextile *tile;
	static float prev_tm;
	float dt, tm = (float)(time_msec - part_start) / 1000.0f;
	cgm_vec3 pos;

	dt = tm - prev_tm;
	prev_tm = tm;

	update();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, -2, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	}

	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);

	pos.x = cos(tm * 0.5f) * 4.0f;
	pos.y = OBJ_HEIGHT;
	pos.z = sin(tm * 1.0f) * 4.0f;

	g3d_light_pos(0, pos.x, pos.y, pos.z);

	g3d_enable(G3D_LIGHTING);

	tile = tiles;
	for(i=0; i<NUM_TILES; i++) {
		float dx, dy, dist, d, t, newh;

		if(tile->hidden) {
			tile++;
			continue;
		}

		dx = pos.x - tile->x;
		dy = pos.z - tile->y;
		dist = (dx * dx + dy * dy) * 0.45f;
		d = dist > CGM_PI / 2.0f ? CGM_PI / 2.0f : dist;
		t = cos(d);
		newh = t * 3.0f;

		if(newh > tile->height) {
			tile->height = newh;
		}

		tile->height -= 0.75f * dt;
		if(tile->height < 0.0f) tile->height = 0.0f;

		draw_hextile(tile++, t);
	}

	g3d_disable(G3D_LIGHTING);
	draw_billboard(pos.x, pos.y, pos.z, 0.5f, 255, 255, 255, 255);

	swap_buffers(fb_pixels);
}

static INLINE int clamp(int x, int a, int b)
{
	if(x < a) return a;
	if(x > b) return b;
	return x;
}

#define LR		64
#define LG		80
#define LB		128

#define HR		80
#define HG		128
#define HB		255

static void draw_hextile(struct hextile *tile, float t)
{
	int i, r, g, b;
	int tt = cround64(t * 255.0f);

	r = LR + (((HR - LR) * tt) >> 8);
	g = LG + (((HG - LG) * tt) >> 8);
	b = LB + (((HB - LB) * tt) >> 8);

	for(i=0; i<6; i++) {
		hextop[i].y = tile->height;
		hextop[i].r = r;
		hextop[i].g = g;
		hextop[i].b = b;
	}

	for(i=0; i<12; i++) {
		hexsides[i].y = tile->height;
		hexsides[i].r = r;
		hexsides[i].g = g;
		hexsides[i].b = b;
	}

	g3d_push_matrix();
	g3d_translate(tile->x, 0, tile->y);

	g3d_draw_indexed(G3D_TRIANGLES, hextop, 6, hextop_idx, 12);
	g3d_draw_indexed(G3D_QUADS, hexsides, 24, hexsides_idx, 24);

	g3d_pop_matrix();
}

static void keypress(int key)
{
}
