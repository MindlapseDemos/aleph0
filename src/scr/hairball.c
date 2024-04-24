#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "demo.h"
#include "3dgfx.h"
#include "track.h"
#include "vmath.h"
#include "screen.h"
#include "util.h"
#include "cfgopt.h"
#include "mesh.h"
#include "cgmath/cgmath.h"
#include "anim.h"

#define NUM_TENT		8
#define TENT_NODES		6
#define NODE_DIST		0.35f
#define NUM_FRAMES		32

#define THING_RAD		0.6f

#define TENT_UVERTS		5
#define TENT_NVERTS		(TENT_UVERTS * TENT_NODES + 1)
#define TENT_NTRIS		(TENT_UVERTS * (TENT_NODES - 1) * 2 + TENT_UVERTS)
#define TENT_RAD0		0.26f

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

	struct anm_node node;
	struct anm_animation *anim;
};

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void keypress(int key);
static void update_thing(float dt);
static void draw_thing(void);
static void update_tentacle(struct g3d_mesh *mesh, int tidx);

static int load_anim(struct anm_animation *anm, const char *fname);
static int save_anim(struct anm_animation *anm, const char *fname);


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
static struct g3d_mesh tentmesh[NUM_TENT];
static struct g3d_material thingmtl;
static struct image envmap;

static int playanim, record;
static long anim_start_time;


struct screen *hairball_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j, numpt = 0;
	struct anm_animation *anim;

	if(load_image(&envmap, "data/refmap1.jpg") == -1) {
		fprintf(stderr, "hairball: failed to load envmap\n");
		return -1;
	}

	gen_sphere_mesh(&sphmesh, THING_RAD * 1.2, 6, 3);
	sphmesh.mtl = &thingmtl;

	init_g3dmtl(&thingmtl);
	thingmtl.envmap = &envmap;

	if(anm_init_node(&thing.node) == -1) {
		fprintf(stderr, "failed to initialize thing animation node\n");
		return -1;
	}
	anm_set_extrapolator(&thing.node, ANM_EXTRAP_REPEAT);
	thing.anim = anm_get_animation(&thing.node, 0);
	load_anim(thing.anim, "data/thing.anm");

	return 0;
}

static void destroy(void)
{
}

static void start(long trans_time)
{
	int i, j, k;
	cgm_vec3 v, dir;
	struct g3d_vertex *vert;
	uint16_t *idxptr, vidx;

	g3d_enable(G3D_DEPTH_TEST);
	g3d_enable(G3D_CULL_FACE);
	g3d_enable(G3D_LIGHT0);

	g3d_polygon_mode(G3D_GOURAUD);

	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.33333, 0.5, 100.0);

	cgm_midentity(thing.xform);
	cgm_vcons(&thing.pos, 0, 0, 0);
	cgm_qcons(&thing.rot, 0, 0, 0, 1);

	thing.cur_frm = 0;
	for(i=0; i<NUM_TENT; i++) {
		fast_vnormalize(root_pos + i);

		thing.orig_tent[i].node[0] = root_pos[i];
		cgm_vscale(thing.orig_tent[i].node, THING_RAD);

		for(j=1; j<TENT_NODES; j++) {
			v = thing.orig_tent[i].node[j - 1];
			dir = root_pos[i];
			cgm_vadd_scaled(&v, &dir, NODE_DIST);
			thing.orig_tent[i].node[j] = v;
		}
	}

	for(i=0; i<NUM_FRAMES; i++) {
		for(j=0; j<NUM_TENT; j++) {
			thing.tent[i][j] = thing.orig_tent[j];
		}
	}

	for(i=0; i<NUM_TENT; i++) {
		init_mesh(tentmesh + i, G3D_TRIANGLES, TENT_NVERTS, TENT_NTRIS * 3);
		tentmesh[i].mtl = sphmesh.mtl;

		vert = tentmesh[i].varr;
		for(j=0; j<TENT_NODES; j++) {
			for(k=0; k<TENT_UVERTS; k++) {
				vert->u = (float)k / (float)TENT_NVERTS;
				vert->v = (float)j / (float)TENT_NODES;
				vert->w = 1.0f;
				vert->r = vert->g = vert->b = vert->a = 1.0f;
				vert++;
			}
		}
		vert->u = 0;
		vert->v = 1.0f;
		vert->r = vert->g = vert->b = vert->a = 1.0f;

		vidx = 0;
		idxptr = tentmesh[i].iarr;
		for(j=0; j<TENT_NODES - 1; j++) {
			for(k=0; k<TENT_UVERTS; k++) {
				idxptr[0] = vidx + k;
				idxptr[1] = vidx + (k + 1) % TENT_UVERTS;
				idxptr[2] = vidx + TENT_UVERTS + k;
				idxptr[3] = idxptr[1];
				idxptr[4] = vidx + TENT_UVERTS + (k + 1) % TENT_UVERTS;
				idxptr[5] = idxptr[2];
				idxptr += 6;
			}
			vidx += TENT_UVERTS;
		}

		for(j=0; j<TENT_UVERTS; j++) {
			idxptr[0] = vidx + j;
			idxptr[1] = vidx + (j + 1) % TENT_UVERTS;
			idxptr[2] = TENT_NVERTS - 1;
			idxptr += 3;
		}

		update_tentacle(tentmesh + i, i);
	}

	printf("thing triangles: %d\n", TENT_NTRIS * NUM_TENT + sphmesh.icount / 4);

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


	if(opt.sball && !playanim) {
		memcpy(thing.xform, sball_matrix, 16 * sizeof(float));

		thing.pos = sball_pos;
		thing.rot = sball_rot;

	} else {
		if(playanim) {
			long atime = time_msec - anim_start_time;

			anm_get_position(&thing.node, &thing.pos.x, atime);
			anm_get_rotation(&thing.node, &thing.rot.x, atime);
		} else {
			if(mouse_bmask & MOUSE_BN_MIDDLE) {
				thing.pos.x += mouse_dx * 0.05;
				thing.pos.y -= mouse_dy * 0.05;
			}
		}

		cgm_mrotation_quat(thing.xform, &thing.rot);
		thing.xform[12] = thing.pos.x;
		thing.xform[13] = thing.pos.y;
		thing.xform[14] = thing.pos.z;
	}

	update_thing(dt);

	if(record) {
		static long last_sample_t;
		long atime = time_msec - anim_start_time;

		if(atime - last_sample_t >= 500) {
			anm_set_position(&thing.node, &thing.pos.x, atime);
			anm_set_rotation(&thing.node, &thing.rot.x, atime);
			last_sample_t = atime;
		}
	}

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

