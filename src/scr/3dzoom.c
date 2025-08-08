#include <stdlib.h>
#include <math.h>
#include "demo.h"
#include "3dgfx.h"
#include "mesh.h"
#include "scene.h"
#include "screen.h"
#include "util.h"
#include "gfxutil.h"
#include "dynarr.h"
#include "curve.h"
#include "polyclip.h"

#define VFOV	80.0f


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void stop(long trans_time);
static void draw(void);
static void keypress(int key);
static int presort_cmp(const void *a, const void *b);


static struct screen scr = {
	"reactor",
	init,
	destroy,
	start,
	stop,
	draw,
	keypress
};

extern struct image infcube_envmap;	/* defined & loaded in infcubes.c */

static float cam_theta, cam_phi;
static float cam_dist = 80;

static struct g3d_scene *scn;
static struct g3d_mesh *poolmesh, *cubemesh;

static struct curve campath;

static int dbgfreelook;
static dseq_event *ev_reactor;

struct screen *zoom3d_screen(void)
{
	return &scr;
}

/* defined in space.c */
int g3dass_load(struct goat3d *g, const char *fname);

static int init(void)
{
	int i, num_cp;
	struct goat3d *g;

	if(!(g = goat3d_create()) || g3dass_load(g, "data/3dzoom/reactor.g3d") == -1) {
		goat3d_free(g);
		return -1;
	}
	scn = scn_create();
	scn->texpath = "data/3dzoom";

	conv_goat3d_scene(scn, g);
	goat3d_free(g);

	poolmesh = scn_find_mesh(scn, "3-Plane");
	for(i=0; i<poolmesh->vcount; i++) {
		poolmesh->varr[i].nx = poolmesh->varr[i].u;
		poolmesh->varr[i].ny = poolmesh->varr[i].v;
	}

	cubemesh = scn_find_mesh(scn, "5-cube");

	if(!infcube_envmap.pixels && load_image(&infcube_envmap, "data/refmap1.jpg") == -1) {
		return -1;
	}
	cubemesh->mtl->envmap = &infcube_envmap;

	qsort(scn->meshes, scn_mesh_count(scn), sizeof(struct g3d_mesh*), presort_cmp);

	if(crv_load(&campath, "data/rcampath.crv") == -1) {
		fprintf(stderr, "reactor: failed to load camera path spline\n");
		goat3d_free(g);
		return -1;
	}
	num_cp = crv_num_points(&campath);
	for(i=0; i<num_cp; i++) {
		cgm_vscale(campath.cp + i, 0.036f);
	}

	ev_reactor = dseq_lookup("reactor");
	return 0;
}

static void destroy(void)
{
}

static void start(long trans_time)
{
	/*g3d_framebuffer(240, 240, fb_pixels + 320 - 240, 320);*/

	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(VFOV, 1.3333333, 2.0, 300.0);

	g3d_enable(G3D_CULL_FACE);
	g3d_disable(G3D_DEPTH_TEST);
	g3d_disable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);
	g3d_polygon_mode(G3D_FLAT);
	g3d_texture_mode(G3D_TEX_REPLACE);

	g3d_clear_color(0, 0, 0);
}

static void stop(long trans_time)
{
	g3d_framebuffer(320, 240, fb_pixels, 320);
	g3d_texture_mode(G3D_TEX_MODULATE);
}

static cgm_vec3 campos, camtarg = {0};
static float cam_t;

static void update(void)
{
	int i;
	float texanim = (float)(time_msec & 8191) / 8192.0f;
	struct g3d_vertex *vptr = poolmesh->varr;

	for(i=0; i<poolmesh->vcount; i++) {
		vptr->u = vptr->nx + texanim;
		vptr->v = vptr->ny + texanim;
		vptr++;
	}

	cam_t = dseq_param(ev_reactor);
	crv_eval(&campath, cam_t, &campos);
	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

static void draw(void)
{
	static unsigned int frontface = G3D_CCW;
	float tm;
	unsigned int i, num;
	float viewmat[16];
	cgm_vec3 up = {0, 1, 0};
	char buf[64];

	update();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	if(dseq_started()) {
		g3d_rotate_z(0.4);
		cgm_minv_lookat(viewmat, &campos, &camtarg, &up);
		g3d_mult_matrix(viewmat);
	} else {
		g3d_translate(0, -0.5, -cam_dist);
		g3d_rotate(cam_phi, 1, 0, 0);
		g3d_rotate(cam_theta, 0, 1, 0);
		if(opt.sball) {
			g3d_mult_matrix(sball_matrix);
		}
	}

	g3d_clear(G3D_COLOR_BUFFER_BIT);

	num = scn_mesh_count(scn);
	for(i=0; i<num - 1; i++) {		/* -1 to skip the cube */
		draw_mesh(scn->meshes[i]);
	}

	tm = (float)time_msec * 0.1;
	g3d_translate(0, 4, 0);
	g3d_rotate(tm, 1, 0, 0);
	g3d_rotate(tm, 0, 1, 0);
	g3d_translate(0, -8.4, 0);

	clipper_flags = 0;
	g3d_front_face(frontface);
	draw_mesh(scn->meshes[i]);
	g3d_front_face(G3D_CCW);

	if(clipper_flags & (1 << CLIP_NEAR)) {
		frontface = G3D_CW;
	} else {
		frontface = G3D_CCW;
	}

	swap_buffers(0);
}

static void keypress(int key)
{
	switch(key) {
	case ' ':
		dbgfreelook ^= 1;
		break;
	}
}

/* comparison function for pre-sorting nodes by name */
static int presort_cmp(const void *a, const void *b)
{
	const struct g3d_mesh *na = *(const struct g3d_mesh**)a;
	const struct g3d_mesh *nb = *(const struct g3d_mesh**)b;

	return strcmp(na->name, nb->name);
}
