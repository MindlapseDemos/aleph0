#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "cgmath/cgmath.h"
#include "screen.h"
#include "demo.h"
#include "3dgfx.h"
#include "gfxutil.h"
#include "util.h"
#include "msurf2.h"
#include "mesh.h"
#include "imago2.h"

#undef DBG_VISIT

#define VOL_SIZE		14
#define VOL_XSZ			((int)(VOL_SIZE * VOL_XSCALE / VOL_YSCALE))
#define VOL_YSZ			10
#define VOL_ZSZ			((int)(VOL_SIZE * VOL_ZSCALE / VOL_YSCALE))
#define VOL_XSCALE		16.5f
#define VOL_YSCALE		11.5f
#define VOL_ZSCALE		15.0f
#define VOL_HALF_XSCALE	(VOL_XSCALE * 0.5f)
#define VOL_HALF_YSCALE	(VOL_YSCALE * 0.5f)
#define VOL_HALF_ZSCALE	(VOL_ZSCALE * 0.5f)

#define NUM_MBALLS	5

static int molten_init(void);
static void molten_destroy(void);
static void molten_start(long trans_time);
static void molten_draw(void);
static void molten_keyb(int key);
static int gen_sflake(cgm_vec4 *sarr, int num, int depth, float x, float y, float z, float rad);

static struct screen scr = {
	"molten",
	molten_init,
	molten_destroy,
	molten_start, 0,
	molten_draw,
	molten_keyb
};

static float cam_theta, cam_phi;
static float cam_dist = 14;
static cgm_vec3 viewpos = {0, 1, 14}, viewtgt;
static struct g3d_mesh mmesh, room_mesh;
static uint16_t *room_texmap, *envmap;
static int roomtex_xsz, roomtex_ysz;
static int envmap_xsz, envmap_ysz;

static struct msurf_volume vol;
static cgm_vec4 ballpos[NUM_MBALLS];
static int show_voxdbg, show_wire;

static unsigned long start_time;
static float prev_upd;

static int ev_rise;


struct screen *molten_screen(void)
{
	return &scr;
}

static int molten_init(void)
{
	if(load_mesh(&room_mesh, "data/moltroom.obj", 0) == -1) {
		fprintf(stderr, "failed to load data/moltroom.obj\n");
		return -1;
	}
	if(!(room_texmap = img_load_pixels("data/moltroom.png", &roomtex_xsz, &roomtex_ysz, IMG_FMT_RGB565))) {
		fprintf(stderr, "failed to load data/moltroom.png\n");
		return -1;
	}

	if(!(envmap = img_load_pixels("data/myenvmap.jpg", &envmap_xsz, &envmap_ysz, IMG_FMT_RGB565))) {
		fprintf(stderr, "failed to load data/myenvmap.jpg\n");
		return -1;
	}

	printf("molten: init msurf volume %dx%dx%d\n", VOL_XSZ, VOL_YSZ, VOL_ZSZ);
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

	gen_sflake(ballpos, 0, 2, 0, 0, 0, 18);
	vol.flags |= MSURF_FLOOR;
	vol.floor_z = 0;
	vol.floor_energy = 8.5;

	ev_rise = dseq_lookup("molten.rise");
	return 0;
}

static void molten_destroy(void)
{
	msurf_destroy(&vol);
	destroy_mesh(&room_mesh);
	img_free_pixels(room_texmap);
}

static void molten_start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(60.0, 1.3333333, 0.5, 100.0);

	g3d_enable(G3D_DEPTH_TEST);
	g3d_enable(G3D_CULL_FACE);
	/*g3d_enable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);*/
	g3d_texture_mode(G3D_TEX_REPLACE);
	g3d_polygon_mode(G3D_FLAT);

	start_time = time_msec;
	prev_upd = time_msec;
	cgm_vcons(&viewtgt, 0, 0, 0);
}

