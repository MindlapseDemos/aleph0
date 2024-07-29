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
	float rad;
	float vvel;
	float lt;
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

static float cam_theta = 45, cam_phi = 20;
static float cam_dist = 12;

static long part_start;

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
#define TILES_YSZ	32
#define NUM_TILES	(TILES_XSZ * TILES_YSZ)
static struct hextile tiles[NUM_TILES];

#define OBJ_HEIGHT	3.0f


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
			tile->vvel = 0.0f;

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
	g3d_disable(G3D_LIGHTING);
	g3d_disable(G3D_TEXTURE_2D);

	g3d_polygon_mode(G3D_FLAT);

	g3d_clear_color(85, 70, 136);

	part_start = time_msec;
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

	tile = tiles;
	for(i=0; i<NUM_TILES; i++) {
		float dx = pos.x - tile->x;
		float dy = pos.z - tile->y;
		float rdsq = rsqrt(dx * dx + dy * dy);
		float force;

		force = rdsq * 5.0f - 2.0f;

		tile->vvel = (tile->vvel * 0.9 + force) * dt;
		tile->height += tile->vvel;
		if(tile->height > OBJ_HEIGHT * 0.9f) {
			tile->height = OBJ_HEIGHT * 0.9f;
			tile->vvel = 0.0f;
		}
		if(tile->height < 0.0f) {
			tile->height = 0.0f;
			tile->vvel = 0.0f;
		}

		draw_hextile(tile++);
	}

	draw_billboard(pos.x, pos.y, pos.z, 0.5f, 255, 255, 255, 255);

	swap_buffers(fb_pixels);
}

static INLINE int clamp(int x, int a, int b)
{
	if(x < a) return a;
	if(x > b) return b;
	return x;
}

static void draw_hextile(struct hextile *tile)
{
	int i, r, g, b;

	for(i=0; i<12; i++) {
		if(i < 6) {
			hextop[i].y = tile->height;
		}
		r = (int)(tile->r * tile->height);
		g = (int)(tile->g * tile->height);
		b = (int)(tile->b * tile->height);
		hextop[i].r = clamp(r, 16, 255);
		hextop[i].g = clamp(g, 16, 255);
		hextop[i].b = clamp(b, 16, 255);
	}

	g3d_push_matrix();
	g3d_translate(tile->x, 0, tile->y);

	g3d_draw_indexed(G3D_TRIANGLES, hextop, 6, hextop_idx, 12);
	g3d_draw_indexed(G3D_QUADS, hextop, 6, hexsides_idx, sizeof hexsides_idx / sizeof *hexsides_idx);

	g3d_pop_matrix();
}
