#include <stdlib.h>
#include <math.h>
#include "demo.h"
#include "3dgfx.h"
#include "mesh.h"
#include "screen.h"
#include "util.h"

#define VFOV	50.0f
#define HFOV	(VFOV * 1.333333f)


struct hextile {
	float x, y, height;
	float rad;
	unsigned char r, g, b;
};

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void draw_hextile(struct hextile *tile);


static struct screen scr = {
	"hexfloor",
	init,
	destroy,
	start,
	0,
	draw
};

static float cam_theta = 0, cam_phi = 0;
static float cam_dist = 10;

static long part_start;

static struct g3d_mesh foomesh;

static struct g3d_vertex hextop[12];
static uint16_t hextop_idx[12] = {
	0, 5, 4, 0, 4, 3, 0, 3, 1, 1, 3, 2
};
static uint16_t hexsides_idx[] = {
	1, 7, 6, 0,
	2, 8, 7, 1,
	3, 9, 8, 2,
	4, 10, 9, 3,
	5, 11, 10, 4,
	0, 6, 11, 5
};

#define TILES_XSZ	8
#define TILES_YSZ	16
static struct hextile tiles[TILES_XSZ * TILES_YSZ];


struct screen *hexfloor_screen(void)
{
	return &scr;
}


static int init(void)
{
	int i, j;
	float theta;
	struct hextile *tile;

	for(i=0; i<6; i++) {
		theta = (float)i / 6.0f * (M_PI * 2.0);
		hextop[i].x = cos(theta);
		hextop[i].z = sin(theta);
		hextop[i].y = 0;

		hextop[i].r = rand() & 0xff;
		hextop[i].g = rand() & 0xff;
		hextop[i].b = rand() & 0xff;

		hextop[i + 6] = hextop[i];
	}

	tile = tiles;
	for(i=0; i<TILES_YSZ; i++) {
		for(j=0; j<TILES_XSZ; j++) {
			float offs = (float)(i & 1) * 1.5f;
			tile->x = (j - TILES_XSZ / 2) * 3 + offs;
			tile->y = (i - TILES_YSZ / 2) * hextop[1].z;
			tile->height = 0;

			tile->r = rand() & 0xff;
			tile->g = rand() & 0xff;
			tile->b = rand() & 0xff;

			tile->rad = sqrt(tile->x * tile->x + tile->y * tile->y);

			tile++;
		}
	}


	gen_torus_mesh(&foomesh, 3, 1, 20, 10);


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

	g3d_clear_color(85, 70, 136);

	part_start = time_msec;
}

static void update(void)
{
	int i;

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

static void draw(void)
{
	int i;
	struct hextile *tile;
	float tm = (float)(time_msec - part_start) / 100.0f;

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

	tile = tiles;
	for(i=0; i<TILES_XSZ * TILES_YSZ; i++) {
		float r = tile->rad;
		tile->height = (1.5 + sin(r + tm)) * 2.0 * (r ? 1.0 / r : 1.0f);
		draw_hextile(tile++);
	}

	swap_buffers(fb_pixels);
}

static void draw_hextile(struct hextile *tile)
{
	int i;

	for(i=0; i<6; i++) {
		hextop[i].y = tile->height;
		hextop[i].r = tile->r;
		hextop[i].g = tile->g;
		hextop[i].b = tile->b;
	}

	g3d_push_matrix();
	g3d_translate(tile->x, 0, tile->y);

	g3d_draw_indexed(G3D_TRIANGLES, hextop, 6, hextop_idx, 12);
	g3d_draw_indexed(G3D_QUADS, hextop, 6, hexsides_idx, sizeof hexsides_idx / sizeof *hexsides_idx);

	g3d_pop_matrix();
}
