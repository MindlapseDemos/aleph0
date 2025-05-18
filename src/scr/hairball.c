#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <assert.h>
#include "demo.h"
#include "3dgfx.h"
#include "vmath.h"
#include "screen.h"
#include "util.h"
#include "cfgopt.h"
#include "mesh.h"
#include "cgmath/cgmath.h"
#include "anim.h"
#include "dynarr.h"

#define NUM_TENT		8
#define TENT_NODES		6
#define NODE_DIST		0.35f
#define NUM_FRAMES		32

#define THING_RAD		0.6f

#define TENT_UVERTS		5
#define TENT_NVERTS		(TENT_UVERTS * TENT_NODES + 1)
#define TENT_NTRIS		(TENT_UVERTS * (TENT_NODES - 1) * 2 + TENT_UVERTS)
#define TENT_RAD0		0.25f

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
static void draw_thing(void);
static void draw_backdrop(void);

static void update_thing(void);
static void update_tentacle(struct g3d_mesh *mesh, int tidx);

static void clear_anim(struct anm_animation *anm);
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

static float cam_theta, cam_phi;
static float cam_dist = 8;

static struct thing thing;
static struct g3d_mesh sphmesh;
static struct g3d_mesh tentmesh[NUM_TENT];
static struct g3d_material thingmtl;
static struct image envmap, bgimg;
static int bgscroll_x, bgscroll_y;

#define BGIMG_WIDTH		512
#define BGIMG_HEIGHT	350
#define MAX_BGSCROLL_X	(BGIMG_WIDTH - 320)
#define MAX_BGSCROLL_Y	(BGIMG_HEIGHT - 240)

static int playanim, record;
static long anim_start_time;


struct screen *hairball_screen(void)
{
	return &scr;
}

static int init(void)
{
	if(load_image(&envmap, "data/myenvmap.jpg") == -1) {
		fprintf(stderr, "hairball: failed to load envmap\n");
		return -1;
	}
	if(load_image(&bgimg, "data/space.jpg") == -1) {
		fprintf(stderr, "hairball: failed to load backdrop\n");
		return -1;
	}
	if(bgimg.width != BGIMG_WIDTH || bgimg.height != BGIMG_HEIGHT) {
		fprintf(stderr, "hairball: unexpected bg image size (expected: %dx%d)\n", BGIMG_WIDTH, BGIMG_HEIGHT);
		return -1;
	}

	gen_sphere_mesh(&sphmesh, THING_RAD * 1.3, 6, 3);
	sphmesh.mtl = &thingmtl;

	init_g3dmtl(&thingmtl);
	thingmtl.envmap = &envmap;

	if(anm_init_node(&thing.node) == -1) {
		fprintf(stderr, "failed to initialize thing animation node\n");
		return -1;
	}
	anm_set_extrapolator(&thing.node, ANM_EXTRAP_REPEAT);
	thing.anim = anm_get_animation(&thing.node, 0);
	if(load_anim(thing.anim, "data/thing.anm") != -1) {
		playanim = 1;
	}

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
				int chess = (k & 1) == (j & 1);
				vert->u = (float)k / (float)TENT_NVERTS;
				vert->v = (float)j / (float)TENT_NODES;
				vert->w = 1.0f;
				vert->r = chess ? 255 : 128;
				vert->g = 128;
				vert->b = chess ? 128 : 255;
				vert->a = 255;
				vert++;
			}
		}
		vert->u = 0;
		vert->v = 1.0f;
		vert->r = vert->g = vert->b = vert->a = 255;

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

		if(!playanim) update_tentacle(tentmesh + i, i);
	}

	if(playanim) {
		anm_get_position(&thing.node, &thing.pos.x, 0);
		anm_get_rotation(&thing.node, &thing.rot.x, 0);

		cgm_mrotation_quat(thing.xform, &thing.rot);
		thing.xform[12] = thing.pos.x;
		thing.xform[13] = thing.pos.y;
		thing.xform[14] = thing.pos.z;

		for(i=0; i<NUM_FRAMES; i++) {
			update_thing();
		}
	}

	printf("thing triangles: %d\n", TENT_NTRIS * NUM_TENT + sphmesh.icount / 4);

	start_time = time_msec;
}

