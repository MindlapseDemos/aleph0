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

#define VOL_SIZE		24
#define VOL_XSZ			(VOL_SIZE * VOL_XSCALE / VOL_ZSCALE)
#define VOL_YSZ			(VOL_SIZE * VOL_YSCALE / VOL_ZSCALE)
#define VOL_ZSZ			VOL_SIZE
#define VOL_XSCALE		16.0f
#define VOL_YSCALE		15.0f
#define VOL_ZSCALE		10.0f
#define VOL_HALF_XSCALE	(VOL_XSCALE * 0.5f)
#define VOL_HALF_YSCALE	(VOL_YSCALE * 0.5f)
#define VOL_HALF_ZSCALE	(VOL_ZSCALE * 0.5f)

#define NUM_MBALLS	5

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void keyb(int key);
static int gen_sflake(cgm_vec4 *sarr, int num, int depth, float x, float y, float z, float rad);

static struct screen scr = {
	"molten",
	init,
	destroy,
	start, 0,
	draw,
	keyb
};

static float cam_theta, cam_phi;
static float cam_dist = 14;
static struct g3d_mesh mmesh, room_mesh;
static uint16_t *room_texmap, *envmap;
static int roomtex_xsz, roomtex_ysz;
static int envmap_xsz, envmap_ysz;

static struct msurf_volume vol;
static cgm_vec4 ballpos[NUM_MBALLS];


struct screen *molten_screen(void)
{
	return &scr;
}

static int init(void)
{
	if(load_mesh(&room_mesh, "data/moltroom.obj", 0) == -1) {
		fprintf(stderr, "failed to load data/moltroom.obj\n");
		return -1;
	}
	if(!(room_texmap = img_load_pixels("data/moltroom.png", &roomtex_xsz, &roomtex_ysz, IMG_FMT_RGB565))) {
		fprintf(stderr, "failed to load data/moltroom.png\n");
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
	vol.isoval = 8;

	if(msurf_begin(&vol) == -1)	{	/* force allocation now */
		fprintf(stderr, "msurf_begin failed\n");
		msurf_destroy(&vol);
		return -1;
	}

	mmesh.prim = G3D_TRIANGLES;
	mmesh.varr = 0;
	mmesh.iarr = 0;
	mmesh.vcount = mmesh.icount = 0;

	gen_sflake(ballpos, 0, 2, 0, 0, 0, 20);
	vol.flags |= MSURF_FLOOR;
	vol.floor_z = VOL_ZSCALE / 4.0f;
	return 0;
}

static void destroy(void)
{
	msurf_destroy(&vol);
	destroy_mesh(&room_mesh);
	img_free_pixels(room_texmap);
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.3333333, 0.5, 100.0);

	g3d_enable(G3D_DEPTH_TEST);
	g3d_enable(G3D_CULL_FACE);
	g3d_enable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);

	g3d_polygon_mode(G3D_GOURAUD);
}

static void update(void)
{
	int i;

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);

	for(i=0; i<NUM_MBALLS; i++) {
		vol.mballs[i].pos.x = ballpos[i].x;
		vol.mballs[i].pos.y = ballpos[i].y;
		vol.mballs[i].pos.z = ballpos[i].z;
		vol.mballs[i].energy = ballpos[i].w;

		vol.mballs[i].pos.x += VOL_HALF_XSCALE;
		vol.mballs[i].pos.y += VOL_HALF_YSCALE;
		vol.mballs[i].pos.z += VOL_HALF_ZSCALE;
	}

	msurf_begin(&vol);
	msurf_genmesh(&vol);

	mmesh.vcount = vol.num_verts;
	mmesh.varr = vol.varr;
}

static void draw(void)
{
	int i, j;
	char buf[128];
	int x, y, z;
	struct msurf_voxel *vox;
	struct msurf_cell *cell;
	int faces;

	update();

	g3d_clear(G3D_DEPTH_BUFFER_BIT | G3D_COLOR_BUFFER_BIT);	/* XXX drop color */

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 1, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);

	g3d_push_matrix();
	g3d_translate(0, -VOL_YSCALE * 0.89, 0);
	g3d_disable(G3D_LIGHTING);
	g3d_enable(G3D_TEXTURE_2D);
	g3d_set_texture(roomtex_xsz, roomtex_ysz, room_texmap);
	g3d_polygon_mode(G3D_FLAT);
	draw_mesh(&room_mesh);
	g3d_polygon_mode(G3D_GOURAUD);
	g3d_pop_matrix();

	g3d_translate(-VOL_HALF_XSCALE, -VOL_HALF_YSCALE, -VOL_HALF_ZSCALE);

	if(mmesh.vcount) {
		g3d_enable(G3D_LIGHTING);
		g3d_disable(G3D_TEXTURE_2D);
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
}


static int gen_sflake(cgm_vec4 *sarr, int num, int depth, float x, float y, float z, float rad)
{
	int subnum;
	float subrad, offs;

	if(!depth) return 0;

	sarr[num].x = x;
	sarr[num].y = y;
	sarr[num].z = z;
	sarr[num].w = rad;
	num++;

	subrad = rad * 0.4f;
	offs = rad * 0.16f;

	subnum = 0;
	subnum += gen_sflake(sarr, num + subnum, depth - 1, x, y + offs, z, subrad);
	subnum += gen_sflake(sarr, num + subnum, depth - 1, x + offs * 0.75, y - offs * 0.5, z - offs * 0.43, subrad);
	subnum += gen_sflake(sarr, num + subnum, depth - 1, x - offs * 0.75, y - offs * 0.5, z - offs * 0.43, subrad);
	subnum += gen_sflake(sarr, num + subnum, depth - 1, x, y - offs * 0.5, z + offs * 0.86, subrad);
	return subnum + 1;
}

