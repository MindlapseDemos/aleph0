#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "demo.h"
#include "3dgfx.h"
#include "screen.h"
#include "gfxutil.h"
#include "mesh.h"
#include "image.h"
#include "util.h"
#include "cgmath/cgmath.h"

#define TM_RIPPLE_START		1.0f
#define TM_RIPPLE_TRANS_LEN	3.0f

#define VFOV	50.0f
#define HFOV	(VFOV * 1.333333f)

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void draw_mountains(void);

static struct screen scr = {
	"cybersun",
	init,
	destroy,
	start,
	0,
	draw
};

static float cam_theta = 0, cam_phi = 0;
static float cam_dist = 0;

static struct g3d_mesh gmesh;
#define GMESH_GRIDSZ	25
#define GMESH_SIZE		128
static struct image gtex;

static struct image mounttex;

static long part_start;


struct screen *cybersun_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j;

	if(gen_plane_mesh(&gmesh, GMESH_SIZE, GMESH_SIZE, GMESH_GRIDSZ, GMESH_GRIDSZ) == -1) {
		return -1;
	}
	for(i=0; i<gmesh.vcount; i++) {
		gmesh.varr[i].u *= GMESH_GRIDSZ;
		gmesh.varr[i].v *= GMESH_GRIDSZ;
	}
	if(load_image(&gtex, "data/pgrid.png") == -1) {
		return -1;
	}
	if(load_image(&mounttex, "data/cybmount.png") == -1) {
		return -1;
	}
	assert(mounttex.width == 512);
	assert(mounttex.height == 64);

	return 0;
}

static void destroy(void)
{
	destroy_mesh(&gmesh);
	destroy_image(&gtex);
	destroy_image(&mounttex);
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(VFOV, 1.3333333, 0.5, 500.0);

	g3d_enable(G3D_CULL_FACE);

	g3d_clear_color(85, 70, 136);

	part_start = time_msec;
}

static void update(void)
{
	int i, j;
	float t = (time_msec - part_start) / 1000.0f;
	struct g3d_vertex *vptr;

	float ampl = cgm_smoothstep(TM_RIPPLE_START, TM_RIPPLE_START + TM_RIPPLE_TRANS_LEN, t);

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);

	/* update mesh */
	vptr = gmesh.varr;
	for(i=0; i<GMESH_GRIDSZ + 1; i++) {
		for(j=0; j<GMESH_GRIDSZ + 1; j++) {
			float u = (float)j / GMESH_GRIDSZ - 0.5f;
			float v = (float)i / GMESH_GRIDSZ - 0.5f;
			float x = u * 32.0f;
			float y = v * 32.0f;
			float r = sqrt(x * x + y * y);

			vptr->z = sin(x * 0.5 + t) + cos(x * 0.8f) * 0.5f;
			vptr->z += cos(y * 0.5 + t);
			vptr->z += sin(r + t) * 0.5f;
			vptr->z *= r * 0.1f > 1.0f ? 1.0f : r * 0.1f;
			vptr->z *= ampl;
			vptr++;
		}
	}
}

static void draw(void)
{
	int i;

	update();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, -2, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	}

	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);
	draw_mountains();

	g3d_set_texture(gtex.width, gtex.height, gtex.pixels);
	g3d_enable(G3D_TEXTURE_2D);
	g3d_enable(G3D_DEPTH_TEST);

	g3d_push_matrix();
	g3d_rotate(-90, 1, 0, 0);
	draw_mesh(&gmesh);
	g3d_pop_matrix();

	g3d_disable(G3D_DEPTH_TEST);
	g3d_disable(G3D_TEXTURE_2D);

	swap_buffers(fb_pixels);
}

/* XXX all the sptr calculations assume mounttex.width == 512 */
static void draw_mountains(void)
{
	int i, j, y;
	int32_t x, xstart, xend, dx;
	uint16_t *dptr, *sptr;

	xstart = cround64(cam_theta * (256.0 * 512.0 / 90.0));	/* 24.8 fixed point, 512 width */
	xend = cround64((cam_theta + HFOV) * (256.0 * 512.0 / 90.0));
	dx = (xend - xstart) / FB_WIDTH;
	x = xstart;

	y = cround64(-cam_phi * (FB_HEIGHT / 45.0)) + (FB_HEIGHT / 2 - 64);

	dptr = fb_pixels + y * FB_WIDTH;

	for(i=0; i<FB_WIDTH; i++) {
		sptr = mounttex.pixels + ((x >> 8) & 0x1ff);

		for(j=0; j<64; j++) {
			if(j + y >= FB_HEIGHT) break;
			if(j + y >= 0) {
				int32_t col = sptr[j << 9];
				if(col != 0x7e0) {
					dptr[j * FB_WIDTH] = col;
				}
			}
		}
		dptr++;

		x += dx;
	}

	y += 64;
	if(y < 0) {
		memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * 2);
	} else {
		if(y < FB_HEIGHT) {
			dptr = fb_pixels + y * FB_WIDTH;
			memset(dptr, 0, (FB_HEIGHT - y) * FB_WIDTH * 2);
		}
	}
}
