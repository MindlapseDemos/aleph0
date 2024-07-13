#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "screen.h"
#include "demo.h"
#include "3dgfx.h"
#include "gfxutil.h"
#include "util.h"
#include "msurf2.h"
#include "mesh.h"
#include "imago2.h"

#undef DBG_VISIT

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void keyb(int key);
static void shade_blobs(void);

static struct screen scr = {
	"metaballs",
	init,
	destroy,
	start, 0,
	draw,
	keyb
};

static float cam_theta, cam_phi;
static float cam_dist = 8;
static struct g3d_mesh mmesh;
static uint16_t *bgimage, *envmap;
static int envmap_xsz, envmap_ysz;
static int use_envmap = 1, opt_shade = 1;

#define NUM_SPR		4
static struct image spr[NUM_SPR];
static const char *sprfile[NUM_SPR] = {"data/blob_top.png", "data/blob_bot.png",
	"data/blobterm.png", "data/blobgirl.png"};

static struct msurf_volume vol;

#define VOL_SIZE	24
#define VOL_XSZ	VOL_SIZE
#define VOL_YSZ	VOL_SIZE
#define VOL_ZSZ	VOL_SIZE
#define VOL_XSCALE	10.0f
#define VOL_YSCALE	10.0f
#define VOL_ZSCALE	10.0f
#define VOL_HALF_XSCALE	(VOL_XSCALE * 0.5f)
#define VOL_HALF_YSCALE	(VOL_YSCALE * 0.5f)
#define VOL_HALF_ZSCALE	(VOL_ZSCALE * 0.5f)

#define NUM_MBALLS	3

static int evid_faces;


struct screen *metaballs_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j, x, y, z, xsz, ysz;

	if(!(bgimage = img_load_pixels("data/blob_bg.png", &xsz, &ysz, IMG_FMT_RGB565))) {
		return -1;
	}
	for(i=0; i<NUM_SPR; i++) {
		if(load_image(spr + i, sprfile[i]) == -1) {
			return -1;
		}
		if(conv_rle(spr + i, 0xf81f) == -1) {
			destroy_image(spr + i);
			return -1;
		}
	}
	if(!(envmap = img_load_pixels("data/metaenv.jpg", &envmap_xsz, &envmap_ysz, IMG_FMT_RGB565))) {
		fprintf(stderr, "failed to load metaballs envmap\n");
		return -1;
	}

	if(msurf_init(&vol) == -1) {
		fprintf(stderr, "failed to initialize metasurf\n");
		return -1;
	}
	if(msurf_metaballs(&vol, NUM_MBALLS) == -1) {
		msurf_destroy(&vol);
		return -1;
	}
	msurf_resolution(&vol, VOL_XSZ, VOL_YSZ, VOL_ZSZ);
	msurf_size(&vol, VOL_XSCALE, VOL_YSCALE, VOL_ZSCALE);
	vol.isoval = 1.7;

	vol.mballs[0].energy = 1.2;
	vol.mballs[1].energy = 0.8;
	vol.mballs[2].energy = 1.0;

	if(msurf_begin(&vol) == -1) {	/* force allocation now */
		msurf_destroy(&vol);
		return -1;
	}

	mmesh.prim = G3D_TRIANGLES;
	mmesh.varr = 0;
	mmesh.iarr = 0;
	mmesh.vcount = mmesh.icount = 0;

	evid_faces = dseq_lookup("metaballs.faces");
	return 0;
}

static void destroy(void)
{
	msurf_destroy(&vol);
	img_free_pixels(bgimage);
}

static void start(long trans_time)
{

	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.3333333, 0.5, 100.0);

	g3d_enable(G3D_DEPTH_TEST);
	g3d_enable(G3D_CULL_FACE);
	g3d_disable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);

	g3d_polygon_mode(G3D_GOURAUD);
}