static void update(void)
{
	static float prev_mx, prev_my;
	static long prev_thing_upd;
	int mouse_dx, mouse_dy;
	long msec = time_msec - start_time;
	long thing_dt;

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

	bgscroll_x = (int)(thing.pos.x * (MAX_BGSCROLL_X * 0.1f)) + MAX_BGSCROLL_X / 2;
	bgscroll_y = (int)(-thing.pos.y * (MAX_BGSCROLL_Y * 0.3f)) + MAX_BGSCROLL_Y / 2;

	thing_dt = msec - prev_thing_upd;
	if(thing_dt >= 33) {
		while(thing_dt >= 33) {
			thing_dt -= 33;
			update_thing();
		}
		prev_thing_upd = msec;
	}

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
	update();

	g3d_clear(/*G3D_COLOR_BUFFER_BIT | */G3D_DEPTH_BUFFER_BIT);
	draw_backdrop();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);

	draw_thing();

	swap_buffers(fb_pixels);
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

			clear_anim(thing.anim);

			anim_start_time = time_msec;
		}
		break;

	case 's':
		if(thing.anim->tracks[0].count) {
			save_anim(thing.anim, "thing.anm");
		}
		break;
	}
}

#define TENTFRM(x) \
	((thing.cur_frm + NUM_FRAMES - ((x) << 2)) & (NUM_FRAMES - 1))

static void draw_thing(void)
{
	int i;

	g3d_push_matrix();
	g3d_mult_matrix(thing.xform);
	draw_mesh(&sphmesh);
	g3d_pop_matrix();

	for(i=0; i<NUM_TENT; i++) {
		draw_mesh(tentmesh + i);
	}
}

static void draw_backdrop(void)
{
	int i, j;
	uint16_t *dptr, *sptr;

	bgscroll_x &= bgimg.xmask;
	if(bgscroll_y < 0) bgscroll_y = 0;
	if(bgscroll_y >= BGIMG_HEIGHT - 240) bgscroll_y = BGIMG_HEIGHT - 240 - 1;

	dptr = fb_pixels;
	sptr = bgimg.pixels + (bgscroll_y << bgimg.xshift) + bgscroll_x;
	for(i=0; i<240; i++) {
		memcpy64(dptr, sptr, 320 * 2 >> 3);
		dptr += 320;
		sptr += BGIMG_WIDTH;
	}
}

static void update_thing(void)
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
		if(fabs(vk.z) > 0.999) {
			cgm_vcons(&vj, 0, 1, 0);
		} else {
			cgm_vcons(&vj, 0, 0, 1);
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

static void clear_anim(struct anm_animation *anm)
{
	int i;
	for(i=0; i<ANM_NUM_TRACKS; i++) {
		dynarr_clear(anm->tracks[i].keys);
		anm->tracks[i].count = 0;
	}
}

static int load_anim(struct anm_animation *anm, const char *fname)
{
	FILE *fp;
	int nposkeys, nrotkeys;
	float x, y, z, w;
	unsigned int tm;
	char buf[256];
	char *ptr;
	struct anm_keyframe key;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "failed to open %s\n", fname);
		return -1;
	}

	if(!fgets(buf, sizeof buf, fp) || memcmp(buf, "ANIMDUMP", 8) != 0) {
		fprintf(stderr, "%s is not an animation dump\n", fname);
		fclose(fp);
		return -1;
	}
	if(!fgets(buf, sizeof buf, fp) || sscanf(buf, "keys t%d r%d", &nposkeys, &nrotkeys) != 2) {
		fprintf(stderr, "%s: invalid animation dump\n", fname);
		fclose(fp);
		return -1;
	}

	while(fgets(buf, sizeof buf, fp)) {
		ptr = buf;
		while(*ptr && isspace(*ptr)) ptr++;

		switch(*ptr) {
		case 't':
			if(sscanf(ptr, "t(%u): %f %f %f", &tm, &x, &y, &z) != 4) {
				goto inval;
			}
			key.time = tm;
			key.val = x;
			anm_set_keyframe(anm->tracks + ANM_TRACK_POS_X, &key);
			key.val = y;
			anm_set_keyframe(anm->tracks + ANM_TRACK_POS_Y, &key);
			key.val = z;
			anm_set_keyframe(anm->tracks + ANM_TRACK_POS_Z, &key);
			break;

		case 'r':
			if(sscanf(ptr, "r(%u): %f %f %f %f", &tm, &x, &y, &z, &w) != 5) {
				goto inval;
			}
			key.time = tm;
			key.val = x;
			anm_set_keyframe(anm->tracks + ANM_TRACK_ROT_X, &key);
			key.val = y;
			anm_set_keyframe(anm->tracks + ANM_TRACK_ROT_Y, &key);
			key.val = z;
			anm_set_keyframe(anm->tracks + ANM_TRACK_ROT_Z, &key);
			key.val = w;
			anm_set_keyframe(anm->tracks + ANM_TRACK_ROT_W, &key);
			break;

		case '#':
			break;

		default:
			goto inval;
		}
	}

	if(anm->tracks[ANM_TRACK_POS_X].count != nposkeys) {
		fprintf(stderr, "%s: expected %d position keys, found %d\n", fname,
				nposkeys, anm->tracks[ANM_TRACK_POS_X].count);
	}
	if(anm->tracks[ANM_TRACK_ROT_X].count != nrotkeys) {
		fprintf(stderr, "%s: expected %d rotation keys, found %d\n", fname,
				nrotkeys, anm->tracks[ANM_TRACK_ROT_X].count);
	}

	fclose(fp);
	return 0;

