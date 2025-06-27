#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cgmath/cgmath.h"
#include "demo.h"
#include "3dgfx.h"
#include "dseq.h"
#include "mesh.h"
#include "screen.h"
#include "util.h"
#include "gfxutil.h"
#include "curve.h"

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

static cgm_vec2 hexv[6];
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
#define NUM_PATH_POINTS		24

static struct curve path;
static dseq_event *ev_hexfloor;

static float prev_tm;


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
		hexv[i].x = cos(angle[i]) * HEX_SCALE;
		hexv[i].y = sin(angle[i]) * HEX_SCALE;
	}

	for(i=0; i<6; i++) {
		hextop[i].x = hexv[i].x;
		hextop[i].z = hexv[i].y;
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
		vptr->r = 32;
		vptr->g = 24;
		vptr->b = 48;
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

	crv_init(&path);
	path.type = CURVE_BSPLINE;
	for(i=0; i<NUM_PATH_POINTS; i++) {
		float t = (float)i / (NUM_PATH_POINTS - 1) * CGM_PI * 2.0f;
		cgm_vec3 p;
		p.x = cos(t * 3.0f) * 7.0f;
		p.z = sin(t * 2.0f) * 7.0f;
		p.y = OBJ_HEIGHT;
		crv_add(&path, &p);
	}
	ev_hexfloor = dseq_lookup("hexfloor");

	return 0;
}

static void destroy(void)
{
	crv_destroy(&path);
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
	g3d_light_ambient(0.4, 0.4, 0.4);

	part_start = time_msec;
	prev_tm = 0;
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
	float dt, tm = (float)(time_msec - part_start) / 1000.0f;
	float t_obj, t_campos, t_camtarg;
	cgm_vec3 pos, targ, up = {0, 1, 0};
	float viewmat[16];

	dt = tm - prev_tm;
	prev_tm = tm;

	update();

	t_campos = dseq_param(ev_hexfloor);
	t_camtarg = t_campos;
	t_obj = t_camtarg + 0.01;


	g3d_matrix_mode(G3D_MODELVIEW);
	if(dseq_started()) {
		t_campos *= CGM_PI * 2.0f;
		pos.x = cos(t_campos) * 12.0f;
		pos.z = sin(t_campos * 3.0f) * 12.0f;
		pos.y = OBJ_HEIGHT + 1.5f;
		/*crv_eval(&path, fmod(t_campos, 1.0f), &pos);*/
		crv_eval(&path, fmod(t_camtarg, 1.0f), &targ);

		targ.y = OBJ_HEIGHT * 0.4;

		cgm_minv_lookat(viewmat, &pos, &targ, &up);
		g3d_load_matrix(viewmat);
	} else {
		g3d_load_identity();
		g3d_translate(0, -2, -cam_dist);
		g3d_rotate(cam_phi, 1, 0, 0);
		g3d_rotate(cam_theta, 0, 1, 0);
		if(opt.sball) {
			g3d_mult_matrix(sball_matrix);
		}
	}

	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);

	crv_eval(&path, t_obj, &pos);

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

	/*
	g3d_begin(G3D_POINTS);
	g3d_color3f(0, 0.5, 0);
	for(i=0; i<4096; i++) {
		crv_eval(&path, (float)i / 4095.0f, &pos);
		g3d_vertex(pos.x, pos.y, pos.z);
	}

	g3d_color3f(1, 0, 0);
	for(i=0; i<crv_num_points(&path); i++) {
		pos = path.cp[i];
		g3d_vertex(pos.x, pos.y, pos.z);
	}
	g3d_end();
	*/

	swap_buffers(0);
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
	struct g3d_vertex cv;
	struct g3d_vertex *vptr;

	r = LR + (((HR - LR) * tt) >> 8);
	g = LG + (((HG - LG) * tt) >> 8);
	b = LB + (((HB - LB) * tt) >> 8);

	cv.x = tile->x;
	cv.z = tile->y;
	cv.y = tile->height;
	cv.r = r;
	cv.g = g;
	cv.b = b;
	cv.nx = cv.nz = 0;
	cv.ny = 1;

	g3d_viewspace_vertex(&cv);
	g3d_shade(&cv);

	vptr = hextop;
	for(i=0; i<6; i++) {
		vptr->x = hexv[i].x + tile->x;
		vptr->z = hexv[i].y + tile->y;
		vptr->y = tile->height;
		vptr->r = cv.r;
		vptr->g = cv.g;
		vptr->b = cv.b;
		vptr++;
	}

	/* for hex TOPs lighting is computed once for the whole face at the top
	 * of this function, to avoid discontinuities due to different angles of
	 * each triangle the hex is made of. Flat shading is all we need for all
	 * of them.
	 */
	g3d_disable(G3D_LIGHTING);
	g3d_polygon_mode(G3D_FLAT);
	g3d_draw_indexed(G3D_TRIANGLES, hextop, 6, hextop_idx, 12);

	/* only draw the sides if the tile is not at 0 level, let it compute
	 * lighting, and do gouraud to show a nice gradient due to color-material
	 * at the sides (see init for the color of the bottom vertices)
	 */
	if(tile->height > 0.0f) {
		float x = hextop->x;
		float z = hextop->z;

		vptr = hexsides;
		vptr->x = x;
		vptr->z = z;
		vptr[11].x = x;
		vptr[11].z = z;
		vptr += 2;
		for(i=1; i<6; i++) {
			x = hextop[i].x;
			z = hextop[i].z;
			vptr->x = x;
			vptr->z = z;
			vptr[-1].x = x;
			vptr[-1].z = z;
			vptr += 2;
		}

		vptr = hexsides;
		for(i=0; i<12; i++) {
			vptr[12].x = vptr->x;
			vptr[12].z = vptr->z;
			vptr->y = tile->height;
			vptr->r = r;
			vptr->g = g;
			vptr->b = b;
			vptr++;
		}

		g3d_enable(G3D_LIGHTING);
		g3d_polygon_mode(G3D_GOURAUD);
		g3d_draw_indexed(G3D_QUADS, hexsides, 24, hexsides_idx, 24);
	}
}

static void keypress(int key)
{
}
