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

#define VFOV	50.0f


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);


static struct screen scr = {
	"break",
	init,
	destroy,
	start,
	0,
	draw
};

static float cam_theta = -20, cam_phi = 15;
static float cam_dist = 5;

static long part_start;

static struct g3d_scene *scn;
static struct g3d_anim *anim;

static struct image testimg;	/* XXX */
static uint16_t *testimg_pixels;
static unsigned char *testimg_alpha;


struct screen *break_screen(void)
{
	return &scr;
}


static int init(void)
{
	int i, j;
	struct goat3d *g;

	if(!(g = goat3d_create()) || goat3d_load(g, "data/fracscn.g3d") == -1) {
		goat3d_free(g);
		return -1;
	}
	scn = scn_create();

	conv_goat3d_scene(scn, g);
	goat3d_free(g);

	if(scn_anim_count(scn) > 0) {
		scn_merge_anims(scn);
		anim = scn->anims[0];

		printf("animation timeline %ld -> %ld (duration: %ld)\n", anim->start,
				anim->end, anim->dur);
	}

	/* alpha blending test */
	if(load_image(&testimg, "data/blendtst.png") == -1) {
		fprintf(stderr, "failed to load blend test image\n");
		return -1;
	}
	testimg_pixels = testimg.pixels;
	testimg_alpha = testimg.alpha;

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
	g3d_polygon_mode(G3D_GOURAUD);
	g3d_disable(G3D_TEXTURE_2D);

	g3d_clear_color(0, 0, 0);

	part_start = time_msec;
}


static void update(void)
{
	if(anim) {
		long msec = time_msec - part_start;
		scn_eval_anim(anim, msec % anim->dur + anim->start);
	}

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

static void draw(void)
{
	int i;

	update();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, -0.5, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	}

	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);

	scn_draw(scn);

	/* XXX alpha overlay test */
	{
		int xpos = (time_msec >> 3) % 640;
		xpos = 319 - abs(xpos - 320);

		testimg.pixels = testimg_pixels + xpos;
		testimg.alpha = testimg_alpha + xpos;
		overlay_alpha(&fbimg, 0, 0, &testimg, 320, 240);
	}

	swap_buffers(fb_pixels);
}