static void molten_update(void)
{
	float dt;
	int i;
	float tsec = (float)(time_msec - start_time) / 1000.0f;
	float xform[16], dist, rise_val;
	cgm_vec3 pos, newpos;

	dt = tsec - prev_upd;
	/*assert(dt >= 0 && dt < 0.5);*/
	prev_upd = tsec;

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);

	cgm_midentity(xform);
	cgm_mrotate(xform, tsec, 1, 0, 0);
	cgm_mrotate(xform, tsec, 0, 1, 0);

	if(dseq_started()) {
		rise_val = (float)dseq_value(ev_rise) / 1024.0f;
	} else {
		rise_val = 1.0f;
	}

	dist = cos(tsec * 1.8f) * 0.25f + 1.1f;

	for(i=0; i<NUM_MBALLS; i++) {
		pos.x = ballpos[i].x * dist;
		pos.y = ballpos[i].y * dist;
		pos.z = ballpos[i].z * dist - 4.0f;
		cgm_vmul_m3v3(&pos, xform);
		pos.y *= 0.5f;
		pos.z -= 1.0f;

		vol.mballs[i].energy = ballpos[i].w;

		pos.z += cos(tsec * 0.7f) * 4.0f;
		pos.z = cgm_lerp(-VOL_HALF_ZSCALE-1.0f, pos.z, rise_val);
		if(i == 0) {
			cgm_vcons(&newpos, pos.x, pos.z, pos.y);
		}

		vol.mballs[i].pos.x = pos.x + VOL_HALF_XSCALE;
		vol.mballs[i].pos.y = pos.y + VOL_HALF_YSCALE;
		vol.mballs[i].pos.z = pos.z + VOL_HALF_ZSCALE;
	}

	msurf_begin(&vol);
	msurf_genmesh(&vol);

	mmesh.vcount = vol.num_verts;
	mmesh.varr = vol.varr;

	/*viewpos.x = sin(tsec * 0.4f) * 3.0;*/

	cgm_vlerp(&viewtgt, &viewtgt, &newpos, 0.75f * dt);
}

static void molten_draw(void)
{
	char buf[128];
	int x, y, z;
	struct msurf_voxel *vox;
	float viewmat[16];
	static const cgm_vec3 upvec = {0, 1, 0};

	molten_update();

	/*
	g3d_clear_color(255, 0, 0);
	g3d_clear(G3D_DEPTH_BUFFER_BIT | G3D_COLOR_BUFFER_BIT);
	*/
	g3d_clear(G3D_DEPTH_BUFFER_BIT);

	g3d_matrix_mode(G3D_MODELVIEW);
	/*
	g3d_load_identity();
	g3d_translate(0, 1, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	*/
	cgm_minv_lookat(viewmat, &viewpos, &viewtgt, &upvec);
	g3d_load_matrix(viewmat);

	g3d_push_matrix();
	g3d_translate(0, -VOL_HALF_ZSCALE * 0.85, 0);
	g3d_disable(G3D_LIGHTING);
	g3d_enable(G3D_TEXTURE_2D);
	g3d_set_texture(roomtex_xsz, roomtex_ysz, room_texmap);
	draw_mesh(&room_mesh);
	g3d_pop_matrix();

	g3d_rotate(-90, 1, 0, 0);
	g3d_translate(-VOL_HALF_XSCALE + 0.4, -VOL_HALF_YSCALE, -VOL_HALF_ZSCALE);

	if(mmesh.vcount) {
		if(show_wire) {
			g3d_disable(G3D_TEXTURE_2D);
			g3d_polygon_mode(G3D_WIRE);
			draw_mesh(&mmesh);
			g3d_polygon_mode(G3D_FLAT);
		} else {
			g3d_set_texture(envmap_xsz, envmap_ysz, envmap);
			g3d_enable(G3D_TEXTURE_2D);
			g3d_enable(G3D_TEXTURE_GEN);
			draw_mesh(&mmesh);
			g3d_disable(G3D_TEXTURE_GEN);
		}
	}

	g3d_disable(G3D_TEXTURE_2D);

	if(show_voxdbg) {
		int r, g, b;
		unsigned int frmid = vol.cur & 0xffff;
		vox = vol.voxels;
		g3d_disable(G3D_LIGHTING);
		g3d_begin(G3D_POINTS);
		for(z=0; z<vol.zres; z++) {
			for(y=0; y<vol.yres; y++) {
				for(x=0; x<vol.xres; x++) {
					/*if(vox->val > 8 || (vox->flags & 0xffff) == frmid) {*/
						r = vox->val > 8 ? 255 : 64;
						g = 64;
						b = (vox->flags & 0xffff) == frmid ? 255 : 64;
						g3d_color3b(r, g, b);
						g3d_vertex(vox->pos.x, vox->pos.y, vox->pos.z);
					/*}*/
					vox++;
				}
				vox += vol.xstore - vol.xres;
			}
			vox += (vol.ystore - vol.yres) << vol.xshift;
		}
		g3d_end();
	}

	if(opt.dbgmode) {
		sprintf(buf, "%d tris", mmesh.vcount / 3);
		cs_cputs(fb_pixels, 10, 10, buf);
	}

	swap_buffers(fb_pixels);
}

static void molten_keyb(int key)
{
	switch(key) {
	case 'd':
		show_voxdbg ^= 1;
		break;

	case 'w':
		show_wire ^= 1;
		break;
	}
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