static void draw(void)
{
	unsigned long msec;
	static unsigned long last_swap;

	update();

	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);

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
	switch(key) {
	case ' ':
		playanim ^= 1;
		if(playanim) {
			anm_use_animation(&thing.node, 0);
			anim_start_time = time_msec;
		}
		break;

	case 'r':
		record ^= 1;
		if(record) {
			playanim = 0;

			/* XXX handle repeated recordings */

			anim_start_time = time_msec;
		}
		break;
	}
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

		update_tentacle(tentmesh + i, i);
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

	g3d_enable(G3D_LIGHTING);

	g3d_push_matrix();
	g3d_mult_matrix(thing.xform);
	/*zsort_mesh(&sphmesh);*/
	draw_mesh(&sphmesh);
	g3d_pop_matrix();

	for(i=0; i<NUM_TENT; i++) {
		/*zsort_mesh(tentmesh + i);*/
		draw_mesh(tentmesh + i);
	}

	g3d_disable(G3D_LIGHTING);

	/*
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
	*/
}

static void update_tentacle(struct g3d_mesh *mesh, int tidx)
{
	int i, j;
	cgm_vec3 vi, vj, vk;
	float theta, xform[16];
	cgm_vec3 cent, v, prev, next;
	struct g3d_vertex *vptr;
	struct tentacle *tent;
	float rad;

	vptr = mesh->varr;

	for(i=0; i<TENT_NODES; i++) {
		rad = TENT_RAD0 * (1.0f - (float)i / (float)TENT_NODES);

		tent = thing.tent[TENTFRM(i)] + tidx;
		prev = i > 0 ? thing.tent[TENTFRM(i - 1)][tidx].node[i - 1] : thing.pos;
		next = i < TENT_NODES - 1 ? thing.tent[TENTFRM(i + 1)][tidx].node[i + 1] : tent->node[i];
		cent = tent->node[i];

		vk = next;
		cgm_vsub(&vk, &prev);
		fast_vnormalize(&vk);
		if(fabs(vk.y) > fabs(vk.x) && fabs(vk.y) > fabs(vk.z)) {
			cgm_vcons(&vj, 0, 0, -1);
		} else {
			cgm_vcons(&vj, 0, 1, 0);
		}
		cgm_vcross(&vi, &vj, &vk);
		fast_vnormalize(&vi);
		cgm_vcross(&vj, &vk, &vi);

		xform[0] = vi.x; xform[1] = vi.y; xform[2] = vi.z;
		xform[4] = vj.x; xform[5] = vj.y; xform[6] = vj.z;
		xform[8] = vk.x; xform[9] = vk.y; xform[10] = vk.z;
		xform[3] = xform[7] = xform[11] = xform[12] = xform[13] = xform[14] = 0.0f;
		xform[15] = 1.0f;

		for(j=0; j<TENT_UVERTS; j++) {
			theta = (float)j * M_PI * 2.0f / (float)TENT_UVERTS;
			cgm_vcons(&v, cos(theta), sin(theta), 0);

			cgm_vmul_m3v3(&v, xform);
			vptr->nx = v.x;
			vptr->ny = v.y;
			vptr->nz = v.z;

			cgm_vscale(&v, rad);
			vptr->x = v.x + cent.x;
			vptr->y = v.y + cent.y;
			vptr->z = v.z + cent.z;

			vptr++;
		}
	}

	/* tip of the tentacle */
	vptr->nx = vk.x;
	vptr->ny = vk.y;
	vptr->nz = vk.z;
	vptr->x = cent.x + vk.x * 0.5f;
	vptr->y = cent.y + vk.y * 0.5f;
	vptr->z = cent.z + vk.z * 0.5f;
}

static int load_anim(struct anm_animation *anm, const char *fname)
{
	return -1;
}

static int save_anim(struct anm_animation *anm, const char *fname)
{
	return -1;
}
