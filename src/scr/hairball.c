#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "demo.h"
#include "3dgfx.h"
#include "vmath.h"
#include "screen.h"
#include "util.h"
#include "cfgopt.h"
#include "mesh.h"
#include "cgmath/cgmath.h"

#define NUM_TENT		8
#define TENT_NODES		6
#define NODE_DIST		0.4f
#define NUM_FRAMES		32

static cgm_vec3 root_pos[NUM_TENT] = {
	{-0.707, -0.707, 0}, {0.707, -0.707, 0}, {0, -0.707, -0.707}, {0, -0.707, 0.707},
	{-0.5, 0.707, 0.5}, {0.5, 0.707, 0.5}, {0.5, 0.707, -0.5}, {-0.5, 0.707, -0.5}
};

#ifdef MSDOS
#include "dos/gfx.h"	/* for wait_vsync assembly macro */
#else
void wait_vsync(void);
#endif

struct tentacle {
	cgm_vec3 node[TENT_NODES];
};

struct thing {
	cgm_vec3 pos;
	cgm_quat rot;
	float xform[16];
	unsigned int cur_frm;

	struct tentacle orig_tent[NUM_TENT];
	struct tentacle tent[NUM_FRAMES][NUM_TENT];
};

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void keypress(int key);
static void update_thing(float dt);
static void draw_thing(void);


static struct screen scr = {
	"hairball",
	init,
	destroy,
	start, 0,
	draw,
	keypress
};

static long start_time;

static float cam_theta, cam_phi = 15;
static float cam_dist = 8;

static struct thing thing;
static struct g3d_mesh sphmesh;



struct screen *hairball_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j, numpt = 0;

	gen_sphere_mesh(&sphmesh, 0.25f, 8, 4);

	return 0;
}

static void destroy(void)
{
}

static void start(long trans_time)
{
	int i, j;
	cgm_vec3 v, dir;

	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.33333, 0.5, 100.0);

	cgm_midentity(thing.xform);
	cgm_vcons(&thing.pos, 0, 0, 0);
	cgm_qcons(&thing.rot, 0, 0, 0, 1);

	thing.cur_frm = 0;
	for(i=0; i<NUM_TENT; i++) {
		thing.orig_tent[i].node[0] = root_pos[i];
		for(j=1; j<TENT_NODES; j++) {
			v = thing.orig_tent[i].node[j - 1];
			dir = root_pos[i];
			cgm_vnormalize(&dir);
			cgm_vadd_scaled(&v, &dir, NODE_DIST);
			thing.orig_tent[i].node[j] = v;
		}
	}

	for(i=0; i<NUM_FRAMES; i++) {
		for(j=0; j<NUM_TENT; j++) {
			thing.tent[i][j] = thing.orig_tent[j];
		}
	}

	start_time = time_msec;
}

static void update(void)
{
	static float prev_mx, prev_my;
	static long prev_msec;
	int mouse_dx, mouse_dy;
	long msec = time_msec - start_time;
	float dt = (msec - prev_msec) / 1000.0f;
	prev_msec = msec;

	mouse_dx = mouse_x - prev_mx;
	mouse_dy = mouse_y - prev_my;
	prev_mx = mouse_x;
	prev_my = mouse_y;


	if(opt.sball) {
		memcpy(thing.xform, sball_matrix, 16 * sizeof(float));

		thing.pos.x = thing.xform[12];
		thing.pos.y = thing.xform[13];
		thing.pos.z = thing.xform[14];
	} else {
		if(mouse_bmask & MOUSE_BN_MIDDLE) {
			thing.pos.x += mouse_dx * 0.05;
			thing.pos.y -= mouse_dy * 0.05;
		}

		cgm_mrotation_quat(thing.xform, &thing.rot);
		thing.xform[12] = thing.pos.x;
		thing.xform[13] = thing.pos.y;
		thing.xform[14] = thing.pos.z;
	}

	update_thing(dt);

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

static void draw(void)
{
	unsigned long msec;
	static unsigned long last_swap;

	update();

	memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * 2);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);

	draw_thing();

	msec = get_msec();
	if(msec - last_swap < 16) {
		wait_vsync();
	}
	if(!opt.vsync) {
		wait_vsync();
	}
	swap_buffers(fb_pixels);
	last_swap = get_msec();
}

static void keypress(int key)
{
}

static void update_thing(float dt)
{
	int i, j;
	struct tentacle *tent;

	thing.cur_frm = (thing.cur_frm + 1) & (NUM_FRAMES - 1);

	for(i=0; i<NUM_TENT; i++) {
		tent = thing.tent[thing.cur_frm] + i;
		*tent = thing.orig_tent[i];
		for(j=0; j<TENT_NODES; j++) {
			cgm_vmul_m4v3(tent->node + j, thing.xform);
		}
	}
}

#define TENTFRM(x) \
	((thing.cur_frm + NUM_FRAMES - ((x) << 2)) & (NUM_FRAMES - 1))

static void draw_thing(void)
{
	int i, j, col;
	vec3_t prevpos;
	cgm_vec3 *p, *pp;
	struct tentacle *tent;


	g3d_push_matrix();
	g3d_mult_matrix(thing.xform);
	zsort_mesh(&sphmesh);
	draw_mesh(&sphmesh);
	g3d_pop_matrix();

	for(i=0; i<NUM_TENT; i++) {
		g3d_begin(G3D_LINES);
		p = thing.tent[thing.cur_frm][i].node;
		for(j=1; j<TENT_NODES; j++) {
			pp = p;
			p = thing.tent[TENTFRM(j)][i].node + j;

			g3d_color3b(0, 255, 0);
			g3d_vertex(pp->x, pp->y, pp->z);
			g3d_vertex(p->x, p->y, p->z);
		}
		g3d_end();
	}

	g3d_color3b(255, 0, 0);
	g3d_begin(G3D_POINTS);
	for(i=0; i<NUM_TENT; i++) {
		for(j=0; j<TENT_NODES; j++) {
			tent = thing.tent[TENTFRM(j)] + i;
			p = tent->node + j;
			g3d_vertex(p->x, p->y, p->z);
		}
	}
	g3d_end();
}
