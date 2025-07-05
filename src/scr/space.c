#include <stdlib.h>
#include <math.h>
#include "demo.h"
#include "3dgfx.h"
#include "dseq.h"
#include "mesh.h"
#include "scene.h"
#include "screen.h"
#include "util.h"
#include "gfxutil.h"
#include "dynarr.h"
#include "curve.h"
#include "anim.h"
#include "tinymt32.h"

#define VFOV	60.0f

void clear_anim(struct anm_animation *anm);
int load_anim(struct anm_animation *anm, const char *fname);
int save_anim(struct anm_animation *anm, const char *fname);

static int space_init(void);
static void space_destroy(void);
static void space_start(long trans_time);
static void space_draw(void);
static void space_keypress(int key);

static void sphrand(cgm_vec3 *pt, float rad, tinymt32_t *rnd);


static struct screen scr = {
	"space",
	space_init,
	space_destroy,
	space_start,
	0,
	space_draw,
	space_keypress
};

static float cam_theta, cam_phi;
static float cam_dist;

static struct g3d_scene *scn;
static struct g3d_mesh *mesh_hull, *mesh_lit;

#define NUM_STARS	256
static cgm_vec3 stars[NUM_STARS];
static unsigned char starsize[NUM_STARS];

static struct anm_node anmnode;
static struct anm_animation *anim;
static int playanim, recording;
static long anim_start_time, last_sample_t;
static long anim_dur;
#define HAVE_ANIM	(anim->tracks[0].count > 0)

static float view_matrix[16];

static dseq_event *ev_space, *ev_ship;


struct screen *space_screen(void)
{
	return &scr;
}


static int space_init(void)
{
	int i, num_cp;
	struct goat3d *g;
	tinymt32_t rnd;

	if(!(g = goat3d_create()) || goat3d_load(g, "data/frig8.g3d") == -1) {
		goat3d_free(g);
		return -1;
	}
	scn = scn_create();
	scn->texpath = "data";

	conv_goat3d_scene(scn, g);
	goat3d_free(g);

	mesh_hull = scn_find_mesh(scn, "hull");
	mesh_lit = scn_find_mesh(scn, "emissive");

	if(anm_init_node(&anmnode) == -1) {
		fprintf(stderr, "failed to initialize animation node for the space camera\n");
		return -1;
	}
	anm_set_extrapolator(&anmnode, ANM_EXTRAP_REPEAT);
	anim = anm_get_animation(&anmnode, 0);
	if(load_anim(anim, "data/space.anm") != -1 && HAVE_ANIM) {
		anim_dur = anim->tracks[0].keys[anim->tracks[0].count - 1].time;
	}

	tinymt32_init(&rnd, 0xbadf00d);
	for(i=0; i<NUM_STARS; i++) {
		sphrand(stars + i, 128.0f, &rnd);
		starsize[i] = (tinymt32_generate_uint32(&rnd) & 0x7f) + 0x80;
	}

	ev_space = dseq_lookup("space");
	ev_ship = dseq_lookup("space.ship");
	return 0;
}


static void space_destroy(void)
{
}

static void space_start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(VFOV, 1.3333333, 1.0, 200.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_enable(G3D_DEPTH_TEST);
	g3d_enable(G3D_LIGHT0);
	g3d_polygon_mode(G3D_FLAT);

	g3d_clear_color(0, 0, 0);

	if(HAVE_ANIM) {
		playanim = 1;
		anim_start_time = time_msec;
	}
}

static void update(void)
{
	if(recording) {
		long atime = time_msec - anim_start_time;

		if(atime - last_sample_t >= 500) {
			anm_set_position(&anmnode, &sball_view_pos.x, atime);
			anm_set_rotation(&anmnode, &sball_view_rot.x, atime);
			last_sample_t = atime;
		}
		memcpy(view_matrix, sball_view_matrix, sizeof view_matrix);

	} else if(playanim) {
		long atime;
		cgm_vec3 pos;
		cgm_quat rot;
		float rmat[16];

		if(dseq_started()) {
			atime = cround64(dseq_param(ev_ship) * anim_dur);
		} else {
			atime = time_msec - anim_start_time;
		}
		anm_get_position(&anmnode, &pos.x, atime);
		anm_get_rotation(&anmnode, &rot.x, atime);
		cgm_qnormalize(&rot);

		cgm_mrotation_quat(rmat, &rot);

		cgm_mtranslation(view_matrix, -pos.x, -pos.y, -pos.z);
		cgm_mmul(view_matrix, rmat);
		/*anm_get_node_matrix(&anmnode, view_matrix, atime);
		cgm_minverse(view_matrix);*/

	} else if(opt.sball) {
		memcpy(view_matrix, sball_view_matrix, sizeof view_matrix);
	} else {
		cgm_midentity(view_matrix);
	}

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

static void space_draw(void)
{
	int i;
	float xform[16];
	cgm_vec4 pt;

	update();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, -0.5, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	g3d_mult_matrix(view_matrix);

	g3d_light_dir(0, 1, 0.5, 0.5);

	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);

	for(i=0; i<NUM_STARS; i++) {
		cgm_wcons(&pt, stars[i].x, stars[i].y, stars[i].z, 1);
		if(g3d_xform_point(&pt.x)) {
			int x = cround64(pt.x);
			int y = cround64(pt.y);
			uint16_t col = PACK_RGB16(starsize[i], starsize[i], starsize[i]);
			fb_pixels[(y << 8) + (y << 6) + x] = col;
		}
	}

	g3d_rotate(180, 0, 1, 0);
	g3d_translate(0, 0, 3);

	g3d_enable(G3D_LIGHTING);
	draw_mesh(mesh_hull);
	g3d_disable(G3D_LIGHTING);
	draw_mesh(mesh_lit);

	if(opt.dbgmode) {
		unsigned int i;
		char buf[64];
		if(recording) {
			sprintf(buf, "recording: %ldms ...", time_msec - anim_start_time);
		} else if(playanim) {
			sprintf(buf, "playing: %ldms ...", time_msec - anim_start_time);
		} else if(HAVE_ANIM) {
			sprintf(buf, "animation: %ldms", anim->tracks[0].keys[anim->tracks[0].count - 1].time);
		} else {
			strcpy(buf, "animation: -");
		}
		cs_cputs(fb_pixels, 10, 10, buf);
	}

	swap_buffers(0);
}

static void space_keypress(int key)
{
	switch(key) {
	case ' ':
		if(HAVE_ANIM) {
			playanim ^= 1;
			if(playanim) {
				anm_use_animation(&anmnode, 0);
				anim_start_time = time_msec;
			}
		}
		break;

	case 'r':
		recording ^= 1;
		if(recording) {
			playanim = 0;

			clear_anim(anim);
			anim_start_time = time_msec;
			last_sample_t = -65536;
		} else {
			anim_dur = anim->tracks[0].keys[anim->tracks[0].count - 1].time;
		}
		break;

	case 's':
		if(HAVE_ANIM) {
			save_anim(anim, "space.anm");
		}
		break;
	}
}

static void sphrand(cgm_vec3 *pt, float rad, tinymt32_t *rnd)
{
	float u, v, theta, phi;

	u = tinymt32_generate_float(rnd);
	v = tinymt32_generate_float(rnd);

	theta = 2.0f * CGM_PI * u;
	phi = acos(2.0f * v - 1.0f);

	pt->x = cos(theta) * sin(phi) * rad;
	pt->y = sin(theta) * sin(phi) * rad;
	pt->z = cos(phi) * rad;
}