inval:
	fprintf(stderr, "%s: invalid animation dump line: %s\n", fname, ptr);
	fclose(fp);
	return -1;
}

static int save_anim(struct anm_animation *anm, const char *fname)
{
	FILE *fp;
	int i, nposkeys, nrotkeys;
	float x, y, z, w;
	unsigned int tm;

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "failed to open %s for writing\n", fname);
		return -1;
	}

	nposkeys = anm->tracks[ANM_TRACK_POS_X].count;
	assert(anm->tracks[ANM_TRACK_POS_Y].count == nposkeys);
	assert(anm->tracks[ANM_TRACK_POS_Z].count == nposkeys);
	nrotkeys = anm->tracks[ANM_TRACK_ROT_X].count;
	assert(anm->tracks[ANM_TRACK_ROT_Y].count == nrotkeys);
	assert(anm->tracks[ANM_TRACK_ROT_Z].count == nrotkeys);
	assert(anm->tracks[ANM_TRACK_ROT_W].count == nrotkeys);

	fprintf(fp, "ANIMDUMP\n");
	fprintf(fp, "keys t%d r%d\n", nposkeys, nrotkeys);

	for(i=0; i<nposkeys; i++) {
		tm = anm->tracks[ANM_TRACK_POS_X].keys[i].time;
		assert(anm->tracks[ANM_TRACK_POS_Y].keys[i].time == tm);
		assert(anm->tracks[ANM_TRACK_POS_Z].keys[i].time == tm);
		x = anm->tracks[ANM_TRACK_POS_X].keys[i].val;
		y = anm->tracks[ANM_TRACK_POS_Y].keys[i].val;
		z = anm->tracks[ANM_TRACK_POS_Z].keys[i].val;
		fprintf(fp, "t(%u): %f %f %f\n", tm, x, y, z);
	}

	for(i=0; i<nrotkeys; i++) {
		tm = anm->tracks[ANM_TRACK_ROT_X].keys[i].time;
		assert(anm->tracks[ANM_TRACK_ROT_Y].keys[i].time == tm);
		assert(anm->tracks[ANM_TRACK_ROT_Z].keys[i].time == tm);
		assert(anm->tracks[ANM_TRACK_ROT_W].keys[i].time == tm);
		x = anm->tracks[ANM_TRACK_ROT_X].keys[i].val;
		y = anm->tracks[ANM_TRACK_ROT_Y].keys[i].val;
		z = anm->tracks[ANM_TRACK_ROT_Z].keys[i].val;
		w = anm->tracks[ANM_TRACK_ROT_W].keys[i].val;
		fprintf(fp, "r(%u): %f %f %f %f\n", tm, x, y, z, w);
	}

	fclose(fp);
	return 0;
}
