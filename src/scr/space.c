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
#include "polyfill.h"
#include "imago2.h"

#define VFOV	60.0f

void clear_anim(struct anm_animation *anm);
int load_anim(struct anm_animation *anm, const char *fname);
int save_anim(struct anm_animation *anm, const char *fname);

static int space_init(void);
static void space_destroy(void);
static void space_start(long trans_time);
static void space_draw(void);
static void draw_lensflare(void);
static void draw_planet(void);
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

#define STARS_RAD	256.0f
#define NUM_STARS	600
static cgm_vec3 stars[NUM_STARS];
static unsigned char starsize[NUM_STARS];
static struct image starimg[2];

static struct image sunimg, flareimg, flareimg2, flarebig, planet;
static struct img_pixmap flashimg;

static struct anm_node anmnode;
static struct anm_animation *anim;
static int playanim, recording;
static long anim_start_time, last_sample_t;
static long anim_dur;
#define HAVE_ANIM	(anim->tracks[0].count > 0)

static float view_matrix[16];

static dseq_event *ev_space, *ev_ship, *ev_warp, *ev_flash, *ev_fadeout, *ev_fadein;


struct screen *space_screen(void)
{
	return &scr;
}


static int space_init(void)
{
	int i, num_cp;
	struct goat3d *g;
	tinymt32_t rnd = {0};
	const char *starimg_name[] = {"data/star_04.png", "data/star_07.png"};

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
	anm_set_interpolator(&anmnode, ANM_INTERP_CUBIC);
	anm_set_extrapolator(&anmnode, ANM_EXTRAP_REPEAT);
	anim = anm_get_animation(&anmnode, 0);
	if(load_anim(anim, "data/space.anm") != -1 && HAVE_ANIM) {
		anim_dur = anim->tracks[0].keys[anim->tracks[0].count - 1].time;
	}

	tinymt32_init(&rnd, 7296824);
	for(i=0; i<NUM_STARS; i++) {
		sphrand(stars + i, STARS_RAD, &rnd);
		starsize[i] = tinymt32_generate_uint32(&rnd) & 0xff;
	}

	for(i=0; i<2; i++) {
		if(load_image(starimg + i, starimg_name[i]) == -1) {
			fprintf(stderr, "failed to load: %s\n", starimg_name[i]);
			return -1;
		}
		conv_rle_alpha(starimg + i);
	}

	if(load_image(&sunimg, "data/sunflare.png") == -1) {
		fprintf(stderr, "failed to load sun image\n");
		return -1;
	}
	conv_rle_alpha(&sunimg);

	if(load_image(&flareimg, "data/flare1.png") == -1) {
		fprintf(stderr, "failed to load flare image\n");
		return -1;
	}
	conv_rle_alpha(&flareimg);

	if(load_image(&flareimg2, "data/flare2.png") == -1) {
		fprintf(stderr, "failed to load flare image\n");
		return -1;
	}
	conv_rle_alpha(&flareimg2);

	if(load_image(&flarebig, "data/flare3.png") == -1) {
		fprintf(stderr, "failed to load flare image\n");
		return -1;
	}
	conv_rle_alpha(&flarebig);

	if(load_image(&planet, "data/planet.png") == -1) {
		fprintf(stderr, "failed to load planet sprite\n");
		return -1;
	}
	conv_rle_alpha(&planet);

	img_init(&flashimg);
	if(img_load(&flashimg, "data/addflare.png") == -1) {
		fprintf(stderr, "failed to load flash flare image\n");
		return -1;
	}

	ev_space = dseq_lookup("space");
	ev_ship = dseq_lookup("space.ship");
	ev_warp = dseq_lookup("space.warp");
	ev_flash = dseq_lookup("space.flash");
	ev_fadeout = dseq_lookup("space.fadeout");
	ev_fadein = dseq_lookup("space.fadein");
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

static const uint16_t star_coltab[] = {
	PACK_RGB16(26, 26, 26),
	PACK_RGB16(64, 64, 64),
	PACK_RGB16(64, 64, 64),
	PACK_RGB16(128, 128, 128),
	PACK_RGB16(128, 128, 128),
	PACK_RGB16(190, 190, 190),
	PACK_RGB16(190, 190, 190),
	PACK_RGB16(255, 255, 255)
};

static void space_draw(void)
{
	int i;
	float xform[16];
	cgm_vec4 pt;
	float warp;

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
			unsigned int sz = starsize[i];

			if(sz > 251) {
				blendfb_rle(fb_pixels, x - starimg[1].width / 2, y - starimg[1].height / 2, starimg + 1);
			} else if(sz > 230) {
				blendfb_rle(fb_pixels, x - starimg[0].width / 2, y - starimg[0].height / 2, starimg);
			} else {
				unsigned int idx = sz >> 5;
				fb_pixels[(y << 8) + (y << 6) + x] = star_coltab[idx];
			}
		}
	}

	draw_planet();


	if(!dseq_started() || dseq_isactive(ev_ship) || dseq_isactive(ev_warp)) {
		warp = cgm_logerp(3, 300, dseq_param(ev_warp));

		g3d_push_matrix();
		g3d_rotate(180, 0, 1, 0);
		g3d_translate(0, 0, warp);

		g3d_enable(G3D_LIGHTING);
		draw_mesh(mesh_hull);
		g3d_disable(G3D_LIGHTING);
		draw_mesh(mesh_lit);
		g3d_pop_matrix();
	}

	draw_lensflare();

	if(dseq_isactive(ev_flash)) {
		unsigned int val = cround64(dseq_value(ev_flash) * 256.0f);
		struct img_colormap *cmap = img_colormap(&flashimg);
		unsigned int pal[256 * 4];
		for(i=0; i<cmap->ncolors; i++) {
			pal[i * 4] = (cmap->color[i].r * val) >> 8;
			pal[i * 4 + 1] = (cmap->color[i].g * val) >> 8;
			pal[i * 4 + 2] = (cmap->color[i].b * val) >> 8;
		}
		overlay_add_pal(fb_pixels + 250, flashimg.pixels, flashimg.width, flashimg.height, flashimg.width, pal);
	}

	if(dseq_isactive(ev_fadein)) {
		unsigned int i, r, g, b, pix;
		uint16_t *fbptr = fb_pixels;
		unsigned int fade = cround64(dseq_value(ev_fadein));
		for(i=0; i<320*240; i++) {
			pix = *fbptr;
			r = 255u + ((((unsigned int)UNPACK_R16(pix) - 255u) * fade) >> 8);
			g = 255u + ((((unsigned int)UNPACK_G16(pix) - 255u) * fade) >> 8);
			b = 255u + ((((unsigned int)UNPACK_B16(pix) - 255u) * fade) >> 8);
			*fbptr++ = PACK_RGB16(r, g, b);
		}
	}

	if(dseq_isactive(ev_fadeout)) {
		unsigned int i, r, g, b, val = cround64(dseq_value(ev_fadeout));
		uint16_t *fbptr = fb_pixels;
		for(i=0; i<320*240; i++) {
			uint16_t pcol = *fbptr;
			r = (UNPACK_R16(pcol) * val) >> 8;
			g = (UNPACK_G16(pcol) * val) >> 8;
			b = (UNPACK_B16(pcol) * val) >> 8;
			*fbptr++ = PACK_RGB16(r, g, b);
		}
	}

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