static void update(void)
{
	int i, j;
	float tsec = time_msec / 1000.0f;
	static float phase[] = {0.0, M_PI / 3.0, M_PI * 0.8};
	static float speed[] = {0.8, 1.4, 1.0};
	static float scale[][3] = {{1, 2, 0.8}, {0.5, 1.6, 0.6}, {1.5, 0.7, 0.5}};
	static float offset[][3] = {{0, 0, 0}, {0.25, 0, 0}, {-0.2, 0.15, 0.2}};

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);

	for(i=0; i<NUM_MBALLS; i++) {
		float t = (tsec + phase[i]) * speed[i];

		for(j=0; j<3; j++) {
			float x = sin(t + j * M_PI / 2.0);
			(&vol.mballs[i].pos.x)[j] = offset[i][j] + x * scale[i][j];
		}

		vol.mballs[i].pos.x += VOL_HALF_XSCALE;
		vol.mballs[i].pos.y += VOL_HALF_YSCALE;
		vol.mballs[i].pos.z += VOL_HALF_ZSCALE;
	}

	msurf_begin(&vol);
	msurf_genmesh(&vol);

	mmesh.vcount = vol.num_verts;
	mmesh.varr = vol.varr;
}

extern int dbg_visited;

static void draw(void)
{
	int i, j;
	char buf[128];
	int x, y, z;
	struct msurf_voxel *vox;
	struct msurf_cell *cell;
	int faces;

	update();

	g3d_clear(G3D_DEPTH_BUFFER_BIT);
	memcpy64(fb_pixels, bgimage, 320 * 240 / 4);

	/* sprites */
	blitfb_rle(fb_pixels, 160 - 121/2, 0, spr);
	blitfb_rle(fb_pixels, 160 - 197/2, 240 - 45, spr + 1);
	if((faces = dseq_value(evid_faces))) {
		int offs = faces * 98 >> 10;
		blitfb_rle(fb_pixels, offs - 98, 20, spr + 2);
		blitfb_rle(fb_pixels, 320 - offs, 21, spr + 3);
	}

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);

	g3d_light_pos(0, -10, 10, 20);

	/*zsort_mesh(&mmesh);*/
	if(opt_shade) {
		shade_blobs();
		g3d_texture_mode(G3D_TEX_ADD);
	} else {
		g3d_texture_mode(G3D_TEX_MODULATE);
	}

	g3d_mtl_diffuse(0.6, 0.6, 0.6);

	g3d_translate(-VOL_HALF_XSCALE, -VOL_HALF_YSCALE, -VOL_HALF_ZSCALE);

	if(use_envmap) {
		g3d_enable(G3D_TEXTURE_2D);
		g3d_enable(G3D_TEXTURE_GEN);
		g3d_set_texture(envmap_xsz, envmap_ysz, envmap);
		draw_mesh(&mmesh);
		g3d_texture_mode(G3D_TEX_MODULATE);
		g3d_disable(G3D_TEXTURE_GEN);
		g3d_disable(G3D_TEXTURE_2D);
	} else {
		draw_mesh(&mmesh);
	}

	if(opt.dbgmode) {
		sprintf(buf, "%d tris", mmesh.vcount / 3);
		cs_cputs(fb_pixels, 10, 10, buf);
	}

	swap_buffers(fb_pixels);
}

static void keyb(int key)
{
	switch(key) {
	case 'e':
		use_envmap ^= 1;
		break;

	case 's':
		opt_shade ^= 1;
		break;
	}
}

static cgm_vec3 blobcol[2] = {
	{0.05, 0.05, 0.15},
	{0.9, 0.3, 0.05}
};

static void shade_blobs(void)
{
	int i;
	struct g3d_vertex *varr;
	cgm_vec3 *n, col;
	float t, y, vgrad;

	varr = mmesh.varr;
	for(i=0; i<mmesh.vcount; i++) {
		/*n = (cgm_vec3*)&varr->nx;*/
		y = varr->y - VOL_HALF_YSCALE;
		vgrad = fabs(4.0 * y / VOL_ZSCALE);
		t = fabs(varr->ny) * vgrad;
		cgm_vlerp(&col, blobcol, blobcol + 1, t);
		varr->r = cround64(col.x * 255.0f);
		varr->g = cround64(col.y * 255.0f);
		varr->b = cround64(col.z * 255.0f);
		varr++;
	}
}