static void draw_lensflare(void)
{
	int px, py, px2, py2;
	uint32_t zval;
	cgm_vec4 pt = {209, 104, 104, 1};

	if(!g3d_xform_point(&pt.x)) return;

	px = cround64(pt.x);
	py = cround64(pt.y);

	zval = pfill_zbuf[(py << 8) + (py << 6) + px];
	if(zval < 0xffffff) {
		return;
	}

	blendfb_rle(fb_pixels, px - sunimg.width / 2, py - sunimg.height / 2, &sunimg);

	px = 320 - px;
	py = 240 - py;
	blendfb_rle(fb_pixels, px - flareimg.width / 2, py - flareimg.height / 2, &flareimg);

	px2 = (px - 160) * 5 / 8 + 160;
	py2 = (py - 120) * 5 / 8 + 120;
	blendfb_rle(fb_pixels, px2 - flareimg2.width / 2, py2 - flareimg2.height / 2, &flareimg2);

	px = (px - 160) * 6 / 4 + 160;
	py = (py - 120) * 6 / 4 + 120;
	blendfb_rle(fb_pixels, px - flarebig.width / 2, py - flarebig.height / 2, &flarebig);
}

static void draw_planet(void)
{
	int px, py;
	cgm_vec4 pt = {-190, -25, -120, 1};

	g3d_xform_point(&pt.x);

	if(pt.z < -pt.w) return;

	px = cround64(pt.x);
	py = cround64(pt.y);

	blendfb_rle(fb_pixels, px - planet.width / 2, py - planet.height / 2, &planet);
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
